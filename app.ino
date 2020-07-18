#include "Partes/Configuracao.hpp"
#include "Partes/Pluviometro.hpp"
#include "Partes/Rede.hpp"
#include "Partes/GPS.hpp"

void setup()
{
    //int pinosStatus[] = {13,12,14,27,26,25,33};
    Serial.begin(115200);
    Memoria::Iniciar();
    Registros::Iniciar();
    Configuracao::Iniciar();
    Pluviometro::Iniciar(23);
    Rede::Iniciar();
    GPS::Iniciar();
}

void loop()
{
    if (Configuracao::isRequisicaoRecebida) {
        auto result = Configuracao::ProcessarRequisicao();
        Serial.println(result);
        Configuracao::isRequisicaoRecebida = false;
    }
}