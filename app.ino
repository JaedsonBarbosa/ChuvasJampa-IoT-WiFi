#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <NTPClient.h>

// Definimos aqui qual o pino que será conectado ao pluviômetro
#define PINO_PLUVIOMETRO GPIO_NUM_15
#define SECS_ENTRE_VARREDURAS 600
#define SECS_BT_ATIVADO 600
#define SECS_ENTRE_ACORDADAS 1800
#define EXIBIR_LOG false
uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};

DynamicJsonDocument Json(1024);
Preferences preferences;
BluetoothSerial SerialBT;
std::vector<time_t> registros;
time_t ultimaVarreduraMemoria = 0;
time_t ligadoDesde = 0;
const time_t limiteTempo = 1600548513;

char * idEstacao = new char[24];
char * ssidWiFi = new char[32];
char * senhaWiFi = new char[64];

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Processamos as requisições do aplicativo para configurar a estação
void ProcessarRequisicaoBluetooth() {
    deserializeJson(Json, SerialBT.readString());
    const char* metodo = Json["metodo"];
    if (!strcmp(metodo, "GetDados")) {
        Json.clear();
        Json["idEstacao"] = idEstacao;
        Json["isConectado"] = WiFi.status() == WL_CONNECTED;
        Json["ssidWiFi"] = ssidWiFi;
        Json["senhaWiFi"] = senhaWiFi;
        Json["possuiGPS"] = false;
    } else if (!strcmp(metodo, "SetDados")) {
        auto novaSenhaWifi = strdup(Json["senhaWiFi"]);
        auto novoSSIDWiFI = strdup(Json["ssidWiFi"]);
        auto attWifi = !strcmp(senhaWiFi, novaSenhaWifi) || !strcmp(ssidWiFi, novoSSIDWiFI);
        idEstacao = strdup(Json["idEstacao"]);
        senhaWiFi = novaSenhaWifi;
        ssidWiFi = novoSSIDWiFI;

        preferences.begin("configs", false);
        preferences.putString("idEstacao", idEstacao);
        preferences.putString("senhaWiFi", senhaWiFi);
        preferences.putString("ssidWiFi", ssidWiFi);
        preferences.end();

        if (attWifi) WiFi.begin(ssidWiFi, senhaWiFi);
        Json.clear();
        Json["success"] = true;
    }
    else if (!strcmp(metodo, "GetStatus")) {
        Json.clear();
        Json["nuvemConectada"] = true;
        Json["bluetoothAtivado"] = true;
        Json["relogioConfiguradoGPS"] = false;
        Json["relogioConfiguradoNTP"] = timeClient.getEpochTime() > limiteTempo;
        Json["wifiConectado"] = WiFi.status() == WL_CONNECTED;
        Json["gpsConectado"] = false;
    } else return;
    char envioC[1024];
    serializeJson(Json, envioC);
    SerialBT.println(envioC);
}

// Registramos tudo na nuvem
void EnviarParaNuvem() {
    Json.clear();
    Json["idEstacao"] = idEstacao;
    JsonArray datashoras = Json.createNestedArray("datashoras");
    for (int i = 0; i < registros.size(); i++) datashoras.add(registros[i]);
    try
    {
        char envioC[1024];
        serializeJson(Json, envioC, 1024);
        HTTPClient https;
        https.begin("https://us-central1-chuvasjampa.cloudfunctions.net/AdicionarRegistro");
        https.addHeader("Content-Type", "application/json");
        // Limpamos a memória se a operação bem sucedida
        if (https.POST(envioC) == 201) {
            auto inicio = registros.begin();
            if (datashoras.size() == registros.size()) {
                registros.clear();
                #if EXIBIR_LOG
                Serial.println("LIMPEZA COMPLETA");
                #endif
            }
            else {
                registros.erase(inicio, inicio + datashoras.size());
                #if EXIBIR_LOG
                Serial.println("LIMPEZA PARCIAL");
                #endif
            }
        }
        https.end();
    }
    catch(const std::exception& e) { }
}

bool liberadaInterrupcao = false;
unsigned long ultimoRegistro = 0;

void RegistroExtra() {
    auto atual = timeClient.getEpochTime();
    if (liberadaInterrupcao && atual - ultimoRegistro > 1) {
        liberadaInterrupcao = false;
        registros.push_back(atual);
        #if EXIBIR_LOG
        Serial.println("REGISTRADO 0");
        #endif
    }
}

