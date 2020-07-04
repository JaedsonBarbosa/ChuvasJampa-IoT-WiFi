#pragma once
#include <Preferences.h>

namespace Memoria {
    Preferences preferences;
    const char * nomeTabela = "configs";

    char * idEstacao;
    char * ssidWiFi;
    char * senhaWiFi;
    int latRegistrada;
    int lonRegistrada;

    void Iniciar() {
        preferences.begin(nomeTabela, true);
        //Comprimentos máximos determinados pelo IEEE
        idEstacao = new char[24];
        ssidWiFi = new char[32];
        senhaWiFi = new char[64];
        preferences.getString("idEstacao", idEstacao, 24);
        preferences.getString("ssidWiFi", ssidWiFi, 32);
        preferences.getString("senhaWiFi", senhaWiFi, 64);
        latRegistrada = preferences.getInt("latRegistrada");
        lonRegistrada = preferences.getInt("lonRegistrada");
        preferences.end();
    }

    void Salvar() {
        preferences.begin(nomeTabela, false);
        preferences.putString("idEstacao", idEstacao);
        preferences.putString("senhaWiFi", senhaWiFi);
        preferences.putString("ssidWiFi", ssidWiFi);
        preferences.putInt("latRegistrada", latRegistrada);
        preferences.putInt("lonRegistrada", lonRegistrada);
        preferences.end();
    }
};