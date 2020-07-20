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

// Aqui gerenciamos as configurações da estação
namespace Configuracao {
    BluetoothSerial SerialBT;
    DynamicJsonDocument * Requisicao;
    DynamicJsonDocument * RetornoInfos;
    bool isRequisicaoRecebida = false;
    String requisicao;

    // Processamos as requisições
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
            res["possuiGPS"] = POSSUI_GPS;
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
        }
        #if POSSUI_GPS
        else if (!strcmp(metodo, "GetGPS")) {
            auto local = GPS::gps.location;
            auto lat = local.lat();
            auto lng = local.lng();;
            res["metodo"] = "GetGPS";
            res["valid"] = local.isValid() && lat != 0 && lng != 0;
            res["lat"] = lat;
            res["lon"] = lng;
        }
        #endif
        else if (!strcmp(metodo, "GetStatus")) {
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

    // Se o evento é de recebimento de dados precisamos mandar uma resposta
    void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
    {
        if (event == ESP_SPP_DATA_IND_EVT) {
            requisicao = SerialBT.readString();
            isRequisicaoRecebida = true;
        }
    }

    // Liberamos o recebimento de conexões Bluetooth
    void Iniciar() {
        Requisicao = new DynamicJsonDocument(512);
        RetornoInfos = new DynamicJsonDocument(512);
        SerialBT.register_callback(callback);
        Status::bluetoothAtivado = SerialBT.begin("Estacao");
    }
}