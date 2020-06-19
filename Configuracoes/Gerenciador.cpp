#include "../BaseEstados/SuperEstado.cpp"
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

namespace Configuracoes {
    class Gerenciador : SuperEstado {
        BluetoothSerial SerialBT;
        DynamicJsonDocument * Requisicao;
        DynamicJsonDocument * RetornoInfos;
        DynamicJsonDocument * RetornoStatus;
        DynamicJsonDocument * RetornoGPS;

        bool isGPSAtivo;
        bool isConfiguracoesRegistradas;
        const int PinoAlimentacaoGPS = 13;
        const char* ServidorNTP = "pool.ntp.org";
        const int HorarioBrasilia = -3600 * 3;

        public:
        Dados ConfigsAtuais;

        Gerenciador() {
            pinMode(PinoAlimentacaoGPS, OUTPUT);
            digitalWrite(PinoAlimentacaoGPS, HIGH);
            configTime(0, HorarioBrasilia, ServidorNTP);
            time_t now;
            isGPSAtivo = false;
            isConfiguracoesRegistradas = false;
            Requisicao = new DynamicJsonDocument(2048);
            RetornoInfos = new DynamicJsonDocument(1024);
            RetornoStatus = new DynamicJsonDocument(128);
            RetornoGPS = new DynamicJsonDocument(512);
        }

        void Iniciar() {
            SerialBT.begin("Estação");
        }

        void Encerrar() {
            digitalWrite(PinoAlimentacaoGPS, HIGH);
            isGPSAtivo = false;
            isConfiguracoesRegistradas = false;
            SerialBT.flush();
            SerialBT.disconnect();
            SerialBT.end();
        }
    
        // O retorno informa se este estado ainda pode rodar novamente
        void Executar() {
            if (SerialBT.available()) {
                auto req = *Requisicao;
                deserializeJson(req, SerialBT);
                const char* metodo = req["metodo"];
                if (!strcmp(metodo, "GetInfos")) {
                    GetInfos();
                } else if (!strcmp(metodo, "LigarGPS")) {
                    isGPSAtivo = true;
                    digitalWrite(PinoAlimentacaoGPS, !isGPSAtivo);
                    GetStatus();
                } else if (!strcmp(metodo, "DesligarGPS")) {
                    isGPSAtivo = false;
                    digitalWrite(PinoAlimentacaoGPS, !isGPSAtivo);
                    GetStatus();
                } else if (!strcmp(metodo, "SalvarConfigs")) {
                    isConfiguracoesRegistradas = true;
                    Serial.println("Configurações salvas.");
                }
            }
        }

        void GetInfos() {
            auto res = *RetornoInfos;
            res["isConectado"] = WiFi.status() == WL_CONNECTED;
            res["nomeEstacao"] = ConfigsAtuais.nomeEstacao;
            res["ssidWiFi"] = ConfigsAtuais.ssidWiFi;
            res["codIBGE"] = ConfigsAtuais.codIBGE;
            res["senhaWiFi"] = ConfigsAtuais.senhaWiFi;
            JsonArray redesDisponiveis = res.createNestedArray("redesDisponiveis");
            int quant = WiFi.scanNetworks();
            for (int i = 0; i < quant; i++)
                redesDisponiveis.add(WiFi.SSID(i));
            serializeJson(res, SerialBT);
        }

        void GetStatus() {
            auto res = *RetornoStatus;
            res["isGPSAtivo"] = isGPSAtivo;
            res["isConfiguracoesRegistradas"] = isConfiguracoesRegistradas;
            serializeJson(res, SerialBT);
        }
    };
}