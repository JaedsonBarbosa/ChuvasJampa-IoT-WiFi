#pragma once

namespace Relogio
{
    bool configuradoGPS = false;
    bool configuradoNTP = false;

    void ConfigurarNTP() {
        try
        {
            const char* ntpServer = "pool.ntp.org";
            const long  gmtOffset_sec = -3 * 3600;
            const int   daylightOffset_sec = 0;
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
            configuradoNTP = true;
        }
        catch(const std::exception& e) {}
    }

    
}