#pragma once
#ifndef CONFIG
#define CONFIG


//#define DEBUG

#define BAUD_RATE           115200

#define WIFI_SSID           "Livebox-Florelie"
#define WIFI_PASSWORD       "r24hpkr2"

#define NTP_SERVER          "pool.ntp.org"
#define NTP_UPDATE_TIMEZONE "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"
#define NTP_LOCAL_TIMESTAMP 0                   // 0 for GMT, 1 for local
#define NTP_UPDATE_PERIOD   24                  // hours

#define SERVER_PORT         1150
IPAddress SERVER_IP(192, 168, 1, 21);

#define FLOW_PIN            GPIO_NUM_21

#define TEMPERATURE_PIN     GPIO_NUM_32

#define NETWORK_PERIOD      100                 // milliseconds
#define MEASURE_PERIOD      5                   // seconds
#define TCP_RECEIPT_PERIOD  100                 // milliseconds

#define RESPONSE_TIMEOUT    6                   // seconds
#define WIFI_TIMEOUT        5                   // seconds

#define MIN_FREE_MEMORY     50                  // kB


#endif