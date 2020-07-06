#include "Partes/Configuracao.hpp"
#include "Partes/Pluviometro.hpp"
#include "Partes/Rede.hpp"
#include "Partes/GPS.hpp"

void setup()
{
    int pinosStatus[] = {13,12,14,27,26,25,33};
    Serial.begin(115200);
    Memoria::Iniciar();
    Registros::Iniciar(pinosStatus[0]);
    Configuracao::Iniciar(pinosStatus[1]);
    Pluviometro::Iniciar(23);
    Relogio::Iniciar(pinosStatus[5], pinosStatus[6]);
    Rede::Iniciar(pinosStatus[2]);
    GPS::Iniciar(pinosStatus[3], pinosStatus[4]);
}

void loop()
{
    if (Configuracao::isRequisicaoRecebida) {
        Configuracao::ProcessarRequisicao();
        Configuracao::isRequisicaoRecebida = false;
    }
}