#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <list>
#include <mutex>
#include <ESP.h>

#include "config.h"

struct Measure
{
	unsigned long int timestamp;
	unsigned short int temperature;
	unsigned short int flow;
};

// Variables declarations
WiFiClient tcpClient;
volatile unsigned short int transitionCount(0);

std::list<Measure> measuresQueue;
std::mutex measuresListMutex;
std::mutex flowMutex;
unsigned long int lastSentTimestamp(0);
unsigned long int wifiConnectTimestamp(0);
unsigned long previousMeasureTime (0);


std::mutex logMutex;
std::list<String> logList;


// Setup Functions prototypes
void grabMeasureSetup();
void measureQueueTimerSetup();
void ntpSetup();

// Callback Functions prototypes
void ntpTimerCallback(void *arg);
void measureQueueCallback(void *arg);
void grabMeasureCallback(void *arg);
void counterCallback();

// Misc Functions prototypes
bool sendMeasure(Measure *measure);
unsigned long getTime();
void log(String function, String message);

// Debug Functions prototypes
String getLocalTimeStr();
String getMeasureStr(const Measure &measure);
size_t getListMemorySize();


void measureQueueCallback(void *arg)
{
	// Check connection
	if (WiFi.status() != WL_CONNECTED || !tcpClient.connected()) {
		return;
	}

	// Check if there a new response receipted
	if (tcpClient.available()) {
		uint32_t receivedTimestamp;
		tcpClient.readBytes((char *)&receivedTimestamp, sizeof(receivedTimestamp));
		log("measureQueueCallback", "Response received: " + String(receivedTimestamp));

		if (lastSentTimestamp == receivedTimestamp){

		log("measureQueueCallback", "Remove measure of the queue:" + String(receivedTimestamp));
			measuresListMutex.lock();
			measuresQueue.pop_front();
			measuresListMutex.unlock();
			lastSentTimestamp = 0;
		}
	}
		
	// If there are some measures in the queue
	if (!measuresQueue.empty()){

		// If there is no measure waiting for response, send the first measure of the queue
		if (lastSentTimestamp == 0) {		
			measuresListMutex.lock();
			Measure firstMeasure = measuresQueue.front();
			measuresListMutex.unlock();

			if(sendMeasure(&firstMeasure)){
				lastSentTimestamp = firstMeasure.timestamp;
				log("measureQueueCallback", "Send succesfully: " + String(lastSentTimestamp));
			}
			else{
				log("measureQueueCallback", "Fail send: " + String(lastSentTimestamp));
			}

		}
		// If the response is not receipted before "RESPONSE_TIMEOUT", stop waiting to try a new send on the next step of the timer
		else if (getTime() - lastSentTimestamp >= RESPONSE_TIMEOUT) {
			log("measureQueueCallback", "Send timeout: " + String(lastSentTimestamp));
			lastSentTimestamp = 0;
		}
		// Else, wait for response or RESPONSE_TIMEOUT
	}
}


// Functions definition
void setup()
{
//#ifdef DEBUG
	Serial.begin(BAUD_RATE);
	Serial.println("setup - Start");
//#endif

	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	Serial.println("setup - WiFi.begin() OK");
	measureQueueTimerSetup();
	Serial.println("setup - measureQueueTimerSetup OK");
	grabMeasureSetup();
	Serial.println("setup - grabMeasureSetup() OK");
	ntpSetup();
	Serial.println("setup - ntpSetup() OK");

	pinMode(TEMPERATURE_PIN, INPUT);

#ifdef DEBUG
	Serial.println("setup - sizeof(Measure) = " + String(sizeof(Measure)));
	logList.clear();
#endif
}

void loop()
{
#ifdef DEBUG
	logMutex.lock();
	while(!logList.empty()){	
		Serial.println(logList.back());
		logList.pop_back();
	}
	logMutex.unlock();
#endif

	unsigned long currentMillis = millis();
	if (currentMillis - previousMeasureTime >= NETWORK_PERIOD) {

		// If wifi and tcp are connected, return directly
		if (WiFi.status() == WL_CONNECTED && tcpClient.connected()) {
			return;
		}

		// If wifi is disconnected
		if (WiFi.status() != WL_CONNECTED) {
			log("loop", "Wifi waiting for connection");

			// and if last connection try duration is superior to WIFI_TIMEOUT, try to reconnect
			unsigned long currentTime = getTime();
			if ((currentTime == 0) || (currentTime - wifiConnectTimestamp >= WIFI_TIMEOUT)) {
				log("loop", "Connection to WiFi");
				log("loop", "currentTime = " + String(currentTime));
				WiFi.disconnect();
				WiFi.reconnect();
				wifiConnectTimestamp = getTime();

				if (currentTime == 0) {
					while((WiFi.status() != WL_CONNECTED)) {
						log("loop", "Wifi waiting for connection");
						delay(1000);
					}
				}
			}
			return;
		}

		// If tcp is disconnected, connect it
		if (!tcpClient.connected()) {
			log("loop", "Connection to TCP server");
			tcpClient.connect(SERVER_IP, SERVER_PORT);
		}

		previousMeasureTime = currentMillis;
	}

}


