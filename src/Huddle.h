// SAFE UFRJ - CAPES TECNODIGITAL
// BIoT Device
// Authors: Guilherme Horta Travassos


/*
 *  Pinout
 */
#define PIN_DHT11 GPIO_NUM_23

/*
 *  Configurations & Macros
 */

// Wi-Fi
#define WIFI_SSID "LENS-ESE"
#define WIFI_PASSWORD "LensESE*789"

// MQTT
#define MQTT_SERVER "k3s.cos.ufrj.br"
#define MQTT_PORT 30150
#define BROKER_USERNAME "cos603"
#define BROKER_PASSWORD ".2022pesc"
#define MQTT_CLIENT_ID "BIOT"
#define MQTT_TOPIC "HUDDLE_MATERIAIS"
#define MQTT_TOPIC_ID "idBIoT"
#define MQTT_TOPIC_DATAHORA "dataHoraMedicao"
#define MQTT_TOPIC_TEMP "temperatura"
#define MQTT_TOPIC_HUM "umidade"
#define MQTT_PERIOD_MS 5000

// Debounce
#define DEBOUNCE_DELAY 50

// CO2 and Temperature
#define DHTTYPE DHT11

// Serial
#define DEBUG_MODE // Comment this line for disabling Serial
#define SERIAL_BAUD_RATE 115200

// Multi-core
#define CORE_ZERO 0
#define CORE_ONE 1
#define PRIORITY_DEFAULT 1
#define STACK_SIZE_DEFAULT 10000
#define NO_PARAMETERS NULL

/*
* Tratamento de Data e Hora da Internet
*/
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = - 3 * 3600;

//Ajuste do fuso horário
const int   daylightOffset_sec = 0;

/*
 *  Constructors
 */

#define RMT_CLK_DIV             100         /* RMT counter clock divider */
#define RMT_TX_CARRIER_EN       0           /* Disable carrier  */
#define rmt_item32_tIMEOUT_US   9500        /*!< RMT receiver timeout value(us) */

#define RMT_TICK_10_US          (80000000/RMT_CLK_DIV/100000)   /* RMT counter value for 10 us.(Source clock is APB clock) */
#define ITEM_DURATION(d)  ((d & 0x7fff)*10/RMT_TICK_10_US)
#define TASK_DELAY              50    /* Taks Delay in ms */
#define TOTAL_MEASURES          60    /* Total f measures for sensors */


/* 
* Intervalos de Atuação
*/
#define INTERVALO_ATUALIZACAO_DADOS_BROKER 300  // 15 SEGUNDOS
#define INTERVALO_ATUALIZACAO_DADOS_TEMPERATURA 150 // 7,5 SEGUNDOS
#define INTERVALO_ATUALIZACAO_NOTIFICACOES 200 // 10 SEGUNDOS
