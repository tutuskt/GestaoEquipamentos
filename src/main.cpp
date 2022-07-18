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

//esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação
MFRC522::StatusCode status;

// Definicoes pino modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); 

void setup() {
  // Inicia a serial
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus

  pinMode(pinVerde, OUTPUT);
  pinMode(pinVermelho, OUTPUT);
  
  // Inicia MFRC522
  mfrc522.PCD_Init(); 

  //setupMQTT();
}

//faz a leitura dos dados do cartão/tag
void leituraDados()
{
  // Mensagens iniciais no serial monitor
  Serial.println("\nAproxime o seu cartao do leitor...\n");

  //imprime os detalhes tecnicos do cartão/tag
  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); 

  //Prepara a chave - todas as chaves estão configuradas para FFFFFFFFFFFFh (Padrão de fábrica).
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //buffer para colocar os dados lidos
  byte buffer[SIZE_BUFFER] = {0};

  //bloco que faremos a operação
  byte bloco = 1;
  byte tamanho = SIZE_BUFFER;

  //faz a autenticação do bloco que vamos operar
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, bloco, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(pinVermelho, HIGH);
    delay(1000);
    digitalWrite(pinVermelho, LOW);
    return;
  }

  //faz a leitura dos dados do bloco
  status = mfrc522.MIFARE_Read(bloco, buffer, &tamanho);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(pinVermelho, HIGH);
    delay(1000);
    digitalWrite(pinVermelho, LOW);
    return;
  }
  else{
      digitalWrite(pinVerde, HIGH);
      delay(1000);
      digitalWrite(pinVerde, LOW);
  }

  Serial.print(F("\nDados bloco ["));
  Serial.print(bloco);Serial.print(F("]: "));

  //imprime os dados lidos
  for (uint8_t i = 0; i < MAX_SIZE_BLOCK; i++)
  {
      Serial.write(buffer[i]);
  }
  Serial.println(" ");
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
  Serial.print("Connected.");
  digitalWrite(pinVermelho, LOW);
  digitalWrite(pinVerde, HIGH);
  delay(5000);
  digitalWrite(pinVerde, LOW);
}

// void setupMQTT(){
//   WiFiClient wifiClient;
//   PubSubClient mqttClient(wifiClient); 
//   char *mqttServer ="k3s.cos.ufrj.br";
//   int mqttPort = 30150;
//   mqttClient.setServer(mqttServer, mqttPort);
// }

void loop() 
{
  if(WiFi.status() != WL_CONNECTED){
    connect_wifi();
  }
  
   // Aguarda a aproximacao do cartao
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Seleciona um dos cartoes
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }

  leituraDados();

  // instrui o PICC quando no estado ACTIVE a ir para um estado de "parada"
  mfrc522.PICC_HaltA(); 
  // "stop" a encriptação do PCD, deve ser chamado após a comunicação com autenticação, caso contrário novas comunicações não poderão ser iniciadas
  mfrc522.PCD_StopCrypto1();  
}
