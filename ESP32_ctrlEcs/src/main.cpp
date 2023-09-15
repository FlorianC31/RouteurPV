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

volatile unsigned short int transitionCount = 0;
unsigned short int flow;

std::list<Measure> fifoOut;
std::list<unsigned long int> fifoIn;
std::mutex fifoOutMutex;
std::mutex fifoInMutex;



// Functions prototypes
void networkSetup();
void counterSetup();
void timerCallback(void *arg);
void IRAM_ATTR counterCallback();
void grabMeasure();
bool sendMeasure(Measure *measure);
String getMeasureStr(const Measure &measure);
void ntpSetup();
void printLocalTime();
void ntpTimerCallback(void *arg);
bool tcpConnect();
unsigned long getTime();
void tcpReceiptCallback();
bool isTimestampSentBack(unsigned long int timestamp);

// Functions definition
void setup()
{
	Serial.begin(BAUD_RATE);
	networkSetup();

	counterSetup();
	ntpSetup();

	pinMode(TEMPERATURE_PIN, INPUT);

	Serial.println("sizeof(Measure) = " + String(sizeof(Measure)));

}

void loop()
{
	fifoOutMutex.lock();
	unsigned long int firstTimestamp = fifoOut.back().timestamp;
	fifoOutMutex.unlock();

	// if the FIFO is empty, wait another measure
	if (fifoOut.empty())
	{
		delay(MEASURE_PERIOD * 1000);
		return;
	}

	// Try to send the last measure of the FIFO
	if (!sendMeasure(&fifoOut.back()))
	{
		// If failed wait before retrying
		Serial.println("Connection error");
		delay(MEASURE_PERIOD * 1000);
		return;
	}

	// Wait until the timestamp is sent back by the server
	unsigned int nbTries = 0;
	while (!isTimestampSentBack(firstTimestamp) && nbTries < MAX_RX_TRIES) {		
		delay(TCP_RECEIPT_PERIOD);
		nbTries++;	
	}
	
	// if after 10 tries of 100ms the paquet is still not received, exit the loop to send the first measure again
	if (nbTries == MAX_RX_TRIES) {
		return;
	}

	// else, remove the timestamp from the FIFO in 
	fifoInMutex.lock();
	fifoIn.remove(firstTimestamp);
	fifoInMutex.unlock();

	// and the first measure from the FIFO out
	fifoOutMutex.lock();
	Measure sentMeasure = fifoOut.back();
	fifoOut.pop_back();
	fifoOutMutex.unlock();
	Serial.println("Paquet well sent: " + getMeasureStr(sentMeasure));

}


bool isTimestampSentBack(unsigned long int timestamp)
{
	auto it = std::find(fifoIn.begin(), fifoIn.end(), timestamp);
	return it != fifoIn.end();
}

void networkSetup()
{
	// Connexion au réseau WiFi
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		Serial.println("Connexion au WiFi...");
	}
	Serial.println("Connecté au WiFi");
}

bool tcpConnect()
{
	// Connexion au serveur TCP
	if (tcpClient.connect(SERVER_IP, SERVER_PORT))
	{
		Serial.println("Connexion au serveur établie");

		// Creation of the TCP receipt timer
		esp_timer_create_args_t ntp_timer_conf = {
			.callback = &ntpTimerCallback,
		};
		esp_timer_handle_t ntp_timer;
		esp_timer_create(&ntp_timer_conf, &ntp_timer);
		esp_timer_start_periodic(ntp_timer, TCP_RECEIPT_PERIOD * 1000);

		return true;
	}
	else
	{
		Serial.println("Échec de la connexion au serveur");
		return false;
	}
}

void tcpReceiptCallback()
{
	if (tcpClient.available()) {
		uint32_t receivedTimestamp;
		tcpClient.readBytes((char *)&receivedTimestamp, sizeof(receivedTimestamp));
		
		// If the timestamp is not already in the FIFO add it
		fifoInMutex.lock();
		auto it = std::find(fifoIn.begin(), fifoIn.end(), receivedTimestamp);
		if (it == fifoIn.end()) {
			fifoIn.push_front(receivedTimestamp);
		}
		fifoInMutex.unlock();
	}
}

void counterSetup()
{
	// Digital input configuration
	gpio_pad_select_gpio(FLOW_PIN);
	gpio_set_direction(FLOW_PIN, GPIO_MODE_INPUT);

	// Configuration of harware counter
	esp_timer_init();
	esp_timer_create_args_t timer_conf = {
		.callback = &timerCallback,
	};

	esp_timer_handle_t measureTimer;
	esp_timer_create(&timer_conf, &measureTimer);
	esp_timer_start_periodic(measureTimer, FLOW_TIMER_PERIOD * 1e6);
	attachInterrupt(digitalPinToInterrupt(FLOW_PIN), counterCallback, RISING);
}


void timerCallback(void *arg)
{
	flow = transitionCount;
	transitionCount = 0;
	grabMeasure();	
}


void IRAM_ATTR counterCallback()
{
	transitionCount++;
};

void grabMeasure()
{
	// Creation of the measure and store it in
	Measure measure;
	measure.timestamp = getTime();
	measure.temperature = analogRead(TEMPERATURE_PIN);
	measure.flow = flow;

	fifoOutMutex.lock();
	if (fifoOut.size() >= MAX_FIFO_SIZE)
	{
		Serial.println("Remove measure from FIFO: " + getMeasureStr(fifoOut.back()));
		fifoOut.pop_back();
	}
	fifoOut.emplace_front(measure);
	Serial.println("Add measure to FIFO: " + getMeasureStr(measure));
	Serial.println("FIFO size = " + String(fifoOut.size()));
	fifoOutMutex.unlock();
}

bool sendMeasure(Measure *measure)
{
	// If the client is not yet connected and the connexion try fail
	if (!tcpClient.connected() && !tcpConnect())
	{
		return false;
	}

	// Serialize data structure
	uint8_t buffer[sizeof(Measure)];
	memcpy(buffer, measure, sizeof(Measure));

	tcpClient.write(buffer, sizeof(Measure));

	return true;
}

String getMeasureStr(const Measure &measure)
{
	return (String(measure.timestamp) + " -> " + String(measure.temperature) + " / " + String(measure.flow));
}

void ntpSetup()
{
	unsigned int ntpUpdatePeriod = NTP_UPDATE_PERIOD * 1000; // From seconds to milliseconds

	if (NTP_LOCAL_TIMESTAMP == 0)
	{
		configTime(0, 0, NTP_SERVER);
	}
	else
	{
		configTzTime(NTP_UPDATE_TIMEZONE, NTP_SERVER);
	}
	printLocalTime();

	esp_timer_create_args_t ntp_timer_conf = {
		.callback = &ntpTimerCallback,
	};
	esp_timer_handle_t ntp_timer;
	esp_timer_create(&ntp_timer_conf, &ntp_timer);
	esp_timer_start_periodic(ntp_timer, NTP_UPDATE_PERIOD * 1000 * 1000); // NTP_UPDATE_PERIOD est en secondes
}

void ntpTimerCallback(void *arg)
{
	// Mettez à jour le serveur NTP ici
	configTzTime(NTP_UPDATE_TIMEZONE, NTP_SERVER);
	printLocalTime();
}

void printLocalTime()
{
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return;
	}
	Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

unsigned long getTime()
{
	time_t now;
	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
	{
		Serial.println("Failed to obtain time");
		return (0);
	}
	time(&now);
	return now;
}
