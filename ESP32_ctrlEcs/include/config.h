#pragma once
#ifndef CONFIG
#define CONFIG

#define BAUD_RATE           115200

#define WIFI_SSID           "Livebox-Florelie"
#define WIFI_PASSWORD       "r24hpkr2"

#define NTP_SERVER          "pool.ntp.org"
#define NTP_UPDATE_TIMEZONE "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"
#define NTP_LOCAL_TIMESTAMP 0                   // 0 for GMT, 1 foor local
#define NTP_UPDATE_PERIOD   10                  // hours

#define SERVER_PORT         1150
IPAddress SERVER_IP(192, 168, 1, 21);

#define FLOW_PIN            GPIO_NUM_4
#define TIMER_GROUP         TIMER_GROUP_0
#define TIMER_IDX           TIMER_0

#define TEMPERATURE_PIN     32
#define TENSION_SCALE       10e4                // multiplicator to store the tension value in an unsigned short int (from 0 to 65535)

#define MEASURE_PERIOD      5                   // seconds
#define FLOW_TIMER_PERIOD   1                   // seconds
#define TCP_RECEIPT_PERIOD  100                 // milliseconds

#define MAX_RX_TRIES        10

#define MAX_FIFO_SIZE       2048

#define TCP_RX_BUFFER_SIZE  1024


#endif