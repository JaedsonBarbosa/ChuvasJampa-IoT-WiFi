#pragma once
#include "Memoria.hpp"
#include "Registros.hpp"

// Gerenciamos a comunicação com o pluviômetro
namespace Pluviometro
{
    int pinoPluv;
    bool isHabilitado = false;
    unsigned long ultimoClick = 0;

    // Registramos o pulso detectado
    void registrar(void *arg) {
        Registros::Registrar(time(0));
        vTaskDelete(NULL);
    }
    
    // Criamos a tarefa que irá registrar o pulso detectado
    void pulsoDetectado() {
        auto atual = millis();
        if (atual - ultimoClick > 1000) {
            xTaskCreatePinnedToCore(registrar, "Registrar", 10000, NULL, 1, NULL, 0);
            ultimoClick = atual;
        }
    }

    // Configuramos o pino correto
    void Iniciar(int _pinoPluv) {
        pinoPluv = _pinoPluv;
        pinMode(pinoPluv, INPUT_PULLDOWN);
    }

    // Habilitamos a leitura do pino
    void Habilitar() {
        if (isHabilitado) return;
        attachInterrupt(pinoPluv, pulsoDetectado, RISING);
        isHabilitado = true;
    }
}