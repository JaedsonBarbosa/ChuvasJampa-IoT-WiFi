#include "Memoria.hpp"
#include <TinyGPS++.h>
#include <time.h>

namespace GPS {
    int ledGPS, ledLocalCorreto;
    HardwareSerial SerialGPS(2);
    TinyGPSPlus gps;
    bool localEncontrado = false;

    time_t GPSTime() {
        time_t t_of_day;
        struct tm t;
        t.tm_year = gps.date.year()-1900;
        t.tm_mon = gps.date.month()-1;           // Month, 0 - jan
        t.tm_mday = gps.date.day();          // Day of the month
        t.tm_hour = gps.time.hour();
        t.tm_min =  gps.time.minute();
        t.tm_sec = gps.time.second();
        t_of_day = mktime(&t);
        return t_of_day;
    }

    void IRAM_ATTR serialEvent() {
        bool isPrimeiroLocal = false;
        while (SerialGPS.available())
            if (gps.encode(SerialGPS.read()) && !localEncontrado)
                isPrimeiroLocal = localEncontrado = true;
        if (isPrimeiroLocal) {
            timeval epoch = {GPSTime(), 0};
            const timeval *tv = &epoch;
            timezone utc = {0,0};
            const timezone *tz = &utc;
            settimeofday(tv, tz);
        }
        int latitude = gps.location.lat() * 1000;
        int longitude = gps.location.lng() * 1000;
        if (latitude == Memoria::latRegistrada && longitude == Memoria::lonRegistrada) {
            digitalWrite(ledLocalCorreto, HIGH);
        } else digitalWrite(ledLocalCorreto, LOW);
    }

    void Iniciar(int _ledGPS, int _ledLocalCorreto) {
        ledGPS = _ledGPS;
        ledLocalCorreto = _ledLocalCorreto;
        pinMode(ledGPS, OUTPUT);
        pinMode(ledLocalCorreto, OUTPUT);
        SerialGPS.begin(9600, SERIAL_8N1, 16, 17);
	    SerialGPS.setInterrupt(&serialEvent);
    }
}