/*Library for DW3000*/
#include "dw3000.h"

/*Define the device*/
#define APP_NAME "DS TWR INIT v1.0"
 
/*connection pins*/
/*Reset PIN*/
const uint8_t PIN_RST = 27;

/*Interrupt Reqeuest PIN*/
const uint8_t PIN_IRQ = 34;

/*SPI Select PIN*/
const uint8_t PIN_SS = 4;

/* Default communication configuration. We use default non-STS DW mode. */
static dwt_config_t config = {
  /* Channel number. */
  5,

  /* Preamble length. Used in TX only. */
  DWT_PLEN_128,
  
  /* Preamble acquisition chunk size. Used in RX only. */
  DWT_PAC8,
  
  /* TX preamble code. Used in TX only. */
  9,
  
  /* RX preamble code. Used in RX only. */
  9,

  /* 0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol,
     2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type */
  1,

  /* Data rate. */
  DWT_BR_6M8,

  /* PHY header mode. */
  DWT_PHRMODE_STD,

  /* PHY header rate. */
  DWT_PHRRATE_STD,
  
  /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
  (128 + 1 + 8 - 8),
  
  /* STS disabled */
  DWT_STS_MODE_OFF,

  /* STS length see allowed values in Enum dwt_sts_lengths_e */
  DWT_STS_LEN_64,

  /* PDOA mode off */
  DWT_PDOA_M0
};

/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 100

/* Default antenna delay values for 64 MHz PRF. See NOTE 1 below. */
#define TX_ANT_DLY 16385
#define RX_ANT_DLY 16385

/* Frames used in the ranging process. See NOTE 2 below. */
/*For 1st Anchor*/
static uint8_t tx_poll_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x21};
static uint8_t rx_resp_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0x10, 0x02, 0, 0};
static uint8_t tx_final_msg_1[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*For 2nd Anchor*/
static uint8_t tx_poll_msg_2[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'A', 'C', 'R', '2', 0x21, 0, 0};
static uint8_t rx_resp_msg_2[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'R', '2', 'A', 'C', 0x10, 0x02, 0, 0, 0, 0};
static uint8_t tx_final_msg_2[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'A', 'C', 'R', '2', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Length of the common part of the message (up to and including the function code, 
   see NOTE 2 below). */
#define ALL_MSG_COMMON_LEN_1 10
#define ALL_MSG_COMMON_LEN_2 10

/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX_1 2
#define ALL_MSG_SN_IDX_2 2
#define ALL_MSG_SN_IDX_3 2

#define FINAL_MSG_POLL_TX_TS_IDX_1 10
#define FINAL_MSG_POLL_TX_TS_IDX_2 10

#define FINAL_MSG_RESP_RX_TS_IDX_1 14
#define FINAL_MSG_RESP_RX_TS_IDX_2 14

#define FINAL_MSG_FINAL_TX_TS_IDX_1 18
#define FINAL_MSG_FINAL_TX_TS_IDX_2 18

/* Frame sequence number, incremented after each transmission. */
static uint8_t frame_seq_nb_1 = 0;
static uint8_t frame_seq_nb_2 = 0;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code 
 * is supposed to handle. */
#define RX_BUF_LEN_1 20
static uint8_t rx_buffer_1[RX_BUF_LEN_1];

#define RX_BUF_LEN_2 20
static uint8_t rx_buffer_2[RX_BUF_LEN_2];

/* Hold copy of status register state here for reference so that it can be 
 * examined at a debug breakpoint. */
static uint32_t status_reg_1 = 0;
static uint32_t status_reg_2 = 0;

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from the end of the frame transmission to the enable of 
 * the receiver, as programmed for the DW IC's wait for response feature. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 700

/* This is the delay from Frame RX timestamp to TX reply timestamp used for 
 * calculating/setting the DW IC's delayed TX function. This includes the
 * frame length of approximately 190 us with above configuration. */
#define RESP_RX_TO_FINAL_TX_DLY_UUS 700

/* Receive response timeout. See NOTE 5 below. */
#define RESP_RX_TIMEOUT_UUS 300

/* Preamble timeout, in multiple of PAC size. See NOTE 7 below. */
#define PRE_TIMEOUT 5

/* Hold copies of computed time of flight and distance here for reference so that it can be examined at a debug breakpoint. */
static double tof_1;
static double distance_1;

static double tof_2;
static double distance_2;

/* Time-stamps of frames transmission/reception, expressed in device time units. */
static uint64_t poll_tx_ts_1;
static uint64_t resp_rx_ts_1;
static uint64_t final_tx_ts_1;

static uint64_t poll_tx_ts_2;
static uint64_t resp_rx_ts_2;
static uint64_t final_tx_ts_2;

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and power 
 * of the spectrum at the current temperature. 
 * These values can be calibrated prior to taking reference measurements.
 * See NOTE 8 below. */
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

    /* Display application name on LCD. */
    //LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
    //port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
    /* Target specific drive of RSTn line into DW IC low for a period. */
    //reset_DWIC(); 

    /* Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC */
    Sleep(2);

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) { 
    UART_puts("IDLE FAILED\r\n");
    while (1) ;

    }

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
        UART_puts("INIT FAILED\r\n");      
        //LOG_INF("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* Configure DW IC. See NOTE 2 below. */
    /* if the dwt_configure returns DWT_ERROR either the PLL or RX calibration 
     * has failed the host should reset the device */
    if (dwt_configure(&config)) {
       UART_puts("CONFIG FAILED\r\n");
        //LOG_INF("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);

    /* Apply default antenna delay value. See NOTE 1 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    /* Set expected response's delay and timeout. See NOTE 4, 5 and 7 below.
     * As this example only handles one incoming frame with always the same 
     * delay and timeout, those values can be set here once for all. */
    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);
    dwt_setpreambledetecttimeout(PRE_TIMEOUT);

    /* Next can enable TX/RX states output on GPIOs 5 and 6 to help debug, 
     * and also TX/RX LEDs.
     * Note, in real low power applications the LEDs should not be used. */
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);

    //LOG_INF("Initiator ready");
}

