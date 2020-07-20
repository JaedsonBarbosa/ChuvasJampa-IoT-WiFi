#include "Partes/Configuracao.hpp"
#include "Partes/Pluviometro.hpp"
#include "Partes/Rede.hpp"
#include "Partes/GPS.hpp"
#define PINO_PLUVIOMETRO 23

void setup()
{
    Memoria::Iniciar();
    Registros::Iniciar();
    Configuracao::Iniciar();
    Pluviometro::Iniciar(PINO_PLUVIOMETRO);
    Rede::Iniciar();
    #if POSSUI_GPS
    GPS::Iniciar();
    #endif
}

void loop()
{
    if (Configuracao::isRequisicaoRecebida) {
        auto result = Configuracao::ProcessarRequisicao();
        Serial.println(result);
        Configuracao::isRequisicaoRecebida = false;
    }
}