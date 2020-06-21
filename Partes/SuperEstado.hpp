#pragma once
#include <Preferences.h>

class SuperEstado {
    public:
    virtual void Iniciar();
    virtual void Encerrar();
    virtual void Executar();
};

class Dados {
    Preferences preferences;
    const char * CONFIGURACOES = "configs";

    public:
    bool ativa;
    char * idEstacao;
    char * ssidWiFi;
    char * senhaWiFi;
    time_t ultimaAtt;

    Dados() {
        preferences.begin(CONFIGURACOES, true);
        ativa = preferences.getBool("ativa");
        if (ativa) {
            Serial.println("Estacao ativa.");
            ultimaAtt = preferences.getLong64("ultimaAtt");
            //Comprimentos máximos determinados pelo IEEE
            idEstacao = new char[24];
            ssidWiFi = new char[32];
            senhaWiFi = new char[64];
            preferences.getString("idEstacao", idEstacao, 24);
            preferences.getString("ssidWiFi", ssidWiFi, 32);
            preferences.getString("senhaWiFi", senhaWiFi, 64);
        } else {
            Serial.println("Estacao desativada ou nao configurada.");
        }
        preferences.end();
    }

    void Salvar() {
        preferences.begin(CONFIGURACOES, false);
        preferences.putBool("ativa", ativa);
        preferences.putString("idEstacao", idEstacao);
        preferences.putString("senhaWiFi", senhaWiFi);
        preferences.putString("ssidWiFi", ssidWiFi);
        preferences.putLong64("ultimaAtt", ultimaAtt);
        preferences.end();
        Serial.println("Configuracoes salvas.");
    }
};