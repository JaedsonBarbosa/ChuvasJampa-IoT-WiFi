#include "Partes/Configuracao.hpp"
#include "Partes/Pluviometro.hpp"
#include "Partes/Rede.hpp"
#include "Partes/GPS.hpp"

void setup()
{
    Serial.begin(115200);
    Memoria::Iniciar();
    GPS::Iniciar(14,27);
    Registros::Iniciar();
    Configuracao::Iniciar(13);
    Pluviometro::Iniciar(23);
    Rede::Iniciar(12);
}

void loop() { vTaskDelete(NULL); }