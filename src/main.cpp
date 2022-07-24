#include <Arduino.h>
#include <mfrc522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <UUID.h>
#include <Huddle.h>

#define SS_PIN    21
#define RST_PIN   22

#define SIZE_BUFFER     18
#define MAX_SIZE_BLOCK  16

#define pinVerde     12
#define pinVermelho  32

const char *SSID = "TP-Link 2";
const char *PWD = "@@AP702@2020";

//esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação
MFRC522::StatusCode status;

// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); 

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient); 

const char *topic = "EQUIPAMENTOS";

String MacAddress;

bool CurrentCardPresentStatus = false;         // current state of the button
bool lastCardPresentStatus = false;     // previous state of the button
/*
 *  Logging wrapper
 */
void log(const char *msg, const char *end = "\n")
{
  Serial.print(msg);
  Serial.print(end);
}

//faz a leitura dos dados do cartão/tag
String leituraDados()
{
  String conteudo = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

    Serial.print("Card detected, UID: "); 
    conteudo.toUpperCase();
    Serial.println(conteudo.substring(1));

  return conteudo;
}

//Conecta o esp32 ao wifi
void connect_wifi(){

  log("[INFO] Conectando na Rede:", SSID);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(SSID, PWD);
    digitalWrite(pinVermelho, HIGH);
    delay(5000); //espera conectar para o status mudar
  }
  if(WiFi.status() == WL_CONNECTED){
    MacAddress = WiFi.macAddress();
    log("[INFO] WiFi connected!");
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(MacAddress);

    digitalWrite(pinVermelho, LOW);
    digitalWrite(pinVerde, HIGH);
    delay(5000);
    digitalWrite(pinVerde, LOW);
  }
  else{
    log("[Erro] Falha de Conexão!");
  }
}

//Conexão com o broker MQTT
void connectMqtt() {  
  while (!mqttClient.connected()) {
      Serial.println("");    
      log("[INFO] Connecting to MQTT Broker...");
      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);
      
      if (mqttClient.connect(clientId.c_str(),BROKER_USERNAME, BROKER_PASSWORD)) {
        log("[INFO] Connected.");
      }
      else
      {
        log("[WARNING] Failed to connect to the MQTT broker, rc=", "");
        log(mqttClient.state());
        delay(500);
      }
      
  }
}

void mqttPublish(String uid){
  
  StaticJsonDocument<200> doc;
  char json_buffer[200];
  struct tm timeinfo;
  String Vazio = "VAZIO";
  char dataHorabuffer[80];
  String dataHora;
  UUID idemPotencyKey;
  
  log("[INFO] PUBLICANDO NO BROKER");   

  if (!mqttClient.connected())
  {
    connectMqtt();
  }

  if (!getLocalTime(&timeinfo)){
    log("[INFO] Data e Hora não foi coletada");
      dataHora = Vazio.c_str(); 
      Serial.println(dataHora);
    
  }
  else {
        strftime(dataHorabuffer, sizeof(dataHorabuffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
        dataHora = String(dataHorabuffer);
        Serial.println(dataHora);
      }

  idemPotencyKey.generate(); //TODO: Não está gerando um novo a cada iteração
  //UuidCreate(&idemPotencyKey);
  doc.clear();
  doc["macAddress"] = MacAddress;
  doc["dataHoraEvento"] = dataHora;
  doc["equi_RFID"] = uid;
  doc["idemPotencyKey"] = idemPotencyKey;
  

  serializeJsonPretty(doc, json_buffer);

  if (!mqttClient.publish(topic, json_buffer, true))
  {
    log("[FALHA] BROKER INACESSIVEL!");
    delay(500);
  }
  else {
    log("[INFO] Mensagem publicada!");
    mqttClient.loop();
  }
}

void testPublish(String uid){
  StaticJsonDocument<200> doc;
  char json_buffer[200];
  struct tm timeinfo;
  String Vazio = "VAZIO";
  char dataHorabuffer[80];
  String dataHora;
  UUID idemPotencyKey;
  
  log("[INFO] PUBLICANDO NO BROKER");   

  if (!getLocalTime(&timeinfo)){
    log("[INFO] Data e Hora não foi coletada");
      dataHora = Vazio.c_str(); 
      Serial.print("Data e hora: ");
      Serial.println(dataHora);
    
  }
  else {
        strftime(dataHorabuffer, sizeof(dataHorabuffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
        dataHora = String(dataHorabuffer);
        Serial.print("Data e hora: ");
        Serial.println(dataHora);
      }

  idemPotencyKey.generate(); //TODO: Não está gerando um novo a cada iteração
  //UuidCreate(&idemPotencyKey);
  doc.clear();
  doc["macAddress"] = MacAddress;
  doc["dataHoraEvento"] = dataHora;
  doc["equi_RFID"] = uid;
  doc["idemPotencyKey"] = idemPotencyKey;
  

  serializeJsonPretty(doc, json_buffer);

  log("[INFO] Mensagem publicada!\n");
  mqttClient.loop();
  
}

void setup() {
  // Inicia a serial
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus

  pinMode(pinVerde, OUTPUT);
  pinMode(pinVermelho, OUTPUT);
  
  // Inicia MFRC522
  mfrc522.PCD_Init(); 

  //setup mqtt broker
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  //Connect to Wifi
  while(WiFi.status() != WL_CONNECTED){
    connect_wifi();
  }

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Mensagens iniciais no serial monitor
  Serial.println("\nAproxime o seu cartao do leitor...\n");
}

void loop() 
{
  if(WiFi.status() != WL_CONNECTED){
    connect_wifi();
  }
    
  // if (!mqttClient.connected())
  //   connectMqtt();
  // mqttClient.loop();
  
  //Aguarda a aproximação do cartao 
  CurrentCardPresentStatus = mfrc522.PICC_IsNewCardPresent();

  if (CurrentCardPresentStatus != lastCardPresentStatus) {
    if (CurrentCardPresentStatus == true) {
      Serial.println("[INFO] Lendo tag do equipamento...");  
      String uid = leituraDados();
      testPublish(uid);
      Serial.println("------------------------------------------\n");
    }
    else {
      Serial.println("[INFO] Equipamento saindo do deck");  
      testPublish("");
      Serial.println("------------------------------------------\n");
    }
  }

   lastCardPresentStatus = CurrentCardPresentStatus;

  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA(); 
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();
  
}
