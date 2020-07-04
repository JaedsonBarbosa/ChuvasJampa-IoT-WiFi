#include "Memoria.hpp"
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

    void GetDados() {
        auto res = *RetornoInfos;
        res["idEstacao"] = Memoria::idEstacao;
        res["isConectado"] = WiFi.status() == WL_CONNECTED;
        res["ssidWiFi"] = Memoria::ssidWiFi;
        res["senhaWiFi"] = Memoria::senhaWiFi;
        res["latRegistrada"] = Memoria::latRegistrada;
        res["lonRegistrada"] = Memoria::lonRegistrada;
        JsonArray redesDisponiveis = res.createNestedArray("redesDisponiveis");
        int quant = WiFi.scanNetworks();
        for (int i = 0; i < quant; i++)
            redesDisponiveis.add(WiFi.SSID(i));
        serializeJson(res, SerialBT);
    }

    void SetDados(DynamicJsonDocument* req) {
        Memoria::idEstacao = strdup(req->getMember("idEstacao"));
        Memoria::senhaWiFi = strdup(req->getMember("senhaWiFi"));
        Memoria::ssidWiFi = strdup(req->getMember("ssidWiFi"));
        Memoria::latRegistrada = req->getMember("latRegistrada");
        Memoria::lonRegistrada = req->getMember("lonRegistrada");
        Memoria::Salvar();
        SerialBT.print("OK");
    }

    void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
    {
        if (event == ESP_SPP_DATA_IND_EVT) {
            auto req = *Requisicao;
            deserializeJson(req, SerialBT);
            const char* metodo = req["metodo"];
            if (!strcmp(metodo, "GetDados")) {
                GetDados();
            } else if (!strcmp(metodo, "SetDados")) {
                SetDados(&req);
            }
        }
    }

    void Iniciar(int ledBluetooth) {
        Requisicao = new DynamicJsonDocument(1024);
        RetornoInfos = new DynamicJsonDocument(1024);
        SerialBT.register_callback(callback);
        pinMode(ledBluetooth, OUTPUT);
        if (SerialBT.begin("Estação")) {
            digitalWrite(ledBluetooth, HIGH);
        }
    }
}