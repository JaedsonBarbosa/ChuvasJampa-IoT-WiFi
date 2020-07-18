#pragma once
#include "GPS.hpp"
#include "Memoria.hpp"
#include "Rede.hpp"
#include "Status.hpp"
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

    int ProcessarRequisicao() {
        auto req = *Requisicao;
        deserializeJson(req, requisicao);
        auto res = *RetornoInfos;
        const char* metodo = req["metodo"];
        if (!strcmp(metodo, "GetDados")) {
            res["idEstacao"] = Memoria::idEstacao;
            res["isConectado"] = WiFi.status() == WL_CONNECTED;
            res["ssidWiFi"] = Memoria::ssidWiFi;
            res["senhaWiFi"] = Memoria::senhaWiFi;
            JsonArray redesDisponiveis = res.createNestedArray("redesDisponiveis");
            int quant = WiFi.scanNetworks();
            for (int i = 0; i < quant; i++)
                redesDisponiveis.add(WiFi.SSID(i));
        } else if (!strcmp(metodo, "SetDados")) {
            auto novaSenhaWifi = strdup(req["senhaWiFi"]);
            auto novoSSIDWiFI = strdup(req["ssidWiFi"]);
            auto attWifi = !strcmp(Memoria::senhaWiFi, novaSenhaWifi) || !strcmp(Memoria::ssidWiFi, novoSSIDWiFI);
            Memoria::idEstacao = strdup(req["idEstacao"]);
            Memoria::senhaWiFi = novaSenhaWifi;
            Memoria::ssidWiFi = novoSSIDWiFI;
            Memoria::Salvar();
            if (attWifi) Rede::ConectarRedeCadastrada();
            res["metodo"] = "SetDados";
            res["success"] = true;
        } else if (!strcmp(metodo, "GetGPS")) {
            auto local = GPS::gps.location;
            res["metodo"] = "GetGPS";
            res["valid"] = local.isValid();
            res["lat"] = local.lat();
            res["lon"] = local.lng();
        } else if (!strcmp(metodo, "GetStatus")) {
            res["metodo"] = "GetStatus";
            res["nuvemConectada"] = Status::nuvemConectada;
            res["bluetoothAtivado"] = Status::bluetoothAtivado;
            res["relogioConfiguradoGPS"] = Status::relogioConfiguradoGPS;
            res["relogioConfiguradoNTP"] = Status::relogioConfiguradoNTP;
            res["wifiConectado"] = Status::wifiConectado;
            res["gpsConectado"] = Status::gpsConectado;
        } else return 1;
        char envioC[512];
        serializeJson(res, envioC);
        SerialBT.println(envioC);
        return 0;
    }

    void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
    {
        if (event == ESP_SPP_DATA_IND_EVT) {
            requisicao = SerialBT.readString();
            Serial.println(requisicao);
            isRequisicaoRecebida = true;
        }
    }

    void Iniciar() {
        Requisicao = new DynamicJsonDocument(512);
        RetornoInfos = new DynamicJsonDocument(512);
        SerialBT.register_callback(callback);
        Status::bluetoothAtivado = SerialBT.begin("Estacao");
    }
}