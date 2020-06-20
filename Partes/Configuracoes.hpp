#include "SuperEstado.hpp"
#include <Preferences.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

namespace Configuracoes {
    class Dados {
        Preferences preferences;
        const char * CONFIGURACOES = "configs";

        public:
        bool ativa;
        char * nomeEstacao;
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
                nomeEstacao = new char[32];
                ssidWiFi = new char[32];
                senhaWiFi = new char[64];
                preferences.getString("nomeEstacao", nomeEstacao, 32);
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
            preferences.putString("nomeEstacao", nomeEstacao);
            preferences.putString("senhaWiFi", senhaWiFi);
            preferences.putString("ssidWiFi", ssidWiFi);
            preferences.putLong64("ultimaAtt", ultimaAtt);
            preferences.end();
        }
    };

    class Gerenciador {
        Dados ConfigsAtuais;
        BluetoothSerial SerialBT;
        DynamicJsonDocument * Requisicao;
        DynamicJsonDocument * RetornoInfos;

        const char* ServidorNTP = "pool.ntp.org";
        const int HorarioBrasilia = -3600 * 3;

        public:

        Gerenciador() {
            //configTime(0, HorarioBrasilia, ServidorNTP);
            Requisicao = new DynamicJsonDocument(1024);
            RetornoInfos = new DynamicJsonDocument(1024);
        }

        void Iniciar() {
            SerialBT.begin("Estação");
        }

        void Encerrar() {
            SerialBT.flush();
            SerialBT.disconnect();
            SerialBT.end();
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

        void GetInfos() {
            auto res = *RetornoInfos;
            res["MAC"] = WiFi.macAddress();
            res["isConectado"] = WiFi.status() == WL_CONNECTED;
            res["ativa"] = ConfigsAtuais.ativa;
            res["nomeEstacao"] = ConfigsAtuais.nomeEstacao;
            res["ssidWiFi"] = ConfigsAtuais.ssidWiFi;
            res["ultimaAtt"] = ConfigsAtuais.ultimaAtt;
            res["senhaWiFi"] = ConfigsAtuais.senhaWiFi;
            JsonArray redesDisponiveis = res.createNestedArray("redesDisponiveis");
            int quant = WiFi.scanNetworks();
            for (int i = 0; i < quant; i++)
                redesDisponiveis.add(WiFi.SSID(i));
            serializeJson(res, SerialBT);
        }

        void SalvarConfigs(DynamicJsonDocument* req) {
            ConfigsAtuais.nomeEstacao = strdup(req->getMember("nomeEstacao"));
            ConfigsAtuais.senhaWiFi = strdup(req->getMember("senhaWiFi"));
            ConfigsAtuais.ssidWiFi = strdup(req->getMember("ssidWiFi"));
            ConfigsAtuais.ativa = req->getMember("ativa");
            ConfigsAtuais.ultimaAtt = time(0);
            ConfigsAtuais.Salvar();
            SerialBT.print("OK");
        }
    };
}