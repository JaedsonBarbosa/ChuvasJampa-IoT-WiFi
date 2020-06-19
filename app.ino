#include "TinyGPS/TinyGPS.cpp"
#include "Configuracoes/Dados.cpp"
#include "Configuracoes/Gerenciador.cpp"
#include <sstream>
#include <WiFi.h>
#include <HTTPClient.h>

const int ONBOARD_LED = 2;
const int PINO_CONTROLE = 4;
const String MAC = WiFi.macAddress();

TinyGPS gps;
struct InfoGPS {
    long lat, lon;
    unsigned long time, date;
};
InfoGPS ultimaLeitura;

Configuracoes::Gerenciador EstConfiguracoes;

int MakeRequest(HTTPClient * http, String function, std::string content = "") {
    const char * URL_BASE_SERVIDOR = "http://192.168.0.108:5001/chuvasjampa/us-central1/"; //KKK
    http->begin(URL_BASE_SERVIDOR + function);
    http->setConnectTimeout(120000);
    http->setTimeout(120000);
    if (content != "") {
        http->addHeader("Content-Type", "application/json");
        return http->POST(content.c_str());
    } else { return http->GET(); }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    
    auto ConfigsAtuais = EstConfiguracoes.ConfigsAtuais;
    Serial.println("Configurações usadas.");
    Serial.println(ConfigsAtuais.nomeEstacao);
    Serial.println(ConfigsAtuais.codIBGE);
    Serial.println(ConfigsAtuais.senhaWiFi);
    Serial.println(ConfigsAtuais.ssidWiFi);

    EstConfiguracoes.Iniciar();
    return;

    pinMode(ONBOARD_LED, OUTPUT);
    pinMode(PINO_CONTROLE, INPUT_PULLDOWN);
    Serial2.begin(9600, SERIAL_8N1, 16, 17);
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado.");
        WiFi.begin(ConfigsAtuais.ssidWiFi, ConfigsAtuais.senhaWiFi);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.println("Conectando...");
        }
        WiFi.setAutoReconnect(true);
    }
    Serial.println("Conectado");
    
    while(!AnalisarDadosGPS()) {
        Serial.println("Aguardando sinal GPS...");
        delay(1000);
    }
    Serial.println("Conectado ao GPS");

    HTTPClient http;
    http.setReuse(true);
    int codigo = MakeRequest(&http, "BuscarCadastroPlaca?mac=" + MAC);
    bool tudoCerto = false;
    if (codigo == 200) {
        Serial.println("A placa já está cadastrada no servidor.");
    } else if (codigo == 404) {
        Serial.println("A placa não está cadastrada, fazendo cadastro...");
        http.end();
        std::stringstream mensagem;
        mensagem << "{\"MAC\":\"" << MAC.c_str() << "\",\"Nome\":\"" << ConfigsAtuais.nomeEstacao << "\",\"CodIBGE\":" << ConfigsAtuais.codIBGE << ",\"Latitude\":" << ultimaLeitura.lat << ",\"Longitude\":" << ultimaLeitura.lon << "}";
        std::string msgStr = mensagem.str();
        Serial.println(msgStr.c_str());
        codigo = MakeRequest(&http, "CadastrarPlaca", msgStr);
        if (codigo == 201) {
            Serial.println("Placa cadastrada com sucesso.");
        } else {
            Serial.println("Erro no cadastro da placa.");
        }
    } else {
        Serial.println("Erro na requisição.");
    }
    http.end();
}

bool AnalisarDadosGPS() {
    while (Serial2.available() > 0)
        gps.encode(Serial2.read());
    InfoGPS possivelLeitura;
    // retrieves +/- lat/long in 100000ths of a degree
    gps.get_position(&possivelLeitura.lat, &possivelLeitura.lon);
    // time in hhmmsscc, date in ddmmyy
    gps.get_datetime(&possivelLeitura.date, &possivelLeitura.time);
    if (possivelLeitura.lat != TinyGPS::GPS_INVALID_F_ANGLE &&
        possivelLeitura.lon != TinyGPS::GPS_INVALID_F_ANGLE && 
        possivelLeitura.lat != 999999999 &&
        possivelLeitura.lon != 999999999) {
        ultimaLeitura = possivelLeitura;
        return true;
    } return false;
}

bool lastEstado = false;
void loop() {
    EstConfiguracoes.Executar();
    return;
    AnalisarDadosGPS();
    bool estado = digitalRead(PINO_CONTROLE) == HIGH;
    if (estado && !lastEstado && WiFi.status() == WL_CONNECTED) {
        digitalWrite(ONBOARD_LED, HIGH);
        std::stringstream mensagem;
        mensagem << "{\"data\":" << ultimaLeitura.date << ",\"hora\":" << ultimaLeitura.time << ",\"MAC\":\"" << MAC.c_str() << "\"}";
        HTTPClient http;
        int httpCode = MakeRequest(&http, "AdicionarRegistro", mensagem.str().c_str());
        if (httpCode == 201) { //Check for the returning code
            Serial.println("Registrado com sucesso.");
        } else { Serial.println(http.errorToString(httpCode)); }
        http.end();
        digitalWrite(ONBOARD_LED, LOW);
    }
    lastEstado = estado;
    delay(100);
}
