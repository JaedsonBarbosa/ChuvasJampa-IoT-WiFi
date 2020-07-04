#include "Partes/Configuracao.hpp"
#include "Partes/Pluviometro.hpp"
#include "Partes/Rede.hpp"
#include "Partes/GPS.hpp"

void setup()
{
    Serial.begin(115200);
    Memoria::Iniciar();
    Registros::Iniciar(26);
    Configuracao::Iniciar(13);
    Pluviometro::Iniciar(23);
    Rede::Iniciar(12);
    GPS::Iniciar(14,27);
}

void loop() { vTaskDelete(NULL); }