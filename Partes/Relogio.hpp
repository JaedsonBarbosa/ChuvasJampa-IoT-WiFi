#pragma once
#include "Pluviometro.hpp"

namespace Relogio
{
    int ledRelogioGPS;
    int ledRelogioNTP;
    bool configuradoViaGPS = false;
    bool configuradoViaNTP = false;

    void Iniciar(int _ledRelogioGPS, int _ledRelogioNTP) {
        ledRelogioGPS = _ledRelogioGPS;
        ledRelogioNTP = _ledRelogioNTP;
        pinMode(ledRelogioGPS, OUTPUT);
        pinMode(ledRelogioNTP, OUTPUT);
        digitalWrite(ledRelogioGPS, LOW);
        digitalWrite(ledRelogioNTP, LOW);
    }

    void ConfigurarViaNTP() {
        if (configuradoViaNTP) return;
        try
        {
            const char* ntpServer = "pool.ntp.org";
            const long  gmtOffset_sec = -3 * 3600;
            const int   daylightOffset_sec = 0;
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
            configuradoViaNTP = true;
            Pluviometro::Habilitar();
            digitalWrite(ledRelogioGPS, LOW);
            digitalWrite(ledRelogioNTP, HIGH);
        }
        catch(const std::exception& e) {}
    }

    void ConfigurarViaGPS(time_t horario) {
        if (configuradoViaGPS || configuradoViaNTP) return;
        timeval epoch = {horario, 0};
        const timeval *tv = &epoch;
        timezone utc = {0,0};
        const timezone *tz = &utc;
        settimeofday(tv, tz);
        configuradoViaGPS = true;
        Pluviometro::Habilitar();
        digitalWrite(ledRelogioGPS, HIGH);
    }
}