void setup()
{
    #if EXIBIR_LOG
    Serial.begin(115200);
    #endif
    pinMode(2, OUTPUT);
    pinMode(PINO_PLUVIOMETRO, INPUT_PULLDOWN);
    
    // Resgatar configurações salvas na memória
    preferences.begin("configs", true);
    preferences.getString("idEstacao", idEstacao, 24);
    preferences.getString("ssidWiFi", ssidWiFi, 32);
    preferences.getString("senhaWiFi", senhaWiFi, 64);
    preferences.end();
    
    SerialBT.begin("Estacao");
    
    WiFi.mode(WIFI_STA);
    esp_wifi_set_mac(ESP_IF_WIFI_STA, &newMACAddress[0]);
    WiFi.begin(ssidWiFi, senhaWiFi);

    timeClient.setTimeOffset(-3);
    bool wifiConectado = false;
    bool ledLigado = false;
    time_t atual;
    do {
        if (!wifiConectado && WiFi.status() == WL_CONNECTED) {
            wifiConectado = true;
            timeClient.begin();
        }
        else if (wifiConectado) {
            timeClient.update();
        }
        atual = timeClient.getEpochTime();
        if (!ledLigado && atual > limiteTempo) {
            digitalWrite(2, HIGH);
        }
        if (digitalRead(PINO_PLUVIOMETRO) && atual > limiteTempo) {
            registros.push_back(atual);
            delay(500);
        }
        if (SerialBT.available()) ProcessarRequisicaoBluetooth();
        if (ligadoDesde < limiteTempo && atual > limiteTempo)
            ligadoDesde = atual - ligadoDesde;
    } while (
        atual - ligadoDesde < SECS_BT_ATIVADO
        || strlen(idEstacao) <= 0
        || strlen(ssidWiFi) <= 0
        || strlen(senhaWiFi) < 8
        || atual <= limiteTempo);

    SerialBT.end();
    btStop();
    if (WiFi.status() == WL_CONNECTED && strlen(idEstacao) > 0 && registros.size() > 0) {
        EnviarParaNuvem();
        ultimaVarreduraMemoria = atual;
    }
    WiFi.mode(WIFI_OFF);
    esp_sleep_enable_timer_wakeup(SECS_ENTRE_ACORDADAS * 1000000);
    esp_sleep_enable_ext1_wakeup(1 << PINO_PLUVIOMETRO, ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    gpio_pullup_dis(PINO_PLUVIOMETRO);
    gpio_pulldown_en(PINO_PLUVIOMETRO);
    attachInterrupt(PINO_PLUVIOMETRO, RegistroExtra, RISING);
    
    digitalWrite(2, LOW);
    esp_light_sleep_start();
}

bool aguardarConexao(unsigned long inicio) {
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 40) delay(100);
    return WiFi.status() == WL_CONNECTED;
}

void loop()
{
    auto momentoInicio = millis();
    auto atual = timeClient.getEpochTime();
    bool novoRegistro = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1;
    if (atual - ultimaVarreduraMemoria > SECS_ENTRE_VARREDURAS) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssidWiFi, senhaWiFi);
        if (aguardarConexao(atual)) {
            timeClient.update();
            auto tempoEspera = (millis() - momentoInicio) / 1000;
            atual = timeClient.getEpochTime() - tempoEspera;
            ultimoRegistro = atual;
            liberadaInterrupcao = true;
            if (novoRegistro) {
                registros.push_back(atual);
                #if EXIBIR_LOG
                Serial.println("REGISTRADO 1");
                #endif
            }
            if (registros.size() > 0) {
                #if EXIBIR_LOG
                Serial.println("ENVIANDO");
                #endif
                EnviarParaNuvem();
                #if EXIBIR_LOG
                Serial.println("ENVIADO");
                #endif
            }
            liberadaInterrupcao = false;
        }
        ultimaVarreduraMemoria = atual;
        WiFi.mode(WIFI_OFF);
    }
    else if (novoRegistro) {
        registros.push_back(atual);
        #if EXIBIR_LOG
        Serial.println("REGISTRADO 2");
        #endif
    }
    auto tempoDecorrido = millis() - momentoInicio;
    if (novoRegistro && tempoDecorrido < 500) {
        delay(500 - tempoDecorrido);
    }
    esp_light_sleep_start();
}
