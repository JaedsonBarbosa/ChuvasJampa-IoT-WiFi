#pragma once
#include "Memoria.hpp"
#include "Relogio.hpp"
#include "Registros.hpp"
#include "Status.hpp"
#include <WiFi.h>

// Gerenciamos a conexão com a internet via rede WiFI
namespace Rede
{
    // O WiFi foi conectado
    void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        Status::wifiConectado = true;
        if (!Status::relogioConfiguradoNTP) Relogio::ConfigurarViaNTP();
        if ((Status::relogioConfiguradoNTP || Status::relogioConfiguradoGPS) && Registros::quantidade > 0) {
            Registros::Registrar(NULL);
        }
    }

    // O WiFi foi desconectado
    void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        Status::wifiConectado = false;
    }

    // Conectamos à rede cadastrada na memória
    void ConectarRedeCadastrada() {
        Status::wifiConectado = false;
        if (strlen(Memoria::senhaWiFi) >= 8) {
            WiFi.begin(Memoria::ssidWiFi, Memoria::senhaWiFi);
        }
    }

    // Registramos eventos e tentamos nos conectar à rede
    void Iniciar() {
        WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_GOT_IP);
	    WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
        ConectarRedeCadastrada();
    }
}