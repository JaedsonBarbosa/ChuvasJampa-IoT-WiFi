#pragma once
#include "Memoria.hpp"
#include "Relogio.hpp"
#include "Registros.hpp"
#include "Status.hpp"
#include <WiFi.h>

namespace Rede
{
    void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        Status::wifiConectado = true;
        if (!Status::relogioConfiguradoNTP) Relogio::ConfigurarViaNTP();
        if ((Status::relogioConfiguradoNTP || Status::relogioConfiguradoGPS) && Registros::quantidade > 0) {
            Registros::Registrar(NULL);
        }
    }

    void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        Status::wifiConectado = false;
    }

    void ConectarRedeCadastrada() {
        Status::wifiConectado = false;
        if (strlen(Memoria::senhaWiFi) >= 8) {
            WiFi.begin(Memoria::ssidWiFi, Memoria::senhaWiFi);
        }
    }

    void Iniciar() {
        WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_GOT_IP);
	    WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
        ConectarRedeCadastrada();
    }
}