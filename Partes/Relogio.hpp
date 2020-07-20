#pragma once
#include "Pluviometro.hpp"
#include "Status.hpp"

// Gerenciamos o relógio da placa, assim sabemos que momento ocorreu os pulsos
namespace Relogio
{
    void ConfigurarViaNTP() {
        if (Status::relogioConfiguradoNTP) return;
        try
        {
            const char* ntpServer = "pool.ntp.org";
            const long  gmtOffset_sec = -3 * 3600;
            const int   daylightOffset_sec = 0;
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
            Status::relogioConfiguradoGPS = false;
            Status::relogioConfiguradoNTP = true;
            Pluviometro::Habilitar();
        }
        catch(const std::exception& e) {}
    }

    void ConfigurarViaGPS(time_t horario) {
        if (Status::relogioConfiguradoGPS || Status::relogioConfiguradoNTP) return;
        timeval epoch = {horario, 0};
        const timeval *tv = &epoch;
        timezone utc = {0,0};
        const timezone *tz = &utc;
        settimeofday(tv, tz);
        Status::relogioConfiguradoGPS = true;
        Pluviometro::Habilitar();
    }
}