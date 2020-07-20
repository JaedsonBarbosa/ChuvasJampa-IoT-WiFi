#pragma once
#include "Memoria.hpp"
#include "Status.hpp"
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Gerenciamos os registros dos pulsos
namespace Registros
{
    Preferences preferences;
    const char * nomeTabela = "registros";
    const char * campoRegistros = "valores";
    const char * campoQuantidade = "quantidade";
    time_t * registros;
    int quantidade;

    // Analisamos a memória pra ver o quem salvo lá
    void Iniciar() {
        preferences.begin(nomeTabela, true);
        auto schLen = preferences.getBytesLength(campoRegistros);
        if (schLen > 0 && schLen % sizeof(time_t) == 0) {
            char * buffer = new char[schLen + 128 * sizeof(time_t)];
            preferences.getBytes(campoRegistros, buffer, schLen);
            registros = (time_t *) buffer;
            quantidade = preferences.getInt(campoQuantidade);
        } else {
            char * buffer = new char[128 * sizeof(time_t)];
            registros = (time_t *) buffer;
            quantidade = 0;
        }
        preferences.end();
        Status::nuvemConectada = !quantidade;
    }

    // Adicionamos um registro à memória não-volátil
    void Adicionar(time_t datahora) {
        preferences.begin(nomeTabela);
        registros[quantidade++] = datahora;
        preferences.putBytes(campoRegistros, registros, quantidade * sizeof(time_t));
        preferences.putInt(campoQuantidade, quantidade);
        preferences.end();
        Status::nuvemConectada = false;
    }

    // Limpamos a memória não-volátil
    void Limpar() {
        preferences.begin(nomeTabela);
        quantidade = 0;
        preferences.putInt(campoQuantidade, quantidade);
        preferences.end();
        Status::nuvemConectada = true;
    }

    // Criamos a requisição que irá enviar os registros
    int MakeRequest(HTTPClient * http, String function, std::string content) {
        http->begin("http://192.168.0.109:5001/chuvasjampa/us-central1/" + function);
        http->setConnectTimeout(120000);
        http->setTimeout(120000);
        http->addHeader("Content-Type", "application/json");
        return http->POST(content.c_str());
    }
    
    // Registramos tudo na nuvem
    static bool registroOcupado = false;
    void Registrar(time_t datahora) {
        while (registroOcupado) delay(100); //Aguarda o fim da execução anterior
        registroOcupado = true;
        DynamicJsonDocument envio(256);
        envio["idEstacao"] = Memoria::idEstacao;
        if (quantidade != 0 || datahora == NULL) {
            JsonArray datashoras = envio.createNestedArray("datashoras");
            for (int i = 0; i < quantidade; i++)
                datashoras.add(registros[i]);
            if (datahora != NULL) datashoras.add(datahora);
        } else envio["datahora"] = datahora;
        try
        {
            if (strlen(Memoria::idEstacao) == 0) {
                Adicionar(datahora);
            } else {
                char envioC[256];
                serializeJson(envio, envioC, 256);
                HTTPClient http;
                int httpCode = MakeRequest(&http, "AdicionarRegistro", envioC);
                if (httpCode == 201) Limpar();
                else Adicionar(datahora);
                http.end();
            }
        }
        catch(const std::exception& e) { Adicionar(datahora); }
        registroOcupado = false;
    }
}