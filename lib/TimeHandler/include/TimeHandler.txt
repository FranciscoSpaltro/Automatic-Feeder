#ifndef TIME_HANDLER_H
#define TIME_HANDLER_H

#include <time.h>
#include <WiFi.h>
#include "RTClib.h"

static const char* WIFI_SSID     = "RETECA-1";
static const char* WIFI_PASS     = "R3T3C4-SuPSi";
static const char* NTP_SERVER_1  = "pool.ntp.org";
static const char* NTP_SERVER_2  = "time.google.com";

// PRECONDICION: La base de datos almacena fechas en UTC
#define LOCAL_TIME_UTC_HOUR -3

bool obtenerFechaHoyUTC(time_t * out);
bool mismoDia(time_t a, time_t b);

#endif  // TIME_HANDLER_H