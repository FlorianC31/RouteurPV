#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "config.h"
#include "utils.h"

NTPClient* timeClient = nullptr;
WiFiClient tcpClient;

void setup()
{
	Serial.begin(BAUD_RATE);

	// Connexion au réseau WiFi
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(1000);
		Serial.println("Connexion au WiFi...");
	}
	Serial.println("Connecté au WiFi");

	// Connexion au serveur
	if (tcpClient.connect(SERVER_IP, SERVER_PORT))
	{
		Serial.println("Connexion au serveur établie");
	}
	else
	{
		Serial.println("Échec de la connexion au serveur");
	}

	// Init of NTPClient
	WiFiUDP ntpUDP;
	timeClient = new NTPClient(ntpUDP, NTP_SERVER, 0, 60000);
	timeClient->begin();
	timeClient->update();
}

void loop()
{
	// Mettez à jour l'heure du client NTP toutes les 60 secondes
	// timeClient.update();

	// Obtenez l'heure actuelle de l'ESP32
	//Serial.println(timeClient->getEpochTime());
	Serial.println("[Tx ] " + timeClient->getFormattedTime());
	tcpClient.println(timeClient->getFormattedTime());

	/*while(!tcpClient.available());

	String response = tcpClient.readStringUntil('\n');
	Serial.println("[Tx ] " + response);*/
	

	// Votre code peut continuer ici en utilisant l'heure synchronisée
	// Par exemple, vous pouvez l'utiliser pour l'enregistrement des données ou pour des actions basées sur le temps.

	delay(1000); // Attendez 1 seconde avant la prochaine mise à jour
}
