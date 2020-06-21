#include "SuperEstado.hpp"
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

namespace Configuracao {
    class Gerenciador : SuperEstado {
        Dados* ConfigsAtuais;
        BluetoothSerial SerialBT;
        DynamicJsonDocument * Requisicao;
        DynamicJsonDocument * RetornoInfos;

        public:
        Gerenciador(Dados* dados) {
            ConfigsAtuais = dados;
            Requisicao = new DynamicJsonDocument(1024);
            RetornoInfos = new DynamicJsonDocument(1024);
        }

        void Iniciar() {
            SerialBT.begin("Estação");
            Serial.println("Bluetooth habilidato.");
        }

        void Encerrar() {
            SerialBT.disconnect();
            SerialBT.end();
            Serial.println("Bluetooth desabilitado.");
        }
    
        void Executar() {
            if (SerialBT.available()) {
                auto req = *Requisicao;
                deserializeJson(req, SerialBT);
                const char* metodo = req["metodo"];
                if (!strcmp(metodo, "GetInfos")) {
                    GetInfos();
                } else if (!strcmp(metodo, "SalvarConfigs")) {
                    SalvarConfigs(&req);
                }
            }
        }

        private:
        void GetInfos() {
            auto res = *RetornoInfos;
            res["idEstacao"] = ConfigsAtuais->idEstacao;
            res["isConectado"] = WiFi.status() == WL_CONNECTED;
            res["ativa"] = ConfigsAtuais->ativa;
            res["ssidWiFi"] = ConfigsAtuais->ssidWiFi;
            res["ultimaAtt"] = ConfigsAtuais->ultimaAtt;
            res["senhaWiFi"] = ConfigsAtuais->senhaWiFi;
            JsonArray redesDisponiveis = res.createNestedArray("redesDisponiveis");
            int quant = WiFi.scanNetworks();
            for (int i = 0; i < quant; i++)
                redesDisponiveis.add(WiFi.SSID(i));
            serializeJson(res, SerialBT);
        }

        void SalvarConfigs(DynamicJsonDocument* req) {
            ConfigsAtuais->idEstacao = strdup(req->getMember("idEstacao"));
            ConfigsAtuais->senhaWiFi = strdup(req->getMember("senhaWiFi"));
            ConfigsAtuais->ssidWiFi = strdup(req->getMember("ssidWiFi"));
            ConfigsAtuais->ativa = req->getMember("ativa");
            ConfigsAtuais->ultimaAtt = time(0);
            ConfigsAtuais->Salvar();
            SerialBT.print("OK");
        }
    };
}