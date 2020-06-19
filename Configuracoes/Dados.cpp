#include <Preferences.h>
#define CONFIGURACOES "configs"
namespace Configuracoes {
    class Dados {
        Preferences preferences;

        public:
        bool isLimpo;
        char * nomeEstacao;
        char * ssidWiFi;
        char * senhaWiFi;
        int codIBGE;
        //time_t ultimaAtt;

        Dados() {
            preferences.begin(CONFIGURACOES, true);
            isLimpo = codIBGE == 0;
            if (isLimpo) {
                Serial.println("Não existem configurações salvas.");
                nomeEstacao = "Teste";
                ssidWiFi = "Jaedson Privado";
                senhaWiFi = "C#C++uwp20";
                codIBGE = 2505204;
            } else {
                Serial.println("Já existem configurações salvas.");
                codIBGE = preferences.getInt("codIBGE");
                //ultimaAtt = preferences.getLong64("ultimaAtt");
                preferences.getString("nomeEstacao", nomeEstacao, 128);
                preferences.getString("senhaWiFi", senhaWiFi, 64);
                preferences.getString("ssidWiFi", ssidWiFi, 64);
            }
            preferences.end();
        }
    };
}