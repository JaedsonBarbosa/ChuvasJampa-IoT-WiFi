//#include <sstream>
#include <WiFi.h>
#include "Partes/Configuracao.hpp"
#include "Partes/Normal.hpp"

const int PINO_CONFIG = 4;

Configuracao::Gerenciador * EstadoConfig;
Normal::Gerenciador * EstadoNormal;

typedef enum {
    Inicio,
    ModoConfiguracao,
    ModoNormal,
} Estados;
Estados estadoAtual;

void setup()
{
    estadoAtual = Inicio;
    pinMode(PINO_CONFIG, INPUT_PULLDOWN);
    Serial.begin(115200);
    delay(1000);
    auto configs = new Dados();
    EstadoConfig = new Configuracao::Gerenciador(configs);
    EstadoNormal = new Normal::Gerenciador(configs);
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado.");
        if (configs->ativa) {
            WiFi.setAutoReconnect(true);
            WiFi.begin(configs->ssidWiFi, configs->senhaWiFi);
            while (WiFi.status() != WL_CONNECTED) {
                delay(1000);
                Serial.println("Conectando...");
            }
            Serial.println("Conectado");
        }
    }
}

void loop() {
    bool configEnabled = digitalRead(PINO_CONFIG);
    switch (estadoAtual)
    {
    case Estados::Inicio:
        if (configEnabled) {
            estadoAtual = Estados::ModoConfiguracao;
            EstadoConfig->Iniciar();
        } else {
            estadoAtual = Estados::ModoNormal;
            EstadoNormal->Iniciar();
        }
        break;
    case Estados::ModoConfiguracao:
        if (configEnabled) {
            EstadoConfig->Executar();
        } else {
            EstadoConfig->Encerrar();
            EstadoNormal->Iniciar();
            estadoAtual = Estados::ModoNormal;
        }
        break;
    case Estados::ModoNormal:
        if (configEnabled) {
            EstadoNormal->Encerrar();
            EstadoConfig->Iniciar();
            estadoAtual = Estados::ModoConfiguracao;
        } else {
            EstadoNormal->Executar();
        }
        break;
    }
}
