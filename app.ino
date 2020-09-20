#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Definimos aqui qual o pino que será conectado ao pluviômetro
#define PINO_PLUVIOMETRO 15
#define SECS_ENTRE_VARREDURAS 60
#define SECS_ENTRE_ACORDADAS 600
uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};

DynamicJsonDocument Json(1024);
Preferences preferences;
BluetoothSerial SerialBT;
std::vector<time_t> registros;
time_t ultimaVarreduraMemoria = 0;

bool bluetoothAtivado = false;
bool relogioConfiguradoNTP = false;

char * idEstacao = new char[24];
char * ssidWiFi = new char[32];
char * senhaWiFi = new char[64];

// Processamos as requisições
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
        JsonArray redesDisponiveis = Json.createNestedArray("redesDisponiveis");
        int quant = WiFi.scanNetworks();
        for (int i = 0; i < quant; i++) redesDisponiveis.add(WiFi.SSID(i));
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

        if (attWifi) ConectarRedeCadastrada();
        Json.clear();
        Json["success"] = true;
    }
    else if (!strcmp(metodo, "GetStatus")) {
        Json.clear();
        Json["nuvemConectada"] = true;
        Json["bluetoothAtivado"] = bluetoothAtivado;
        Json["relogioConfiguradoGPS"] = false;
        Json["relogioConfiguradoNTP"] = relogioConfiguradoNTP;
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
        bool sucesso = https.POST(envioC) == 201;
        https.end();
        if (sucesso) {
            // Operação bem sucedida, então limpamos a memória
            registros.clear();
        }
    }
    catch(const std::exception& e) { }
}

// Conectamos à rede cadastrada na memória
void ConectarRedeCadastrada() {
    if (strlen(senhaWiFi) >= 8)
        WiFi.begin(ssidWiFi, senhaWiFi);
}

// Aqui é onde tudo começa, sendo apenas executado uma vez logo ao ligar
void setup()
{
    // Resgatar configurações salvas na memória
    preferences.begin("configs", true);
    preferences.getString("idEstacao", idEstacao, 24);
    preferences.getString("ssidWiFi", ssidWiFi, 32);
    preferences.getString("senhaWiFi", senhaWiFi, 64);
    preferences.end();
    
    bluetoothAtivado = SerialBT.begin("Estacao");
    
    WiFi.mode(WIFI_STA);
    esp_wifi_set_mac(ESP_IF_WIFI_STA, &newMACAddress[0]);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (!relogioConfiguradoNTP) {
            try
            {
                configTime(-3 * 3600, 0, "pool.ntp.org");
                relogioConfiguradoNTP = true;
            }
            catch(const std::exception& e) {}
        }
    }, SYSTEM_EVENT_STA_GOT_IP);
    ConectarRedeCadastrada();

    //pinMode(2, OUTPUT);
    //digitalWrite(2, HIGH);

    pinMode(PINO_PLUVIOMETRO, INPUT);

    xTaskCreatePinnedToCore([](void *arg) {
        while (time(0) < 1595386565) delay(100);
        for(;;) {
            if (bluetoothAtivado) loop2();
            else vTaskDelete(NULL);
        }
    }, "loop2", 10000, NULL, 1, NULL, 0);
}

time_t ligadoDesde;
const auto limiteTempo = 1600548513;
// O loop serve para analisar se alguma requisição foi recebida pelo Bluetooth
void loop()
{
    if (bluetoothAtivado) {
        auto atual = time(0);
        if (SerialBT.available()) ProcessarRequisicaoBluetooth();
        if (WiFi.status() == WL_CONNECTED && strlen(idEstacao) > 0 && registros.size() > 0 && atual - ultimaVarreduraMemoria > SECS_ENTRE_VARREDURAS) {
            SerialBT.end();
            bluetoothAtivado = false;
            EnviarParaNuvem();
            bluetoothAtivado = SerialBT.begin("Estacao");
            ultimaVarreduraMemoria = atual;
        }
        if (ligadoDesde < limiteTempo && atual > limiteTempo) {
            ligadoDesde = atual - ligadoDesde;
        }
        if (atual - ligadoDesde > 30) {
            SerialBT.end();
            btStop();
            bluetoothAtivado = false;
            //digitalWrite(2, LOW);
            WiFi.mode(WIFI_OFF);
            esp_sleep_enable_timer_wakeup(SECS_ENTRE_ACORDADAS * 1000000);
            esp_sleep_enable_ext1_wakeup(1 << PINO_PLUVIOMETRO, ESP_EXT1_WAKEUP_ANY_HIGH);
            esp_light_sleep_start();
        }
    } else {
        //digitalWrite(2, HIGH);
        auto atual = time(0);
        auto motivo = esp_sleep_get_wakeup_cause();
        if (motivo == ESP_SLEEP_WAKEUP_EXT1) {
            registros.push_back(atual);
        }
        if (registros.size() > 0 && strlen(idEstacao) > 0 && atual - ultimaVarreduraMemoria > SECS_ENTRE_VARREDURAS) {
            WiFi.mode(WIFI_STA);
            ConectarRedeCadastrada();
            while (WiFi.status() != WL_CONNECTED)
                if (time(0) - atual > 5) break;
            if (WiFi.status() != WL_CONNECTED) {
                EnviarParaNuvem();
                ultimaVarreduraMemoria = atual;
            }
            WiFi.mode(WIFI_OFF);
        }
        //delay(1000);
        //digitalWrite(2, LOW);
        esp_light_sleep_start();
    }
}

void loop2() {
    if (digitalRead(PINO_PLUVIOMETRO)) {
        registros.push_back(time(0));
        delay(500);
    } else delay(10);
}