/* Loop forever initiating ranging exchanges. */
void loop(){
  
  /*For 1st Anchor*/
  while(1){
    /* Write frame data to DW IC and prepare transmission. See NOTE 9 below. */
    tx_poll_msg_1[ALL_MSG_SN_IDX_1] = frame_seq_nb_1;
    dwt_writetxdata(sizeof(tx_poll_msg_1), tx_poll_msg_1, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_poll_msg_1)+FCS_LEN, 0, 1); /* Zero offset in TX buffer, ranging. */

    /* Start transmission, indicating that a response is expected so that
     * reception is enabled automatically after the frame is sent and the 
     * delay set by dwt_setrxaftertxdelay() has elapsed. */
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

    /* We assume that the transmission is achieved correctly, poll for 
     * reception of a frame or error/timeout. See NOTE 10 below. */
    while (!((status_reg_1 = dwt_read32bitreg(SYS_STATUS_ID)) & 
             (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | 
              SYS_STATUS_ALL_RX_ERR))){ /* spin */ };

    /* Increment frame sequence number after transmission of the 
     * poll message (modulo 256). */
    frame_seq_nb_1++;

    if (status_reg_1 & SYS_STATUS_RXFCG_BIT_MASK){
      
      /* Clear good RX frame event and TX frame sent in the DW IC status register. */
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_TXFRS_BIT_MASK);

      /* A frame has been received, read it into the local buffer. */
      uint32_t frame_len_1 = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
      
      if (frame_len_1 <= RX_BUF_LEN_1){
        dwt_readrxdata(rx_buffer_1, frame_len_1, 0);
      }

      /* Check that the frame is the expected response from the 
       * companion "DS TWR responder" example.
       * As the sequence number field of the frame is not relevant, 
       * it is cleared to simplify the validation of the frame. */
      rx_buffer_1[ALL_MSG_SN_IDX_1] = 0;
    
      if (memcmp(rx_buffer_1, rx_resp_msg_1, ALL_MSG_COMMON_LEN_1) == 0){
        uint32_t final_tx_time_1;

        /* Retrieve poll transmission and response reception timestamp. */
        poll_tx_ts_1 = get_tx_timestamp_u64();
        resp_rx_ts_1 = get_rx_timestamp_u64();
       
        /* Compute final message transmission time. See NOTE 11 below. */
        final_tx_time_1 = (resp_rx_ts_1 + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(final_tx_time_1);

        /* Final TX timestamp is the transmission time we programmed 
         * plus the TX antenna delay. */
        final_tx_ts_1 = (((uint64_t)(final_tx_time_1 & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;
                
        /* Write all timestamps in the final message. See NOTE 12 below. */
        final_msg_set_ts(&tx_final_msg_1[FINAL_MSG_POLL_TX_TS_IDX_1], poll_tx_ts_1);
        final_msg_set_ts(&tx_final_msg_1[FINAL_MSG_RESP_RX_TS_IDX_1], resp_rx_ts_1);
        final_msg_set_ts(&tx_final_msg_1[FINAL_MSG_FINAL_TX_TS_IDX_1], final_tx_ts_1);

        /* Write and send final message. See NOTE 9 below. */
        tx_final_msg_1[ALL_MSG_SN_IDX_1] = frame_seq_nb_1;
        dwt_writetxdata(sizeof(tx_final_msg_1), tx_final_msg_1, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_final_msg_1)+FCS_LEN, 0, 1); /* Zero offset in TX buffer, ranging bit set. */
          
        /* If dwt_starttx() returns an error, abandon this ranging exchange and
         * proceed to the next one. See NOTE 13 below. */
        int ret_1 = dwt_starttx(DWT_START_TX_DELAYED);
  
        if (ret_1 == DWT_SUCCESS) {
          
          /* Poll DW IC until TX frame sent event set. See NOTE 10 below. */
          while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)){ /* spin */ };

          /* Clear TXFRS event. */
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

          /* Increment frame sequence number after transmission of the 
           * final message (modulo 256). */
          frame_seq_nb_1++;
        }
      }
    }
    else{
      /* Clear RX error/timeout events in the DW IC status register. */
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_TXFRS_BIT_MASK);
    }
  }

  /*For 2nd Achor*/

  /* Write frame data to DW IC and prepare transmission. See NOTE 9 below. */
  while(1){
    tx_poll_msg_2[ALL_MSG_SN_IDX_2] = frame_seq_nb_2;
    dwt_writetxdata(sizeof(tx_poll_msg_2), tx_poll_msg_2, 0); /* Zero offset in TX buffer. */
    dwt_writetxfctrl(sizeof(tx_poll_msg_2)+FCS_LEN, 0, 1); /* Zero offset in TX buffer, ranging. */

    /* Start transmission, indicating that a response is expected so that
     * reception is enabled automatically after the frame is sent and the 
     * delay set by dwt_setrxaftertxdelay() has elapsed. */
    dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

    /* We assume that the transmission is achieved correctly, poll for 
     * reception of a frame or error/timeout. See NOTE 10 below. */
    while (!((status_reg_2 = dwt_read32bitreg(SYS_STATUS_ID)) & 
             (SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_ALL_RX_TO | 
              SYS_STATUS_ALL_RX_ERR))){ /* spin */ };

    /* Increment frame sequence number after transmission of the 
     * poll message (modulo 256). */
    frame_seq_nb_2++;

    if (status_reg_2 & SYS_STATUS_RXFCG_BIT_MASK){
      
      /* Clear good RX frame event and TX frame sent in the DW IC status register. */
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_TXFRS_BIT_MASK);

      /* A frame has been received, read it into the local buffer. */
      uint32_t frame_len_2 = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
      
      if (frame_len_2 <= RX_BUF_LEN_2){
        dwt_readrxdata(rx_buffer_2, frame_len_2, 0);
      }

      /* Check that the frame is the expected response from the 
       * companion "DS TWR responder" example.
       * As the sequence number field of the frame is not relevant, 
       * it is cleared to simplify the validation of the frame. */
      rx_buffer_2[ALL_MSG_SN_IDX_2] = 0;
    
      if (memcmp(rx_buffer_2, rx_resp_msg_2, ALL_MSG_COMMON_LEN_2) == 0){
        uint32_t final_tx_time_2;

        /* Retrieve poll transmission and response reception timestamp. */
        poll_tx_ts_2 = get_tx_timestamp_u64();
        resp_rx_ts_2 = get_rx_timestamp_u64();
       
        /* Compute final message transmission time. See NOTE 11 below. */
        final_tx_time_2 = (resp_rx_ts_2 + (RESP_RX_TO_FINAL_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(final_tx_time_2);

        /* Final TX timestamp is the transmission time we programmed 
         * plus the TX antenna delay. */
        final_tx_ts_2 = (((uint64_t)(final_tx_time_2 & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;
                
        /* Write all timestamps in the final message. See NOTE 12 below. */
        final_msg_set_ts(&tx_final_msg_2[FINAL_MSG_POLL_TX_TS_IDX_2], poll_tx_ts_2);
        final_msg_set_ts(&tx_final_msg_2[FINAL_MSG_RESP_RX_TS_IDX_2], resp_rx_ts_2);
        final_msg_set_ts(&tx_final_msg_2[FINAL_MSG_FINAL_TX_TS_IDX_2], final_tx_ts_2);

        /* Write and send final message. See NOTE 9 below. */
        tx_final_msg_2[ALL_MSG_SN_IDX_2] = frame_seq_nb_2;
        dwt_writetxdata(sizeof(tx_final_msg_2), tx_final_msg_2, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_final_msg_2)+FCS_LEN, 0, 1); /* Zero offset in TX buffer, ranging bit set. */
          
        /* If dwt_starttx() returns an error, abandon this ranging exchange and
         * proceed to the next one. See NOTE 13 below. */
        int ret_2 = dwt_starttx(DWT_START_TX_DELAYED);
  
        if (ret_2 == DWT_SUCCESS) {
          
          /* Poll DW IC until TX frame sent event set. See NOTE 10 below. */
          while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS_BIT_MASK)){ /* spin */ };

          /* Clear TXFRS event. */
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS_BIT_MASK);

          /* Increment frame sequence number after transmission of the 
           * final message (modulo 256). */
          frame_seq_nb_2++;
        }
      }
    }
    else{
      /* Clear RX error/timeout events in the DW IC status register. */
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR | SYS_STATUS_TXFRS_BIT_MASK);
    }
  }  
  
  /* Execute a delay between ranging exchanges. */
  Sleep(RNG_DELAY_MS);
}