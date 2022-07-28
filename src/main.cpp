#include <Arduino.h>
#include <mfrc522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
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

bool CurrentCardPresentStatus = false;         // estado atual do sensor
bool lastCardPresentStatus = false;     // estado anterior do sensor
String tagEquipament = "";
String personTag = "";

/*
 *  Logging wrapper
 */
void log(const char *msg, const char *end = "\n")
{
  Serial.print(msg);
  Serial.print(end);
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
        log("[INFO] Connected.\n");
      }
      else
      {
        log("[WARNING] Failed to connect to the MQTT broker, rc=", "");
        log(mqttClient.state());
        delay(500);
      }
      
  }
}

//Publica mensagem no broker
void mqttPublish(String uidEquipament, String uidPerson){
  
  StaticJsonDocument<200> doc;
  char json_buffer[200];
  struct tm timeinfo;
  String Vazio = "VAZIO";
  char dataHorabuffer[80];
  String dataHora;

  if (uidPerson == "CANCELAR"){
    CurrentCardPresentStatus = true; //Volta ao estado anterior que o equipamento está no deck
    return;
  }

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

  doc.clear();
  doc["macAddress"] = MacAddress;
  doc["dataHoraEvento"] = dataHora;
  doc["equipamentTag"] = uidEquipament;
  doc["personTag"] = uidPerson;
  

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

//Faz a leitura dos dados do cartão/tag
String readTagUid()
{
  String conteudo = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

    //Serial.print("Card detected, UID: "); 
    conteudo.toUpperCase();

  return conteudo.substring(1);
}

String removalAuthentication(){

  int count = 0;
  int timeToReadTag = 0;

  byte bufferATQA[2];
	byte bufferSize = sizeof(bufferATQA);

  while(timeToReadTag != 30){  //Espera 30 segundos a leitura de outra tag

    digitalWrite(pinVermelho, HIGH);

    if(!mfrc522.PICC_WakeupA(bufferATQA, &bufferSize)){    
      if(!mfrc522.PICC_ReadCardSerial()){
        digitalWrite(pinVermelho, LOW);
        return "";
      }

      personTag = readTagUid();
      Serial.print("[INFO] Tag do Equipamento: ");
      Serial.println(tagEquipament);
      Serial.print("[INFO] Tag do Servidor: ");
      Serial.println(personTag);

      if(personTag == tagEquipament){   //Verifica se é a mesma tag, se for, entende que foi uma retirada falsa e não gera evento.
        log("[Erro] Evento de retirada cancelado!\n");
        digitalWrite(pinVermelho, LOW);
        digitalWrite(pinVerde, HIGH);
        delay(2000);
        digitalWrite(pinVerde, LOW);
        return "CANCELAR";
      }

      digitalWrite(pinVermelho, LOW);
      digitalWrite(pinVerde, HIGH);
      delay(2000);
      digitalWrite(pinVerde, LOW);
      return personTag;
    }
    
    delay(1000);
    timeToReadTag++;
  }

  while(count != 5){  //Led vermelho pisca para indicar que a leitura da tag do servidor não foi feita, gerando um evento de saída não autenticada
    digitalWrite(pinVermelho, LOW);
    delay(500);
    digitalWrite(pinVermelho, HIGH);
    delay(500);
    count++;
  }

  digitalWrite(pinVermelho, LOW);
  return "";
}

void testPublish(String uid){   //DESATUALIZADO! Usado apenas para testes!
  StaticJsonDocument<200> doc;
  char json_buffer[200];
  struct tm timeinfo;
  String Vazio = "VAZIO";
  char dataHorabuffer[80];
  String dataHora;
  
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

  doc.clear();
  doc["macAddress"] = MacAddress;
  doc["dataHoraEvento"] = dataHora;
  doc["equi_RFID"] = uid;
  

  serializeJsonPretty(doc, json_buffer);
  Serial.println(json_buffer);
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

  if (!mqttClient.connected()){
    connectMqtt();
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
    
  if (!mqttClient.connected()){
    connectMqtt();
  }

  mqttClient.loop();
  
  byte bufferATQA[2];
	byte bufferSize = sizeof(bufferATQA);

  //Aguarda a aproximação do cartao 
  CurrentCardPresentStatus = !mfrc522.PICC_WakeupA(bufferATQA, &bufferSize); //Esse cara resolve tudo, NUNCA USAR PICC_IsNewCardPresent
  
  if (CurrentCardPresentStatus != lastCardPresentStatus) {
    if (CurrentCardPresentStatus == true) {
      if(!mfrc522.PICC_ReadCardSerial()){ return; }
      if(personTag == readTagUid()){  return; }

      Serial.println("[INFO] Lendo tag do equipamento...");  
      tagEquipament = readTagUid();
      Serial.print("[INFO] Cartão detectado! UID: ");
      Serial.println(tagEquipament);
      mqttPublish(tagEquipament, "");
      Serial.println("------------------------------------------\n");
    }
    else {
      Serial.println("[INFO] Equipamento saindo do deck");  
      String personTag = removalAuthentication();  //personTag local
      mqttPublish("", personTag);
      Serial.println("------------------------------------------\n");
    }
  }

   lastCardPresentStatus = CurrentCardPresentStatus;

  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA(); 
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();
  
}
