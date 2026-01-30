#include "TimeHandler.h"

RTC_DS3231 rtc;
static const time_t MIN_VALID_EPOCH = 1767225600;   // 01-01-2026 00:00:00 UTC
static bool initialized = false;
    
bool rtcInitOnce() {
    if (initialized) {
        return true;
    }

    bool initOk = rtc.begin();
    
    if(initOk){
        initialized = true;
    }
    
    return initOk;
}

/**
 * @brief Compara si dos epoch UTC pertenecen al mismo día, teniendo en cuenta el horario del país
 * 
 * @param time_t variable A
 * @param time_t variable B
 * @return true si pertenecen al mismo día, false en caso contrario
 */
bool mismoDia(time_t a, time_t b){
    const time_t aCountryTime = a + LOCAL_TIME_UTC_HOUR * 3600;
    const time_t bCountryTime = b + LOCAL_TIME_UTC_HOUR * 3600;

    struct tm tA;
    struct tm tB;

    gmtime_r(&aCountryTime, &tA);
    gmtime_r(&bCountryTime, &tB);

    return (tA.tm_year == tB.tm_year && tA.tm_yday == tB.tm_yday);
}

/**
 * @brief Obtiene la fecha del DS3231 en formato epoch UTC
 * 
 * @param time_t * variable donde se almacena el resultado
 * @return true si se realizó con éxito, false en caso contrario
 */
bool obtenerFechaHoyUTC(time_t * out){
    if(!rtcInitOnce())
        return false;

    if (rtc.lostPower() && !ajustarFechaViaWiFi())
        return false;

    DateTime now = rtc.now();
    *out = now.unixtime();

    return true;
}

/**
 * @brief Ajusta la fecha del DS3231 en formato UTC via WiFi (dos servidores NTP)
 * 
 * @param void
 * @return true si se realizó con éxito, false en caso contrario
 */
bool ajustarFechaViaWiFi(){
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    const uint32_t wifiTimeoutMs = 10000;
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - t0) < wifiTimeoutMs) {
        delay(200);
    }

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        return false;
    }

    configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);  // 0 de offset (UTC) y sin offset de verano

    uint32_t t1 = millis();
    time_t nowUtc = 0;
    const uint32_t ntpTimeoutMs = 20000;

    while ((millis() - t1) < ntpTimeoutMs) {
        nowUtc = time(nullptr);
        if (nowUtc >= MIN_VALID_EPOCH) {    // se consiera hora valida
            break;
        }
        delay(250);
    }

    if (nowUtc < MIN_VALID_EPOCH) {
        // NTP no respondió / no sincronizó a tiempo
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        return false;
    }

    if (!rtcInitOnce()) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        return false;
    }

    nowUtc = time(nullptr);
    rtc.adjust(DateTime((uint32_t)nowUtc));

    DateTime chk = rtc.now();
    time_t chkUtc = (time_t)chk.unixtime();
    if (chkUtc + 2 < nowUtc) { // tolerancia de 2s por latencias
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        return false;
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return true;

}

/**
 * @brief Ajusta la fecha del DS3231 en formato UTC
 * 
 * @param time_t fecha y hora actual expresada en epoch UTC
 * @return true si se realizó con éxito, false en caso contrario
 */
bool ajustarFechaManual(time_t now){
    if(!rtcInitOnce())
        return false;

    rtc.adjust(DateTime((uint32_t) now));

    DateTime updateNow_ = rtc.now();
    time_t updateNow = updateNow_.unixtime();

    if(updateNow + 2 < now) // + 2 segundos por tolerancia
        return false;

    return true;
}