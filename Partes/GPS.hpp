#pragma once
#include "Memoria.hpp"
#include "Relogio.hpp"
#include <TinyGPS++.h>

namespace GPS {
    HardwareSerial SerialGPS(2);
    TinyGPSPlus gps;

    time_t GPSTime() {
        time_t t_of_day;
        struct tm t;
        auto gps = GPS::gps;
        t.tm_year = gps.date.year()-1900;
        t.tm_mon = gps.date.month()-1;           // Month, 0 - jan
        t.tm_mday = gps.date.day();          // Day of the month
        t.tm_hour = gps.time.hour();
        t.tm_min =  gps.time.minute();
        t.tm_sec = gps.time.second();
        t_of_day = mktime(&t);
        return t_of_day;
    }

    void serialEvent() {
        bool localEncontrado = false;
        while (SerialGPS.available())
            if (gps.encode(SerialGPS.read()))
                localEncontrado = true;
        if (localEncontrado) {
            Status::gpsConectado = true;
            Relogio::ConfigurarViaGPS(GPSTime());
        }
    }

    void Iniciar() {
        SerialGPS.begin(9600, SERIAL_8N1, 16, 17);
	    SerialGPS.setInterrupt(&serialEvent);
    }
}