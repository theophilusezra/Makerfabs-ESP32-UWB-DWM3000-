/*Library for DW3000*/
#include "dw3000.h"

/*Define the device*/
#define APP_NAME "DS TWR RESP_3 v1.0"

/*connection pins*/
/*Reset PIN*/
const uint8_t PIN_RST = 27;  

/*Interrupt Request PIN*/
const uint8_t PIN_IRQ = 34; 

/*spi select pin*/
const uint8_t PIN_SS = 4; 
 
/*Default communication configuration. We use default non-STS DW mode*/
static dwt_config_t config = {
  /*Channel number*/
  5,

  /*Preamble length. Used in TX only*/
  DWT_PLEN_128,

  /*Preamble acquisition chunk size. Used in RX only*/
  DWT_PAC8,

  /*TX preamble code. Used in TX only*/
  9,

  /*RX preamble code. Used in RX only*/
  9,

  /*0 to use standard 8 symbol SFD, 1 to use non-standard 8 symbol,
    2 for non-standard 16 symbol SFD and 3 for 4z 8 symbol SDF type*/
  1,

  /*Data rate*/
  DWT_BR_6M8,
  
  /*PHY header mode*/
  DWT_PHRMODE_STD,

  /*PHY header rate*/
  DWT_PHRRATE_STD,
  
  /*SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only*/
  (128 + 1 + 8 - 8),
  
  /*STS disabled*/
  DWT_STS_MODE_OFF,
  
  /*STS length see allowed values in Enum dwt_sts_lengths_e*/
  DWT_STS_LEN_64,

  /*PDOA mode off*/
  DWT_PDOA_M0
};

/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 100

/* Default antenna delay values for 64 MHz PRF. See NOTE 1 below. */
#define TX_ANT_DLY 0
#define RX_ANT_DLY 0 //16385

/* Frames used in the ranging process. See NOTE 2 below. */
/*For 3rd Anchor*/
static uint8_t rx_poll_msg_3[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'S', 'N', 'Y', 'L', 0x21, 0, 0};
static uint8_t tx_resp_msg_3[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'Y', 'L', 'S', 'N', 0x10, 0x02, 0, 0, 0, 0};
static uint8_t rx_final_msg_3[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'S', 'N', 'Y', 'L', 0x23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Length of the common part of the message (up to and including the
 * function code, see NOTE 2 below). */
#define ALL_MSG_COMMON_LEN_3 10

/* Index to access some of the fields in the frames involved in the process. */
#define ALL_MSG_SN_IDX_3 2
#define FINAL_MSG_POLL_TX_TS_IDX_3 10
#define FINAL_MSG_RESP_RX_TS_IDX_3 14
#define FINAL_MSG_FINAL_TX_TS_IDX_3 18

/* Frame sequence number, incremented after each transmission. */
static uint8_t frame_seq_nb_3 = 0;

/* Buffer to store received messages.
 * Its size is adjusted to longest frame that this example code is
 * supposed to handle. */
#define RX_BUF_LEN_3 24
static uint8_t rx_buffer_3[RX_BUF_LEN_3];

/* Hold copy of status register state here for reference so that it can be
 * examined at a debug breakpoint. */
static uint32_t status_reg_3 = 0;

/* Delay between frames, in UWB microseconds. See NOTE 4 below. */
/* This is the delay from Frame RX timestamp to TX reply timestamp used for
 * calculating/setting the DW IC's delayed TX function. This includes the
 * frame length of approximately 190 us with above configuration. */
#define POLL_RX_TO_RESP_TX_DLY_UUS 900

/* This is the delay from the end of the frame transmission to the enable of
 * the receiver, as programmed for the DW IC's wait for response feature. */
#define RESP_TX_TO_FINAL_RX_DLY_UUS 500

/* Receive final timeout. See NOTE 5 below. */
#define FINAL_RX_TIMEOUT_UUS 220

/* Preamble timeout, in multiple of PAC size. See NOTE 6 below. */
#define PRE_TIMEOUT 5

/* Timestamps of frames transmission/reception. */
static uint64_t poll_rx_ts_3;
static uint64_t resp_tx_ts_3;
static uint64_t final_rx_ts_3;

/* Hold copies of computed time of flight and distance here for reference
 * so that it can be examined at a debug breakpoint. */
static double tof_3;
static double distance_3;

/* Values for the PG_DELAY and TX_POWER registers reflect the bandwidth and
 * power of the spectrum at the current temperature. These values can be
 * calibrated prior to taking reference measurements.
 * See NOTE 2 below. */
extern dwt_txconfig_t txconfig_options;

/*! ---------------------------------------------------------------------------
 * @fn ds_twr_responder()
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
   // LOG_INF(APP_NAME);

    /* Configure SPI rate, DW3000 supports up to 38 MHz */
   // port_set_dw_ic_spi_fastrate();

    /* Reset DW IC */
   // reset_DWIC();

    Sleep(2);

    /* Need to make sure DW IC is in IDLE_RC before proceeding */
    while (!dwt_checkidlerc()) {
          UART_puts("IDLE FAILED\r\n");
    while (1) ;

     };

    if (dwt_initialise(DWT_DW_INIT) == DWT_ERROR) {
       // LOG_ERR("INIT FAILED");
        while (1) { /* spin */ };
    }

    /* Configure DW IC. See NOTE 15 below. */
    if (dwt_configure(&config)) {
       // LOG_ERR("CONFIG FAILED");
        while (1) { /* spin */ };
    }

    /* Configure the TX spectrum parameters (power, PG delay and PG count) */
    dwt_configuretxrf(&txconfig_options);

    /* Apply default antenna delay value. See NOTE 1 below. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    /* Next can enable TX/RX states output on GPIOs 5 and 6 to help debug,
     * and also TX/RX LEDs.
     * Note, in real low power applications the LEDs should not be used.
     */
    dwt_setlnapamode(DWT_LNA_ENABLE | DWT_PA_ENABLE);

    dwt_setleds(DWT_LEDS_ENABLE | DWT_LEDS_INIT_BLINK);
}    

