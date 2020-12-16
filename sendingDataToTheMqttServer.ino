#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient
#include <NTPClient.h>//Biblioteca do NTP.
#include <WiFiUDP.h>//Biblioteca do UDP.

WiFiUDP udp;//Cria um objeto "UDP".
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000);//Cria um objeto "NTP" com as configurações.

String hora;//Váriavel que armazenara o horario do NTP.
 
//defines:
//defines de id mqtt e tópicos para publicação e subscribe
#define TOPICO_SUBSCRIBE "MQTTFilipeFlopEnvia"     //tópico MQTT de escuta
#define TOPICO_PUBLISH   "MQTTFilipeFlopRecebe"    //tópico MQTT de envio de informações para Broker
                                                   //IMPORTANTE: recomendamos fortemente alterar os nomes
                                                   //            desses tópicos. Caso contrário, há grandes
                                                   //            chances de você controlar e monitorar o NodeMCU
                                                   //            de outra pessoa.
#define ID_MQTT  "HomeAut"     //id mqtt (para identificação de sessão)
                               //IMPORTANTE: este deve ser único no broker (ou seja, 
                               //            se um client MQTT tentar entrar com o mesmo 
                               //            id de outro já conectado ao broker, o broker 
                               //            irá fechar a conexão de um deles).
                                
  
 
// WIFI
const char* SSID = "NomeWifi"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "senhaWifi"; // Senha da rede WI-FI que deseja se conectar
  
// MQTT
const char* BROKER_MQTT = "40.84.142.230"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1234; // Porta do Broker MQTT 
 
//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
char EstadoSaida = '0';  //variável que armazena o estado atual da saída
  
//Prototypes
void initSerial();
void initWiFi();
void initMQTT();
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT(void);
void InitOutput(void);
 
/* 
 *  Implementações das funções
 */
void setup() 
{
  
  delay(10000);                // atraso de 100 milisegundos
  
  Serial.println("helo word");          // imprime uma linha
  WiFi.mode(WIFI_STA);       // configura rede no modo estacao
    //inicializações:
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();

   ntp.begin();//Inicia o NTP.
   ntp.forceUpdate();//Força o Update.

}
  
//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial 
//        o que está acontecendo.
//Parâmetros: nenhum
//Retorno: nenhum
void initSerial() 
{
    Serial.begin(115200);
}
 
//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum
void initWiFi() 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
     
    reconectWiFi();
}
  
//Função: inicializa parâmetros de conexão MQTT(endereço do 
//        broker, porta e seta função de callback)
//Parâmetros: nenhum
//Retorno: nenhum
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    //MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}
  

  
//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT() 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); 
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
  
//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum
void reconectWiFi() 
{
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
     
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
   
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}
 
//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
     
     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}
 
//Função: inicializa o output em nível lógico baixo
void InitOutput(void)
{
    pinMode(D3, INPUT);
    pinMode(D4, INPUT);
    pinMode(D6, INPUT);
    pinMode(D7, INPUT);
              
}
 
 
//programa principal
void loop() 
{   
    //garante funcionamento das conexões WiFi e ao broker MQTT
    VerificaConexoesWiFIEMQTT();
    
    //envia o status de todos os outputs para o Broker no protocolo esperado
    EnviaEstadoOutputMQTT();
 
    
   
 
    //keep-alive da comunicação com broker MQTT
    MQTT.loop();
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }

}
//Função: envia ao Broker o estado atual do output 
//Parâmetros: nenhum
//Retorno: nenhum
void EnviaEstadoOutputMQTT(void)
{ 
       
    hora = ntp.getFormattedTime();//Armazena na váriavel HORA, o horario atual.
    // teste Serial.println(hora);//Printa a hora já formatada no monitor.

    if(digitalRead(D3)== 0){
         Serial.println("Sensor n° 2 Ativo - Leste - Fim de curso n° 4 ");
         MQTT.publish(TOPICO_PUBLISH, "{'SENSOR':'1'}");
         
    }else if(digitalRead(D4)== 0){
        Serial.println("Sensor n° 1.1 ou 1.2  Ativo - Norte - Fim de curso n° 1 ou 2");
        MQTT.publish(TOPICO_PUBLISH, "{'SENSOR':'1'}");
        
    }else if(digitalRead(D6)== 0){
        Serial.println("Sensor n° 3  Ativo - Sul - Fim de curso n° 5");
        MQTT.publish(TOPICO_PUBLISH, "{'SENSOR':'2'}");
        
    }else if(digitalRead(D7)== 0){
        Serial.println("Sensor n° 4  Ativo - Oeste - Fim de curso n° 3");
        MQTT.publish(TOPICO_PUBLISH, "{'SENSOR':'3'}");
        
    } delay(1500);
}
 
