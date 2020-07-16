#pragma once
#include "GPS.hpp"
#include "Memoria.hpp"
#include "Rede.hpp"
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

namespace Configuracao {
    BluetoothSerial SerialBT;
    DynamicJsonDocument * Requisicao;
    DynamicJsonDocument * RetornoInfos;
    bool isRequisicaoRecebida = false;
    String requisicao;

    void sendResponse(const DynamicJsonDocument &response) {
        char envioC[512];
        serializeJson(response, envioC);
        SerialBT.println(envioC);
        Serial.println(envioC);
    }

    void GetDados() {
        auto res = *RetornoInfos;
        res["metodo"] = "GetDados";
        res["idEstacao"] = Memoria::idEstacao;
        res["isConectado"] = WiFi.status() == WL_CONNECTED;
        res["ssidWiFi"] = Memoria::ssidWiFi;
        res["senhaWiFi"] = Memoria::senhaWiFi;
        JsonArray redesDisponiveis = res.createNestedArray("redesDisponiveis");
        int quant = WiFi.scanNetworks();
        for (int i = 0; i < quant; i++)
            redesDisponiveis.add(WiFi.SSID(i));
        sendResponse(res);
    }

    void SetDados(DynamicJsonDocument* req) {
        auto novaSenhaWifi = strdup(req->getMember("senhaWiFi"));
        auto novoSSIDWiFI = strdup(req->getMember("ssidWiFi"));
        auto attWifi = !strcmp(Memoria::senhaWiFi, novaSenhaWifi) || !strcmp(Memoria::ssidWiFi, novoSSIDWiFI);
        Memoria::idEstacao = strdup(req->getMember("idEstacao"));
        Memoria::senhaWiFi = novaSenhaWifi;
        Memoria::ssidWiFi = novoSSIDWiFI;
        Memoria::Salvar();
        if (attWifi) Rede::ConectarRedeCadastrada();
        auto res = *RetornoInfos;
        res["metodo"] = "SetDados";
        res["success"] = true;
        sendResponse(res);
    }

    void GetGPS() {
        auto local = GPS::gps.location;
        auto res = *RetornoInfos;
        res["metodo"] = "GetGPS";
        res["valid"] = local.isValid();
        res["lat"] = local.lat();
        res["lon"] = local.lng();
        sendResponse(res);
    }

    void GetStatus() {
        auto res = *RetornoInfos;
        
        sendResponse(res);
    }

    void ProcessarRequisicao() {
        auto req = *Requisicao;
        deserializeJson(req, requisicao);
        const char* metodo = req["metodo"];
        if (!strcmp(metodo, "GetDados")) {
            GetDados();
        } else if (!strcmp(metodo, "SetDados")) {
            SetDados(&req);
        } else if (!strcmp(metodo, "GetGPS")) {
            GetGPS();
        }
    }

    void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
    {
        if (event == ESP_SPP_DATA_IND_EVT) {
            requisicao = SerialBT.readString();
            Serial.println(requisicao);
            isRequisicaoRecebida = true;
        }
    }

    void Iniciar(int ledBluetooth) {
        Requisicao = new DynamicJsonDocument(512);
        RetornoInfos = new DynamicJsonDocument(512);
        SerialBT.register_callback(callback);
        pinMode(ledBluetooth, OUTPUT);
        if (SerialBT.begin("Estação")) {
            digitalWrite(ledBluetooth, HIGH);
        }
    }
}