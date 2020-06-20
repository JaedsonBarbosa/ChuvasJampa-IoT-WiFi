//#include <sstream>
#include <WiFi.h>
#include <HTTPClient.h>
#include "Partes/Configuracoes.hpp"

const int ONBOARD_LED = 2;
const int PINO_CONTROLE = 4;
const String MAC = WiFi.macAddress();

Configuracoes::Gerenciador  * EstConfiguracoes;

int MakeRequest(HTTPClient * http, String function, std::string content) {
    http->begin("http://192.168.0.108:5001/chuvasjampa/us-central1/" + function);
    http->setConnectTimeout(120000);
    http->setTimeout(120000);
    http->addHeader("Content-Type", "application/json");
    return http->POST(content.c_str());
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    EstConfiguracoes = new Configuracoes::Gerenciador();
    EstConfiguracoes->Iniciar();
    return;

    pinMode(ONBOARD_LED, OUTPUT);
    pinMode(PINO_CONTROLE, INPUT_PULLDOWN);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado.");
        //WiFi.begin(ConfigsAtuais.ssidWiFi, ConfigsAtuais.senhaWiFi);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.println("Conectando...");
        }
        WiFi.setAutoReconnect(true);
    }
    Serial.println("Conectado");
}

bool lastEstado = false;
void loop() {
    EstConfiguracoes->Executar();
    // return;
    // bool estado = digitalRead(PINO_CONTROLE) == HIGH;
    // if (estado && !lastEstado && WiFi.status() == WL_CONNECTED) {
    //     digitalWrite(ONBOARD_LED, HIGH);
    //     std::stringstream mensagem;
    //     mensagem << "{\"data\":" << ultimaLeitura.date << ",\"hora\":" << ultimaLeitura.time << ",\"MAC\":\"" << MAC.c_str() << "\"}";
    //     HTTPClient http;
    //     int httpCode = MakeRequest(&http, "AdicionarRegistro", mensagem.str().c_str());
    //     if (httpCode == 201) { //Check for the returning code
    //         Serial.println("Registrado com sucesso.");
    //     } else { Serial.println(http.errorToString(httpCode)); }
    //     http.end();
    //     digitalWrite(ONBOARD_LED, LOW);
    // }
    // lastEstado = estado;
    // delay(100);
}
