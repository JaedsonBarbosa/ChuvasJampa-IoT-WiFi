#include "Partes/Configuracao.hpp"
#include "Partes/Pluviometro.hpp"
#include "Partes/Rede.hpp"
#include "Partes/GPS.hpp"

// Definimos aqui qual o pino que será conectado ao pluviômetro
#define PINO_PLUVIOMETRO 23

// Aqui é onde tudo começa, sendo apenas executado uma vez logo ao ligar
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

// O loop serve para analisar se alguma requisição foi recebida pelo Bluetooth
void loop()
{
    if (Configuracao::isRequisicaoRecebida) {
        auto result = Configuracao::ProcessarRequisicao();
        Serial.println(result);
        Configuracao::isRequisicaoRecebida = false;
    }
}