void measureQueueTimerSetup()
{
	esp_timer_create_args_t tcpIn_timer_conf = {
		.callback = &measureQueueCallback,
	};
	esp_timer_handle_t tcpIn_timer;
	esp_timer_create(&tcpIn_timer_conf, &tcpIn_timer);
	esp_timer_start_periodic(tcpIn_timer, TCP_RECEIPT_PERIOD * 1000);
}


void grabMeasureSetup()
{
	// Digital input configuration
	gpio_pad_select_gpio(FLOW_PIN);
	gpio_set_direction(FLOW_PIN, GPIO_MODE_INPUT);

	// Configuration of hardware counter
	esp_timer_init();
	esp_timer_create_args_t timer_conf = {
		.callback = &grabMeasureCallback,
	};

	esp_timer_handle_t measureTimer;
	esp_timer_create(&timer_conf, &measureTimer);
	esp_timer_start_periodic(measureTimer, MEASURE_PERIOD * 1e6);
	attachInterrupt(digitalPinToInterrupt(FLOW_PIN), counterCallback, RISING);
}


void IRAM_ATTR counterCallback()
{
	flowMutex.lock();
	transitionCount++;
	flowMutex.unlock();
};


void grabMeasureCallback(void *arg)
{
	// Creation of the measure and store it in
	Measure measure;
	measure.timestamp = getTime();
	if (measure.timestamp == 0) {
		return;
	}
	measure.temperature = analogRead(TEMPERATURE_PIN);
	flowMutex.lock();
	measure.flow = transitionCount;
	transitionCount = 0;
	flowMutex.unlock();

	// If the free memory is lower than MIN_FREE_MEMORY, queue is considered as full, then remove the first measure (the oldest)

	uint32_t freeMemorySize = ESP.getFreeHeap();
	log("grabMeasureCallback", "FreeMemorySize: " + String(freeMemorySize));

	measuresListMutex.lock();
	if (freeMemorySize < (MIN_FREE_MEMORY * 1024)) {
		log("grabMeasureCallback", "Remove measure from FIFO: " + getMeasureStr(measuresQueue.back()));
		measuresQueue.pop_front();
	}
	// Add the new measure at the end of the queue
	measuresQueue.emplace_back(measure);

	log("grabMeasureCallback", "Add measure to FIFO: " + getMeasureStr(measure));
	log("grabMeasureCallback", "FIFO size = " + String(measuresQueue.size()) + "measures / " + String(getListMemorySize()) + "Bytes");
	log("grabMeasureCallback", "Free memory size = " + String(int(ESP.getFreeHeap() / 1024)) + "kB");

	measuresListMutex.unlock();
}


bool sendMeasure(Measure *measure)
{
	// Check connection
	if (WiFi.status() != WL_CONNECTED || !tcpClient.connected()) {
		return false;
	}

	// Serialize data structure
	uint8_t buffer[sizeof(Measure)];
	memcpy(buffer, measure, sizeof(Measure));
	tcpClient.write(buffer, sizeof(Measure));
	log("sendMeasure", "New Measure added to queue: " + getMeasureStr(*measure));

	return true;
}


void ntpSetup()
{
	if (NTP_LOCAL_TIMESTAMP == 0)
	{
		configTime(0, 0, NTP_SERVER);
	}
	else
	{
		configTzTime(NTP_UPDATE_TIMEZONE, NTP_SERVER);
	}

	esp_timer_create_args_t ntp_timer_conf = {
		.callback = &ntpTimerCallback,
	};
	esp_timer_handle_t ntp_timer;
	esp_timer_create(&ntp_timer_conf, &ntp_timer);
	esp_timer_start_periodic(ntp_timer, NTP_UPDATE_PERIOD * 3600 * 1e6);  // From hours to seconds to milliseconds
}


void ntpTimerCallback(void *arg)
{
	configTzTime(NTP_UPDATE_TIMEZONE, NTP_SERVER);
	log("ntpTimerCallback", "NTP update");
}


unsigned long getTime()
{
	time_t now;
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		log("getTime" ,"Failed to obtain time");
		return (0);
	}
	time(&now);
	return now;
}

#ifdef DEBUG
String getLocalTimeStr()
{
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		//log("printLocalTime" ,"Failed to obtain time");
		return "0";
	}

	char formattedTime[16];
	strftime(formattedTime, sizeof(formattedTime), "%H:%M:%S", &timeinfo);
	return String(formattedTime);
}


String getMeasureStr(const Measure &measure)
{
	return (String(measure.timestamp) + " -> " + String(measure.temperature) + " / " + String(measure.flow));
}


void log(String function, String message)
{
	String logMsg = getLocalTimeStr() + " - " + function + " - " + message;

	logMutex.lock();
	logList.emplace_front(logMsg);
	logMutex.unlock();
}


size_t getListMemorySize()
{
	size_t totalSize = 0;

	for (std::list<Measure>::iterator it = measuresQueue.begin(); it != measuresQueue.end(); ++it) {
        Measure* ptr = &(*it);
		totalSize += sizeof(*ptr);
    }

	return totalSize;
}


#else
void log(String function, String message) {return;}
String getMeasureStr(const Measure &measure) {return ("");}
size_t getListMemorySize() {return 0;}

#endif
