#pragma once
#include "Memoria.hpp"
#include "Registros.hpp"

namespace Pluviometro
{
    int pinoPluv;
    bool isHabilitado = false;
    unsigned long ultimoClick = 0;

    void registrar(void *arg) {
        Registros::Registrar(time(0));
        vTaskDelete(NULL);
    }
    
    void pulsoDetectado() {
        auto atual = millis();
        if (atual - ultimoClick > 1000) {
            xTaskCreatePinnedToCore(registrar, "Registrar", 10000, NULL, 1, NULL, 0);
            ultimoClick = atual;
        }
    }

    void Iniciar(int _pinoPluv) {
        pinoPluv = _pinoPluv;
        pinMode(pinoPluv, INPUT_PULLDOWN);
    }

    void Habilitar() {
        if (isHabilitado) return;
        attachInterrupt(pinoPluv, pulsoDetectado, RISING);
        isHabilitado = true;
    }
}