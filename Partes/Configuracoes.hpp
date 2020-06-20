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
        bool isLimpo;
        char * nomeEstacao;
        char * ssidWiFi;
        char * senhaWiFi;
        time_t ultimaAtt;

        Dados() {
            preferences.begin(CONFIGURACOES, true);
            isLimpo = ultimaAtt == 0;
            if (isLimpo) {
                Serial.println("Não existem configurações salvas.");
            } else {
                Serial.println("Já existem configurações salvas.");
                preferences.getString("nomeEstacao", nomeEstacao, 128);
                preferences.getString("senhaWiFi", senhaWiFi, 64);
                preferences.getString("ssidWiFi", ssidWiFi, 64);
                ultimaAtt = preferences.getLong64("ultimaAtt");
            }
            preferences.end();
        }

        void Salvar() {
            preferences.begin(CONFIGURACOES);
            preferences.putString("nomeEstacao", nomeEstacao);
            preferences.putString("senhaWiFi", senhaWiFi);
            preferences.putString("ssidWiFi", ssidWiFi);
            preferences.putLong64("ultimaAtt", ultimaAtt);
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
                serializeJson(req, Serial);
                const char* metodo = req["metodo"];
                if (!strcmp(metodo, "GetInfos")) {
                    GetInfos();
                } else if (!strcmp(metodo, "SalvarConfigs")) {
                    ConfigsAtuais.nomeEstacao = strdup(req["nomeEstacao"]);
                    ConfigsAtuais.senhaWiFi = strdup(req["senhaWiFi"]);
                    ConfigsAtuais.ssidWiFi = strdup(req["ssidWiFi"]);
                    Serial.println("Passo 2");
                    Serial.println("Passo 3");
                    ConfigsAtuais.isLimpo = false;
                    Serial.println("Passo 4");
                    ConfigsAtuais.ultimaAtt = time(0);
                    Serial.println("Passo 5");
                    ConfigsAtuais.Salvar();
                    Serial.println("Passo 6");
                    Serial.println("Configurações salvas.");
                    SerialBT.write(1);
                }
            }
        }

        void GetInfos() {
            auto res = *RetornoInfos;
            res["MAC"] = WiFi.macAddress();
            res["isConectado"] = WiFi.status() == WL_CONNECTED;
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
    };
}