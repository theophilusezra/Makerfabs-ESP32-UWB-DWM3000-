#include "dw3000.h"
#define APP_NAME "DS TWR INIT v1.0"
 
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

#define TX_ANT_DLY 16387
#define RX_ANT_DLY 16387

/*For 1st Anchor*/
static uint8_t tx_poll_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21};
static uint8_t rx_resp_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0};
static uint8_t tx_final_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*For 2nd Anchor*/
static uint8_t tx_poll_msg_2[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'A', 'C', 'R', '2', 0x21, 0, 0};
static uint8_t rx_resp_msg_2[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'R', '2', 'A', 'C', 0x10, 0x02, 0, 0, 0, 0};
static uint8_t tx_final_msg_2[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'A', 'C', 'R', '2', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*For 3rd Anchor*/
static uint8_t tx_poll_msg_3[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'S', 'N', 'Y', 'L', 0x21, 0, 0};
static uint8_t rx_resp_msg_3[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'Y', 'L', 'S', 'N', 0x10, 0x02, 0, 0, 0, 0};
static uint8_t tx_final_msg_3[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'S', 'N', 'Y', 'L', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#define ALL_MSG_COMMON_LEN_1 10
#define ALL_MSG_COMMON_LEN_2 10
#define ALL_MSG_COMMON_LEN_3 10

#define ALL_MSG_SN_IDX_1 2
#define ALL_MSG_SN_IDX_2 2
#define ALL_MSG_SN_IDX_3 2

#define FINAL_MSG_POLL_TX_TS_IDX_1 10
#define FINAL_MSG_POLL_TX_TS_IDX_2 10
#define FINAL_MSG_POLL_TX_TS_IDX_3 10

#define FINAL_MSG_RESP_RX_TS_IDX_1 14
#define FINAL_MSG_RESP_RX_TS_IDX_2 14
#define FINAL_MSG_RESP_RX_TS_IDX_3 14

#define FINAL_MSG_FINAL_TX_TS_IDX_1 18
#define FINAL_MSG_FINAL_TX_TS_IDX_2 18
#define FINAL_MSG_FINAL_TX_TS_IDX_3 18

static uint8_t frame_seq_nb_1 = 0;
static uint8_t frame_seq_nb_2 = 0;
static uint8_t frame_seq_nb_3 = 0;

#define RX_BUF_LEN_1 20
static uint8_t rx_buffer_1[RX_BUF_LEN_1];

#define RX_BUF_LEN_2 20
static uint8_t rx_buffer_2[RX_BUF_LEN_2];

#define RX_BUF_LEN_3 20
static uint8_t rx_buffer_3[RX_BUF_LEN_3];

static uint32_t status_reg_1 = 0;
static uint32_t status_reg_2 = 0;
static uint32_t status_reg_3 = 0;

#define POLL_TX_TO_RESP_RX_DLY_UUS 700

#define RESP_RX_TO_FINAL_TX_DLY_UUS 700

#define RESP_RX_TIMEOUT_UUS 300

#define PRE_TIMEOUT 5

static double tof_1;
static double distance_1;

static double tof_2;
static double distance_2;

static double tof_3;
static double distance_3;

static uint64_t poll_tx_ts_1;
static uint64_t resp_rx_ts_1;
static uint64_t final_tx_ts_1;

static uint64_t poll_tx_ts_2;
static uint64_t resp_rx_ts_2;
static uint64_t final_tx_ts_2;

static uint64_t poll_tx_ts_3;
static uint64_t resp_rx_ts_3;
static uint64_t final_tx_ts_3;

extern dwt_txconfig_t txconfig_options;

/*! ---------------------------------------------------------------------------
 * @fn ds_twr_initiator()
 *
 * @brief Application entry point.
 *
 * @param  none
 *
 * @return none
 */

void setup()
{
    UART_init();
    test_run_info((unsigned char *)APP_NAME);
    spiBegin(PIN_IRQ, PIN_RST);
    spiSelect(PIN_SS);

    Sleep(2);

    while (!dwt_checkidlerc()) { 
    UART_puts("IDLE FAILED\r\n");
    while (1);
    }

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        UART_puts("INIT FAILED\r\n");      
        //LOG_INF("INIT FAILED");
        while (1) { /* spin */ };
    }

    if (dwt_configure(&config)) {
       UART_puts("CONFIG FAILED\r\n");
        //LOG_INF("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    dwt_configuretxrf(&txconfig_options);
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT);

    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);
    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
}

