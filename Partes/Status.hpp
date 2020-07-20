#pragma once

// Gerenciamos os status de cada sistema em execução
namespace Status
{
    bool nuvemConectada = false;
    bool bluetoothAtivado = false;
    bool relogioConfiguradoGPS = false;
    bool relogioConfiguradoNTP = false;
    bool wifiConectado = false;
    bool gpsConectado = false;
}