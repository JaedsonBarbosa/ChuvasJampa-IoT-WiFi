#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <TinyGPS++.h>
#include <WiFi.h>

// Definimos aqui qual o pino que será conectado ao pluviômetro
#define PINO_PLUVIOMETRO 23
#define POSSUI_GPS true
#define SECS_ENTRE_VARREDURAS 60

DynamicJsonDocument Json(1024);
HardwareSerial SerialGPS(2);
TinyGPSPlus gps;
Preferences preferences;
BluetoothSerial SerialBT;
std::vector<time_t> registros;
time_t ultimaVarreduraMemoria = 0;

bool nuvemConectada = true;
bool bluetoothAtivado = false;
bool relogioConfiguradoGPS = false;
bool relogioConfiguradoNTP = false;
bool wifiConectado = false;
bool gpsConectado = false;

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
        Json["possuiGPS"] = POSSUI_GPS;
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
    #if POSSUI_GPS
    else if (!strcmp(metodo, "GetGPS")) {
        auto local = gps.location;
        auto lat = local.lat();
        auto lng = local.lng();
        Json.clear();
        Json["valid"] = local.isValid() && lat != 0 && lng != 0;
        Json["lat"] = lat;
        Json["lon"] = lng;
    }
    #endif
    else if (!strcmp(metodo, "GetStatus")) {
        Json.clear();
        Json["nuvemConectada"] = nuvemConectada;
        Json["bluetoothAtivado"] = bluetoothAtivado;
        Json["relogioConfiguradoGPS"] = relogioConfiguradoGPS;
        Json["relogioConfiguradoNTP"] = relogioConfiguradoNTP;
        Json["wifiConectado"] = wifiConectado;
        Json["gpsConectado"] = gpsConectado;
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
            auto quantRemoverMemoria = registros.size();
            if (quantRemoverMemoria > 0) {
                auto inicio = registros.begin();
                // O intervalo é [inicio, inicio + quantRemoverMemoria[, por isso o + 1
                registros.erase(inicio, inicio + quantRemoverMemoria + 1);
            }
            nuvemConectada = true;
            return;
        }
    }
    catch(const std::exception& e) { }
    // Caso a operação não tenho sido bem sucedida salvamos este registro na memória
    nuvemConectada = false;
}

// Conectamos à rede cadastrada na memória
void ConectarRedeCadastrada() {
    if (strlen(senhaWiFi) >= 8) {
        wifiConectado = false;
        WiFi.begin(ssidWiFi, senhaWiFi);
    }
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

    // Analisamos se temos registros na memória
    pinMode(PINO_PLUVIOMETRO, INPUT_PULLDOWN);
    
    bluetoothAtivado = SerialBT.begin("Estacao");
    SerialGPS.begin(9600, SERIAL_8N1, 16, 17);
    
    WiFi.mode(WIFI_STA);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        wifiConectado = true;
        if (!relogioConfiguradoNTP) {
            try
            {
                configTime(-3 * 3600, 0, "pool.ntp.org");
                relogioConfiguradoGPS = false;
                relogioConfiguradoNTP = true;
            }
            catch(const std::exception& e) {}
        }
    }, SYSTEM_EVENT_STA_GOT_IP);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        wifiConectado = false;
    }, SYSTEM_EVENT_STA_DISCONNECTED);
    ConectarRedeCadastrada();

    xTaskCreatePinnedToCore([](void *arg) {
        while (time(0) < 1595386565) delay(100);
        for(;;) loop2();
    }, "loop2", 10000, NULL, 1, NULL, 0);
}

// O loop serve para analisar se alguma requisição foi recebida pelo Bluetooth
void loop()
{
    if (SerialBT.available()) ProcessarRequisicaoBluetooth();
    if (SerialGPS.available()) {
        bool dadoLido = false;
        while (SerialGPS.available()) if (gps.encode(SerialGPS.read())) dadoLido = true;
        if (!relogioConfiguradoGPS && !relogioConfiguradoNTP && dadoLido && gps.date.year() > 2020) {
            gpsConectado = true;
            struct tm t;
            t.tm_year = gps.date.year()-1900;
            t.tm_mon = gps.date.month()-1;
            t.tm_mday = gps.date.day();
            t.tm_hour = gps.time.hour();
            t.tm_min =  gps.time.minute();
            t.tm_sec = gps.time.second();
            
            timeval epoch = {mktime(&t), 0};
            timezone utc = {0,0};
            settimeofday(&epoch, &utc);
            relogioConfiguradoGPS = true;
        }
    }
    if (wifiConectado && strlen(idEstacao) > 0 && registros.size() > 0) {
        auto atual = time(0);
        if (atual - ultimaVarreduraMemoria > SECS_ENTRE_VARREDURAS) {
            SerialBT.end();
            bluetoothAtivado = false;
            EnviarParaNuvem();
            bluetoothAtivado = SerialBT.begin("Estacao");
            ultimaVarreduraMemoria = atual;
        }
    }
}

void loop2() {
    if (digitalRead(PINO_PLUVIOMETRO)) {
        registros.push_back(time(0));
        delay(500);
    } else delay(10);
}
