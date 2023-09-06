#pragma once
#ifndef CONFIG
#define CONFIG

const unsigned int BAUD_RATE = 115200;

const char* WIFI_SSID = "Livebox-Florelie";
const char* WIFI_PASSWORD = "r24hpkr2";

const char* NTP_SERVER = "pool.ntp.org";

IPAddress SERVER_IP(192, 168, 1, 21);
const unsigned int  SERVER_PORT = 1150;

#endif