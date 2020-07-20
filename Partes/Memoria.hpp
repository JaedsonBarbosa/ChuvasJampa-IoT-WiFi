#pragma once
#include <Preferences.h>

// Gerenciamos o armazenamento das configurações da estação
namespace Memoria {
    Preferences preferences;
    const char * nomeTabela = "configs";

    char * idEstacao;
    char * ssidWiFi;
    char * senhaWiFi;

    // Pegamos as variáveis da memória não-volátil e as salvamos na memória volátil
    void Iniciar() {
        preferences.begin(nomeTabela, true);
        //Comprimentos máximos determinados pelo IEEE
        idEstacao = new char[24];
        ssidWiFi = new char[32];
        senhaWiFi = new char[64];
        preferences.getString("idEstacao", idEstacao, 24);
        preferences.getString("ssidWiFi", ssidWiFi, 32);
        preferences.getString("senhaWiFi", senhaWiFi, 64);
        preferences.end();
    }

    // Pegamos as variáveis da memória volátil e as salvamos na memória não-volátil
    void Salvar() {
        preferences.begin(nomeTabela, false);
        preferences.putString("idEstacao", idEstacao);
        preferences.putString("senhaWiFi", senhaWiFi);
        preferences.putString("ssidWiFi", ssidWiFi);
        preferences.end();
    }
};