/* Loop forever responding to ranging requests. */
void loop (){
  while (1){
    dwt_setpreambledetecttimeout(0);

    /* Clear reception timeout to start next ranging process. */
    dwt_setrxtimeout(0);

    /* Activate reception immediately. */
    dwt_rxenable(DWT_START_RX_IMMEDIATE);

    /* Poll for reception of a frame or error/timeout. See NOTE 8 below. */
    while (!((status_reg_3 = dwt_read32bitreg(SYS_STATUS_ID)) 
          & (SYS_STATUS_RXFCG_BIT_MASK |SYS_STATUS_ALL_RX_TO 
            |SYS_STATUS_ALL_RX_ERR))){ /* spin */ };

    if (status_reg_3 & SYS_STATUS_RXFCG_BIT_MASK){
      
      /* Clear good RX frame event in the DW IC status register. */
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK);

      /* A frame has been received, read it into the local buffer. */
      uint32_t frame_len = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
      
      if (frame_len <= RX_BUF_LEN_3){
        dwt_readrxdata(rx_buffer_3, frame_len, 0);
      }

      /* Check that the frame is a poll sent by "DS TWR initiator" example.
       * As the sequence number field of the frame is not relevant, it
       * is cleared to simplify the validation of the frame.
       */
      rx_buffer_3[ALL_MSG_SN_IDX_3] = 0;
      if (memcmp(rx_buffer_3, rx_poll_msg_3, ALL_MSG_COMMON_LEN_3) == 0){
        uint32_t resp_tx_time_3;

        /* Retrieve poll reception timestamp. */
        poll_rx_ts_3 = get_rx_timestamp_u64();

        /* Set send time for response. See NOTE 9 below. */
        resp_tx_time_3 = (poll_rx_ts_3 + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(resp_tx_time_3);

        /* Set expected delay and timeout for final message reception. See NOTE 4 and 5 below. */
        dwt_setrxaftertxdelay(RESP_TX_TO_FINAL_RX_DLY_UUS);
        dwt_setrxtimeout(FINAL_RX_TIMEOUT_UUS);

        /* Set preamble timeout for expected frames. See NOTE 6 below. */
        dwt_setpreambledetecttimeout(PRE_TIMEOUT);

        /* Write and send the response message. See NOTE 10 below.*/
        tx_resp_msg_3[ALL_MSG_SN_IDX_3] = frame_seq_nb_3;
        dwt_writetxdata(sizeof(tx_resp_msg_3), tx_resp_msg_3, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(sizeof(tx_resp_msg_3), 0, 1); /* Zero offset in TX buffer, ranging. */
        int ret = dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);

        /* If dwt_starttx() returns an error, abandon this ranging
         * exchange and proceed to the next one. See NOTE 11 below. */
        if (ret == DWT_ERROR){
          continue;
        }

        /* Poll for reception of expected "final" frame or error/timeout.
         * See NOTE 8 below.
         */
        while (!((status_reg_3 = dwt_read32bitreg(SYS_STATUS_ID)) &
                 (SYS_STATUS_RXFCG_BIT_MASK |SYS_STATUS_ALL_RX_TO |
                  SYS_STATUS_ALL_RX_ERR))){ /* spin */ };
                          
        /* Increment frame sequence number after transmission of the
         * response message (modulo 256).
         */
        frame_seq_nb_3++;

        if (status_reg_3 & SYS_STATUS_RXFCG_BIT_MASK) {
          
          /* Clear good RX frame event and TX frame sent in the DW IC status register. */
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG_BIT_MASK | SYS_STATUS_TXFRS_BIT_MASK);

          /* A frame has been received, read it into the local buffer. */
          frame_len = dwt_read32bitreg(RX_FINFO_ID) & FRAME_LEN_MAX_EX;
          if (frame_len <= RX_BUF_LEN_3){
            dwt_readrxdata(rx_buffer_3, frame_len, 0);
          }

          /* Check that the frame is a final message sent by
           * "DS TWR initiator" example.
           * As the sequence number field of the frame is not used in
           * this example, it can be zeroed to ease the validation of
           * the frame.
           */
          rx_buffer_3[ALL_MSG_SN_IDX_3] = 0;
          if (memcmp(rx_buffer_3, rx_final_msg_3, ALL_MSG_COMMON_LEN_3) == 0){
            uint32_t poll_tx_ts_3, resp_rx_ts_3, final_tx_ts_3;
            uint32_t poll_rx_ts_3, resp_tx_ts_3, final_rx_ts_3;
            double Ra_3, Rb_3, Da_3, Db_3;
            int64_t tof_dtu_3;

            /* Retrieve response transmission and final
             * reception timestamps. */
            resp_tx_ts_3 = get_tx_timestamp_u64();
            final_rx_ts_3 = get_rx_timestamp_u64();

            /* Get timestamps embedded in the final message. */
            final_msg_get_ts(&rx_buffer_3[FINAL_MSG_POLL_TX_TS_IDX_3], &poll_tx_ts_3);
            final_msg_get_ts(&rx_buffer_3[FINAL_MSG_RESP_RX_TS_IDX_3], &resp_rx_ts_3);
            final_msg_get_ts(&rx_buffer_3[FINAL_MSG_FINAL_TX_TS_IDX_3], &final_tx_ts_3);

            /* Compute time of flight. 32-bit subtractions give
             * correct answers even if clock has wrapped.
             * See NOTE 12 below. */
            poll_rx_ts_3 = (uint32_t)poll_rx_ts_3;
            resp_tx_ts_3 = (uint32_t)resp_tx_ts_3;
            final_rx_ts_3= (uint32_t)final_rx_ts_3;
            Ra_3 = (double)(resp_rx_ts_3 - poll_tx_ts_3);
            Rb_3 = (double)(final_rx_ts_3 - resp_tx_ts_3);
            Da_3 = (double)(final_tx_ts_3 - resp_rx_ts_3);
            Db_3 = (double)(resp_tx_ts_3 - poll_rx_ts_3);
            tof_dtu_3 = (int64_t)((Ra_3 * Rb_3 - Da_3 * Db_3) / (Ra_3 + Rb_3 + Da_3 + Db_3));

            tof_3 = tof_dtu_3 * DWT_TIME_UNITS;
            distance_3 = tof_3 * SPEED_OF_LIGHT;

            /* Display computed distance. */
            // char dist[20] = {0};
            // snprintf(dist_str, sizeof(dist_str), "distance 1: %3.2f m ", distance_2);
            // test_run_info((unsigned char *)dist_str);
                    
            /*LowerLimit Reading UpperLimit*/
            Serial.println("0 "+String(distance_3*100)+" 200"); 
            // LOG_INF("%s", log_strdup(dist));

            /* As DS-TWR initiator is waiting for RNG_DELAY_MS
             * before next poll transmission we can add a delay
             * here before RX is re-enabled again.
             */
            Sleep(RNG_DELAY_MS - 10);  // start couple of ms earlier
          }
        }
        else {
          /* Clear RX error/timeout events in the DW IC
           * status register. */
          dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
        }
                
      }
    }
    else{
      /* Clear RX error/timeout events in the DW IC status register. */
      dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);
    }
  }
}
