#include"SuperEstado.hpp"
#include<HTTPClient.h>
#include<ArduinoJson.h>

namespace Normal
{
    class Gerenciador : SuperEstado {
        const int PINO_PLUV = 23;
        Dados* ConfigsAtuais;

        public:
        Gerenciador(Dados* dados) {
            ConfigsAtuais = dados;
            pinMode(PINO_PLUV, INPUT_PULLDOWN);
        }

        void Iniciar() {
            Serial.println("Registro habilitado.");
        }

        void Encerrar() {
            Serial.println("Registro desabilitado.");
        }

        int MakeRequest(HTTPClient * http, String function, std::string content) {
            http->begin("http://192.168.0.103:5001/chuvasjampa/us-central1/" + function);
            http->setConnectTimeout(120000);
            http->setTimeout(120000);
            http->addHeader("Content-Type", "application/json");
            return http->POST(content.c_str());
        }

        void Executar() {
            if (digitalRead(PINO_PLUV)) {
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.println("Botao pressionado.");
                    if (time(0) < 1592686817) {
                        const char* ServidorNTP = "pool.ntp.org";
                        const int HorarioBrasilia = -3600 * 3;
                        configTime(0, HorarioBrasilia, ServidorNTP);
                        Serial.println("Horario ajustado.");
                    }
                    DynamicJsonDocument envio(256);
                    envio["DataHora"] = time(0);
                    envio["IdEstacao"] = ConfigsAtuais->idEstacao;
                    char envioC[256];
                    serializeJson(envio, envioC, 256);
                    Serial.println(envioC);
                    HTTPClient http;
                    int httpCode = MakeRequest(&http, "AdicionarRegistro", envioC);
                    if (httpCode == 201) {
                        Serial.println("Registrado com sucesso.");
                    } else {
                        Serial.println("Erro durante registro.");
                    }
                    http.end();
                }
                
                delay(500);
            }
        }
    };
}