void loop(){
  
  /*1st Anchor Program*/
  while(1){
    tx_poll_msg_1[ALL_MSG_SN_IDX_1] = frame_seq_nb_1;
    dwt_writetxdata(sizeof(tx_poll_msg_1), tx_poll_msg_1, 0);
    dwt_writetxfctrl(sizeof(tx_poll_msg_1)+FCS_LEN, 0, 1);

    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

    while (!((status_reg_1 = dwt_read32bitreg(SYS_STATUS_ID)) & 
             (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | 
              SYS_STATUS_ALL_RX_ERR))){ /* spin */ };

    frame_seq_nb_1++;

    if (status_reg_1 & SYS_STATUS_RXFCG_BIT_MASK){

      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_TXFRS_BIT_MASK);
      uint32_t frame_len_1 = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
      
      if (frame_len_1 <= RX_BUF_LEN_1){
        dwt_readrxdata(rx_buffer_1, frame_len_1, 0);
      }

      rx_buffer_1[ALL_MSG_SN_IDX_1] = 0;
    
      if (memcmp(rx_buffer_1, rx_resp_msg_1, ALL_MSG_COMMON_LEN_1) == 0){
        uint32_t final_tx_time_1;

        poll_tx_ts_1 = get_tx_timestamp_u64();
        resp_rx_ts_1 = get_rx_timestamp_u64();

        final_tx_time_1 = (resp_rx_ts_1 + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(final_tx_time_1);

        final_tx_ts_1 = (((uint64_t)(final_tx_time_1 & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        final_msg_set_ts(&tx_final_msg_1[FINAL_MSG_POLL_TX_TS_IDX_1], poll_tx_ts_1);
        final_msg_set_ts(&tx_final_msg_1[FINAL_MSG_RESP_RX_TS_IDX_1], resp_rx_ts_1);
        final_msg_set_ts(&tx_final_msg_1[FINAL_MSG_FINAL_TX_TS_IDX_1], final_tx_ts_1);

        tx_final_msg_1[ALL_MSG_SN_IDX_1] = frame_seq_nb_1;
        dwt_writetxdata(sizeof(tx_final_msg_1), tx_final_msg_1, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_final_msg_1)+FCS_LEN, 0, 1); /* Zero offset in TX buffer, ranging bit set. */

        int ret_1 = dwt_starttx(DWT_START_TX_DELAYED);
  
        if (ret_1 == DWT_SUCCESS) {
          while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)){ /* spin */ };
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
          frame_seq_nb_1++;
        }
      }
    }
    else{
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_TXFRS_BIT_MASK);
    }
  }

  /*Achors 2 Program*/

  while(1){
    tx_poll_msg_2[ALL_MSG_SN_IDX_2] = frame_seq_nb_2;
    dwt_writetxdata(sizeof(tx_poll_msg_2), tx_poll_msg_2, 0);
    dwt_writetxfctrl(sizeof(tx_poll_msg_2)+FCS_LEN, 0, 1);

    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

    while (!((status_reg_2 = dwt_read32bitreg(SYS_STATUS_ID)) & 
             (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | 
              SYS_STATUS_ALL_RX_ERR))){ /* spin */ };

    frame_seq_nb_2++;

    if (status_reg_2 & SYS_STATUS_RXFCG_BIT_MASK){
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_TXFRS_BIT_MASK);
      uint32_t frame_len_2 = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
      
      if (frame_len_2 <= RX_BUF_LEN_2){
        dwt_readrxdata(rx_buffer_2, frame_len_2, 0);
      }
      rx_buffer_2[ALL_MSG_SN_IDX_2] = 0;
    
      if (memcmp(rx_buffer_2, rx_resp_msg_2, ALL_MSG_COMMON_LEN_2) == 0){
        uint32_t final_tx_time_2;

        poll_tx_ts_2 = get_tx_timestamp_u64();
        resp_rx_ts_2 = get_rx_timestamp_u64();

        final_tx_time_2 = (resp_rx_ts_2 + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(final_tx_time_2);

        final_tx_ts_2 = (((uint64_t)(final_tx_time_2 & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        final_msg_set_ts(&tx_final_msg_2[FINAL_MSG_POLL_TX_TS_IDX_2], poll_tx_ts_2);
        final_msg_set_ts(&tx_final_msg_2[FINAL_MSG_RESP_RX_TS_IDX_2], resp_rx_ts_2);
        final_msg_set_ts(&tx_final_msg_2[FINAL_MSG_FINAL_TX_TS_IDX_2], final_tx_ts_2);

        tx_final_msg_2[ALL_MSG_SN_IDX_2] = frame_seq_nb_2;
        dwt_writetxdata(sizeof(tx_final_msg_2), tx_final_msg_2, 0);
        dwt_writetxfctrl(sizeof(tx_final_msg_2)+FCS_LEN, 0, 1);
          
        int ret_2 = dwt_starttx(DWT_START_TX_DELAYED);
  
        if (ret_2 == DWT_SUCCESS) {
          while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)){ /* spin */ };
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
          frame_seq_nb_2++;
        }
      }
    }
    else{
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_TXFRS_BIT_MASK);
    }
  }
  
  /*3rd Anchor Program*/
  while(1){
    tx_poll_msg_1[ALL_MSG_SN_IDX_3] = frame_seq_nb_3;
    dwt_writetxdata(sizeof(tx_poll_msg_3), tx_poll_msg_3, 0);
    dwt_writetxfctrl(sizeof(tx_poll_msg_3)+FCS_LEN, 0, 1);
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);
    while (!((status_reg_3 = dwt_read32bitreg(SYS_STATUS_ID)) & 
             (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | 
              SYS_STATUS_ALL_RX_ERR))){ /* spin */ };

    frame_seq_nb_3++;

    if (status_reg_3 & SYS_STATUS_RXFCG_BIT_MASK){
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_TXFRS_BIT_MASK);
      uint32_t frame_len_3 = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
      
      if (frame_len_3 <= RX_BUF_LEN_3){
        dwt_readrxdata(rx_buffer_3, frame_len_3, 0);
      }
      rx_buffer_3[ALL_MSG_SN_IDX_3] = 0;
    
      if (memcmp(rx_buffer_3, rx_resp_msg_3, ALL_MSG_COMMON_LEN_3) == 0){
        uint32_t final_tx_time_3;

        poll_tx_ts_3 = get_tx_timestamp_u64();
        resp_rx_ts_3 = get_rx_timestamp_u64();

        final_tx_time_3 = (resp_rx_ts_3 + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(final_tx_time_3);

        final_tx_ts_3 = (((uint64_t)(final_tx_time_3 & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        final_msg_set_ts(&tx_final_msg_3[FINAL_MSG_POLL_TX_TS_IDX_3], poll_tx_ts_3);
        final_msg_set_ts(&tx_final_msg_3[FINAL_MSG_RESP_RX_TS_IDX_3], resp_rx_ts_3);
        final_msg_set_ts(&tx_final_msg_3[FINAL_MSG_FINAL_TX_TS_IDX_3], final_tx_ts_3);

        tx_final_msg_1[ALL_MSG_SN_IDX_3] = frame_seq_nb_3;
        dwt_writetxdata(sizeof(tx_final_msg_3), tx_final_msg_3, 0);
        dwt_writetxfctrl(sizeof(tx_final_msg_3)+FCS_LEN, 0, 1);
          
        int ret_3 = dwt_starttx(DWT_START_TX_DELAYED);
  
        if (ret_3 == DWT_SUCCESS) {
          while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)){ /* spin */ };
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);
          frame_seq_nb_3++;
        }
      }
    }
    else{
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_TXFRS_BIT_MASK);
    }
  }
  Sleep(RNG_DELAY_MS);
}