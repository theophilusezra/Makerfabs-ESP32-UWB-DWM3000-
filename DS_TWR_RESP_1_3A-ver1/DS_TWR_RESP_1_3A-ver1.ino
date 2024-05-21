#include "dw3000.h"
#define APP_NAME "DS TWR RESP_1 v1.0"

const uint8_t PIN_RST = 27;  
const uint8_t PIN_IRQ = 34; 
const uint8_t PIN_SS = 4; 

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
  DWT_STS_LEN_64,
  DWT_PDOA_M0
};

#define RNG_DELAY_MS 100

#define TX_ANT_DLY 0
#define RX_ANT_DLY 0 //16385

/*For 1st Anchor*/
static uint8_t rx_poll_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21, 0, 0};
static uint8_t tx_resp_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0, 0, 0};
static uint8_t rx_final_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define ALL_MSG_COMMON_LEN_1 10
#define ALL_MSG_SN_IDX_1 2
#define FINAL_MSG_POLL_TX_TS_IDX_1 10
#define FINAL_MSG_RESP_RX_TS_IDX_1 14
#define FINAL_MSG_FINAL_TX_TS_IDX_1 18

static uint8_t frame_seq_nb_1 = 0;
#define RX_BUF_LEN_1 24
static uint8_t rx_buffer_1[RX_BUF_LEN_1];
static uint32_t status_reg_1 = 0;
#define POLL_RX_TO_RESP_TX_DLY_UUS 900
#define RESP_TX_TO_FINAL_RX_DLY_UUS 500
#define FINAL_RX_TIMEOUT_UUS 220
#define PRE_TIMEOUT 5

static uint64_t poll_rx_ts_1;
static uint64_t resp_tx_ts_1;
static uint64_t final_rx_ts_1;

static double tof_1;
static double distance_1;

extern dwt_txconfig_t txconfig_options;
void setup() 
{
  UART_init();
  test_run_info((unsigned char *)APP_NAME);
  spiBegin(PIN_IRQ, PIN_RST);
  spiSelect(PIN_SS);

  Sleep(2);

  while (!dwt_checkidlerc()) {
    UART_puts("IDLE FAILED\r\n");
    while (1) ;
    };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        while (1) { /* spin */ };
    }
    if (dwt_configure(&config)) {
       // LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }
    dwt_configuretxrf(&txconfig_options);
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
}    

/* Loop forever responding to ranging requests. */
void loop (){
  while (1){
    dwt_setpreambledetecttimeout(0);
    dwt_setrxtimeout(0);
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    while (!((status_reg_1 = dwt_read32bitreg(SYS_STATUS_ID)) 
          & (SYS_STATUS_RXFCG_BIT_MASK |SYS_STATUS_ALL_RX_TO 
            |SYS_STATUS_ALL_RX_ERR))){ /* spin */ };
    if (status_reg_1 & SYS_STATUS_RXFCG_BIT_MASK){
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);
      uint32_t frame_len_1 = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
      
      if (frame_len_1 <= RX_BUF_LEN_1){
        dwt_readrxdata(rx_buffer_1, frame_len_1, 0);
      }
      rx_buffer_1[ALL_MSG_SN_IDX_1] = 0;
      if (memcmp(rx_buffer_1, rx_poll_msg_1, ALL_MSG_COMMON_LEN_1) == 0){
        uint32_t resp_tx_time_1;
        poll_rx_ts_1 = get_rx_timestamp_u64();
        resp_tx_time_1 = (poll_rx_ts_1 + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(resp_tx_time_1);
        dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS);
        dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);

        dwt_setpreambledetecttimeout(PRE_TIMEOUT);
        tx_resp_msg_1[ALL_MSG_SN_IDX_1] = frame_seq_nb_1;
        dwt_writetxdata(sizeof(tx_resp_msg_1), tx_resp_msg_1, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_resp_msg_1), 0, 1); /* Zero offset in TX buffer, ranging. */
        int ret = dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);
        if (ret == DWT_ERROR){
          continue;
        }
        while (!((status_reg_1 = dwt_read32bitreg(SYS_STATUS_ID)) &
                 (SYS_STATUS_RXFCG_BIT_MASK |SYS_STATUS_ALL_RX_TO |
                  SYS_STATUS_ALL_RX_ERR))){ /* spin */ };
        frame_seq_nb_1++;

        if (status_reg_1 & SYS_STATUS_RXFCG_BIT_MASK) {
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_TXFRS_BIT_MASK);
          frame_len_1 = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
          if (frame_len_1 <= RX_BUF_LEN_1){
            dwt_readrxdata(rx_buffer_1, frame_len_1, 0);
          }
          rx_buffer_1[ALL_MSG_SN_IDX_1] = 0;
          if (memcmp(rx_buffer_1, rx_final_msg_1, ALL_MSG_COMMON_LEN_1) == 0){
            uint32_t poll_tx_ts_1, resp_rx_ts_1, final_tx_ts_1;
            uint32_t poll_rx_ts_1, resp_tx_ts_1, final_rx_ts_1;
            double Ra_1, Rb_1, Da_1, Db_1;
            int64_t tof_dtu_1;
            resp_tx_ts_1 = get_tx_timestamp_u64();
            final_rx_ts_1 = get_rx_timestamp_u64();
            final_msg_get_ts(&rx_buffer_1[FINAL_MSG_POLL_TX_TS_IDX_1], &poll_tx_ts_1);
            final_msg_get_ts(&rx_buffer_1[FINAL_MSG_RESP_RX_TS_IDX_1], &resp_rx_ts_1);
            final_msg_get_ts(&rx_buffer_1[FINAL_MSG_FINAL_TX_TS_IDX_1], &final_tx_ts_1);
            poll_rx_ts_1 = (uint32_t)poll_rx_ts_1;
            resp_tx_ts_1 = (uint32_t)resp_tx_ts_1;
            final_rx_ts_1= (uint32_t)final_rx_ts_1;
            Ra_1 = (double)(resp_rx_ts_1 - poll_tx_ts_1);
            Rb_1 = (double)(final_rx_ts_1 - resp_tx_ts_1);
            Da_1 = (double)(final_tx_ts_1 - resp_rx_ts_1);
            Db_1 = (double)(resp_tx_ts_1 - poll_rx_ts_1);
            tof_dtu_1 = (int64_t)((Ra_1 * Rb_1 - Da_1 * Db_1) / (Ra_1 + Rb_1 + Da_1 + Db_1));

            tof_1 = tof_dtu_1 * DWT_TIME_UNITS;
            distance_1 = tof_1 * SPEED_OF_LIGHT;

            char dist[20] = {0};
            snprintf(dist_str, sizeof(dist_str), "distance 1: %3.2f m ", distance_1);
            test_run_info((unsigned char *)dist_str);
            Serial.println("0 "+String(distance_1)); 
            Sleep(RNG_DELAY_MS - 10);  // start couple of ms earlier
          }
        }
        else {
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        }
      }
    }
    else{
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
    }
  }
}