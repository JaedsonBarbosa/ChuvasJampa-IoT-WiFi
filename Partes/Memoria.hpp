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
        idEstacao = "R5JT2uKTUwYe1IacFsRP";//new char[24];
        ssidWiFi = "Jaedson Privado";//new char[32];
        senhaWiFi = "C#C++uwp20";//new char[64];
        // preferences.getString("idEstacao", idEstacao, 24);
        // preferences.getString("ssidWiFi", ssidWiFi, 32);
        // preferences.getString("senhaWiFi", senhaWiFi, 64);
        latRegistrada = -6889;//preferences.getInt("latRegistrada");
        lonRegistrada = -35506;//preferences.getInt("lonRegistrada");
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