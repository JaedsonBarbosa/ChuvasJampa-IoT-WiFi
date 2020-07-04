#include "Memoria.hpp"
#include "Relogio.hpp"
#include "Registros.hpp"
#include <WiFi.h>

namespace Rede
{
    int ledWiFi;

    void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        digitalWrite(ledWiFi, HIGH);
        if (!Relogio::configuradoNTP) Relogio::ConfigurarNTP();
        if (Relogio::configuradoNTP && Registros::quantidade > 0) {
            Registros::Registrar(NULL);
        }
    }

    void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
    {
        digitalWrite(ledWiFi, LOW);
    }

    void ConectarRedeCadastrada() {
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