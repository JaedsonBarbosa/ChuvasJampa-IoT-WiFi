#pragma once
#include "Pluviometro.hpp"

namespace Relogio
{
    int ledRelogio;
    bool configuradoViaGPS = false;
    bool configuradoViaNTP = false;

    void Iniciar(int _ledRelogio) {
        ledRelogio = _ledRelogio;
        pinMode(ledRelogio, OUTPUT);
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
    }
}