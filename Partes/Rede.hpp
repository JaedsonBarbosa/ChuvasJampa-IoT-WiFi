#pragma once
#include "Memoria.hpp"
#include "Relogio.hpp"
#include "Registros.hpp"
#include <WiFi.h>

namespace Rede
{
    int ledWiFi;
    bool conectado = false;

    void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        digitalWrite(ledWiFi, HIGH);
        if (!Relogio::configuradoViaNTP) Relogio::ConfigurarViaNTP();
        if ((Relogio::configuradoViaNTP || Relogio::configuradoViaGPS) && Registros::quantidade > 0) {
            Registros::Registrar(NULL);
        }
        conectado = true;
    }

    void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        digitalWrite(ledWiFi, LOW);
        conectado = false;
    }

    void ConectarRedeCadastrada() {
        digitalWrite(ledWiFi, LOW);
        if (strlen(Memoria::senhaWiFi) >= 8) {
            WiFi.begin(Memoria::ssidWiFi, Memoria::senhaWiFi);
        }
    }

    void Iniciar(int _ledWiFi) {
        ledWiFi = _ledWiFi;
        pinMode(ledWiFi, OUTPUT);
        WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_GOT_IP);
	    WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
        ConectarRedeCadastrada();
    }
}