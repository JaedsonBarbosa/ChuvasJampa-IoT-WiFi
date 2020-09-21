#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Definimos aqui qual o pino que será conectado ao pluviômetro
#define PINO_PLUVIOMETRO GPIO_NUM_15
#define SECS_ENTRE_VARREDURAS 600
#define SECS_BT_ATIVADO 600
#define SECS_ENTRE_ACORDADAS 1200
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
        Json["relogioConfiguradoNTP"] = time(0) > limiteTempo;
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
        if (https.POST(envioC) == 201) registros.clear();
        https.end();
    }
    catch(const std::exception& e) { }
}

void setup()
{
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
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
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        try { configTime(-3 * 3600, 0, "pool.ntp.org"); }
        catch(const std::exception& e) {}
    }, SYSTEM_EVENT_STA_GOT_IP);
    WiFi.begin(ssidWiFi, senhaWiFi);

    time_t atual;
    do {
        atual = time(0);
        if (digitalRead(PINO_PLUVIOMETRO)) {
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
    
    digitalWrite(2, LOW);
    esp_light_sleep_start();
}

void loop()
{
    auto atual = time(0);
    bool novoRegistro = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1;
    if (novoRegistro) registros.push_back(atual);
    if (registros.size() > 0 && atual - ultimaVarreduraMemoria > SECS_ENTRE_VARREDURAS) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssidWiFi, senhaWiFi);
        while (WiFi.status() != WL_CONNECTED) {
            if (time(0) - atual > 4) break;
            else delay(100);
        }
        if (WiFi.status() == WL_CONNECTED) {
            EnviarParaNuvem();
            ultimaVarreduraMemoria = atual;
        }
        WiFi.mode(WIFI_OFF);
    }
    else if (novoRegistro) delay(500);
    esp_light_sleep_start();
}
