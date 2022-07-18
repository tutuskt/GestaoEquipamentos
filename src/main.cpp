#include <Arduino.h>
#include <mfrc522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <WiFi.h>
#include <PubSubClient.h>

#define SS_PIN    21
#define RST_PIN   22

#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16

#define pinVerde     12
#define pinVermelho  32

const char *SSID = "TP-Link 2";
const char *PWD = "@@AP702@2020";
const char *brokerUser = "cos603";
const char *brokerPass = ".2022pesc";

//esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação
MFRC522::StatusCode status;

// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); 

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient); 
char *mqttServer ="k3s.cos.ufrj.br";
int mqttPort = 30150;

void setup() {
  // Inicia a serial
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus

  pinMode(pinVerde, OUTPUT);
  pinMode(pinVermelho, OUTPUT);
  
  // Inicia MFRC522
  mfrc522.PCD_Init(); 

  //setup mqtt broker
  mqttClient.setServer(mqttServer, mqttPort);
}

unsigned long getID(){
  if ( ! mfrc522.PICC_ReadCardSerial()) { //Since a PICC placed get Serial and continue
    return -1;
  }
  unsigned long hex_num;
  hex_num =  mfrc522.uid.uidByte[0] << 24;
  hex_num += mfrc522.uid.uidByte[1] << 16;
  hex_num += mfrc522.uid.uidByte[2] <<  8;
  hex_num += mfrc522.uid.uidByte[3];
  mfrc522.PICC_HaltA(); // Stop reading
  return hex_num;
}

//faz a leitura dos dados do cartão/tag
unsigned long leituraDados()
{
  //imprime os detalhes tecnicos do cartão/tag
  unsigned long hex_num = -1; //Since a PICC placed get Serial and continue  
  hex_num =  mfrc522.uid.uidByte[0] << 24;
  hex_num += mfrc522.uid.uidByte[1] << 16;
  hex_num += mfrc522.uid.uidByte[2] <<  8;
  hex_num += mfrc522.uid.uidByte[3];
  mfrc522.PICC_HaltA(); // Stop reading
  
  if(hex_num != -1){
    Serial.print("Card detected, UID: "); 
    Serial.println(hex_num);
  }
  return hex_num;
   //mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); 
}

//Conecta o esp32 ao wifi
void connect_wifi(){

  Serial.print("Connecting to ");
  Serial.println(SSID);  
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(SSID, PWD);
    digitalWrite(pinVermelho, HIGH);
    delay(5000); //espera conectar para o status mudar
  }
  Serial.println("Connected.");
  digitalWrite(pinVermelho, LOW);
  digitalWrite(pinVerde, HIGH);
  delay(5000);
  digitalWrite(pinVerde, LOW);
}

//Conexão com o broker
void reconnect() {
  Serial.println("Connecting to MQTT Broker...");
  while (!mqttClient.connected()) {
      Serial.println("Reconnecting to MQTT Broker..");
      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);
      
      if (mqttClient.connect(clientId.c_str(),brokerUser, brokerPass)) {
        Serial.println("Connected.");
      }
      
  }
}

void loop() 
{
  if(WiFi.status() != WL_CONNECTED){
    connect_wifi();
  }
  
  if (!mqttClient.connected())
    reconnect();
  mqttClient.loop();

  //Aguarda a aproximacao do cartao
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Seleciona um dos cartoes
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

  // Mensagens iniciais no serial monitor
  Serial.println("\nAproxime o seu cartao do leitor...\n");

  unsigned long uid = leituraDados();
  const char *topic = "EQUIPAMENTOS";
  String macAddress = WiFi.macAddress();
  String env = "MacAddress:";
  env += macAddress;
  mqttClient.publish(topic, env.c_str());

  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA(); 
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();  
}
