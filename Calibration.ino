#include "dw3000.h"
#include <Wire.h>
#include "link.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define APP_NAME "SS TWR-2A INIT v1.0"

const char* ssid = "Your WiFi SSID";
const char* password = "Password";
const char* mqtt_server = "123.456.789.101";

WiFiClient espClient;
PubSubClient client(espClient);

const char* msg_topic = "UWB_Tag";

struct MyLink *uwb_data;
int index_num = 0;
long runtime = 0;
String all_json = "";

const uint8_t PIN_RST = 27;
const uint8_t PIN_IRQ = 34;
const uint8_t PIN_SS = 4;

static double tof_1;
static double tof_2;

static double distance_1;
static double distance_2;

static dwt_config_t config = {
  5,
  DWT_PLEN_128,
  DWT_PAC8,
  9,
  9,
  1,
  DWT_BR_6M8,
  DWT_PHRMODE_STD,
  DWT_PHRRATE_STD,
  (128 + 1 + 8 - 8),
  DWT_STS_MODE_OFF,
  DWT_STS_LEN_128,
  DWT_PDOA_M0
}

#define RNG_DELAY_MS 100

#define TX_ANT_DLY 16387
#define RX_ANT_DLY 16387 //16385

static uint8_t tx_poll_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8_t rx_resp_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*For 2nd Anchor*/
static uint8_t tx_poll_msg_2[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'A', 'C', 'R', '2', 0xE0, 0, 0};
static uint8_t rx_resp_msg_2[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'R', '2', 'A', 'C', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define ALL_MSG_COMMON_LEN_1 10
#define ALL_MSG_COMMON_LEN_2 10

#define ALL_MSG_SN_IDX_1 2
#define ALL_MSG_SN_IDX_2 2

#define RESP_MSG_POLL_RX_TS_IDX_1 10
#define RESP_MSG_POLL_RX_TS_IDX_2 10

#define RESP_MSG_RESP_TX_TS_IDX_1 14
#define RESP_MSG_RESP_TX_TS_IDX_2 14

#define RESP_MSG_TS_LEN_1 4
#define RESP_MSG_TS_LEN_2 4

static uint8_t frame_seq_nb_1 = 0;
static uint8_t frame_seq_nb_2 = 0;

#define RX_BUF_LEN_1 20
static uint8_t rx_buffer_1[RX_BUF_LEN_1];

#define RX_BUF_LEN_2 20
static uint8_t rx_buffer_2[RX_BUF_LEN_2];

static uint32_t status_reg_1 = 0;
static uint32_t status_reg_2 = 0;

#define POLL_TX_TO_RESP_RX_DLY_UUS 1720
#define RESP_RX_TIMEOUT_UUS 250

extern dwt_txconfig_t txconfig_options;

void setup_wifi(){
  delay(100);
  
  Serial.print("Connectign to");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros())
  Serial.println("");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length){}

void reconnect(){
  while(!client.connected()){
    Serial.print("Attempting MQTT connection...");
    
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff)), HEX);

    if(client.connect(clientId.c_str())){
      Serial.println("connected");
    }

    else{
      Serial.print("failed, rc =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup(){
  UART_init();
  test_run_info((unsigned char *)APP_NAME);
  Wire.begin();
  Serial.begin(9600);
  Serial.print("Setup DW3000...");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  spiBegin(PIN_IRQ, PIN_RST);
  spiSelect(PIN_SS);
  delay(2);

  if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR){
    UART_puts("INIT FAILED\r\n");
    while(1);
  }

  dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

  if (dwt_configure(&config)){
    UART_puts("CONFIG FAILED\r\n");
    while(1);
  }

  dwt_configuretxrf(&txconfig_options);

  dwt_setrxantennadelay(RX_ANT_DLY);
  dwt_settxantennadelay(TX_ANT_DLY);

  dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
  dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

  dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

  uwb_data = init_link();
}

void loop(){
  if(!client.connected()){
    reconnect();
  }
  client.loop();

  if((millis() - runtime) > 1000){
    make_link_json(uwb_data, &all_json);
    Serial.println(all_json);
    client.publish(msg_topic, all_json.c_str());
    runtime = millis();
  }
}