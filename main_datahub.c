#include <string.h>
#include "bsp.h"
#include "mrfi.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "nwk_frame.h"
#include "nwk.h"
#include "nwk_pll.h"

#include "max7219.h"

#ifndef APP_AUTO_ACK
#error ERROR: Must define the macro APP_AUTO_ACK for this application.
#endif

/* reserve space for the maximum possible peer Link IDs */
static linkID_t sLID[NUM_CONNECTIONS] = {0};
static uint8_t  sNumCurrentPeers = 0;

/* callback handler */
static uint8_t sCB(linkID_t);

/* received message handler */
static void processMessage(linkID_t, uint8_t *, uint8_t);

/* Frequency Agility helper functions */
static void    checkChangeChannel(void);
static void    changeChannel(void);

/* work loop semaphores */
static volatile uint8_t sPeerFrameSem = 0;
static volatile uint8_t sJoinSem = 0;

#ifdef FREQUENCY_AGILITY
/*     ************** BEGIN interference detection support */

#define INTERFERNCE_THRESHOLD_DBM (-70)
#define SSIZE    25
#define IN_A_ROW  3
static int8_t  sSample[SSIZE];
static uint8_t sChannel = 0;
#endif  /* FREQUENCY_AGILITY */

/* blink LEDs when channel changes... */
static volatile uint16_t sBlinky = 1;

static volatile uint8_t disp_anim_frame = 0;
static const uint8_t disp_anim[4][8] = {
  { 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x24, 0x3c, 0x3c, 0x24, 0x00, 0x00 },
  { 0x00, 0x42, 0x42, 0x5a, 0x5a, 0x42, 0x42, 0x00 },
  { 0x81, 0x81, 0x81, 0x99, 0x99, 0x81, 0x81, 0x81 }
};

static const uint8_t disp_num[10][8] = {
  { 0x00, 0x00, 0x3e, 0x51, 0x49, 0x45, 0x3e, 0x00 },
  { 0x00, 0x00, 0x00, 0x42, 0x7f, 0x40, 0x00, 0x00 },
  { 0x00, 0x00, 0x42, 0x61, 0x51, 0x49, 0x46, 0x00 },
  { 0x00, 0x00, 0x21, 0x41, 0x45, 0x4b, 0x31, 0x00 },
  { 0x00, 0x00, 0x18, 0x14, 0x12, 0x7f, 0x10, 0x00 },
  { 0x00, 0x00, 0x27, 0x45, 0x45, 0x45, 0x39, 0x00 },
  { 0x00, 0x00, 0x3c, 0x4a, 0x49, 0x49, 0x30, 0x00 },
  { 0x00, 0x00, 0x01, 0x71, 0x09, 0x05, 0x03, 0x00 },
  { 0x00, 0x00, 0x36, 0x49, 0x49, 0x49, 0x36, 0x00 },
  { 0x00, 0x00, 0x06, 0x49, 0x49, 0x29, 0x1e, 0x00 }
};

/*     ************** END interference detection support                       */

#define SPIN_ABOUT_A_QUARTER_SECOND   NWK_DELAY(250)

void main (void)
{
  bspIState_t intState;

#ifdef FREQUENCY_AGILITY
  memset(sSample, 0x0, sizeof(sSample));
#endif
  
  BSP_Init();

  max7219_init(10);
  
#ifdef I_WANT_TO_CHANGE_DEFAULT_ROM_DEVICE_ADDRESS_PSEUDO_CODE
  {
    addr_t lAddr;

    createRandomAddress(&lAddr);
    SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &lAddr);
  }
#endif /* I_WANT_TO_CHANGE_DEFAULT_ROM_DEVICE_ADDRESS_PSEUDO_CODE */

  SMPL_Init(sCB);

  BSP_TURN_ON_LED1();
  
  /* main work loop */
  while (1) {
    FHSS_ACTIVE( nwk_pllBackgrounder( false ) );
    
    if (sJoinSem && (sNumCurrentPeers < NUM_CONNECTIONS)) {
      while (1) {
        if (SMPL_SUCCESS == SMPL_LinkListen(&sLID[sNumCurrentPeers])) {
          break;
        }
        /* Implement fail-to-link policy here. otherwise, listen again. */
      }

      sNumCurrentPeers++;

      BSP_ENTER_CRITICAL_SECTION(intState);
      sJoinSem--;
      BSP_EXIT_CRITICAL_SECTION(intState);
    }

    /* Have we received a frame on one of the ED connections?
     * No critical section -- it doesn't really matter much if we miss a poll
     */
    if (sPeerFrameSem) {
      uint8_t     msg[MAX_APP_PAYLOAD], len, i;

      /* process all frames waiting */
      for (i=0; i<sNumCurrentPeers; ++i) {
        if (SMPL_SUCCESS == SMPL_Receive(sLID[i], msg, &len)) {
          processMessage(sLID[i], msg, len);

          BSP_ENTER_CRITICAL_SECTION(intState);
          sPeerFrameSem--;
          BSP_EXIT_CRITICAL_SECTION(intState);
        }
      }
    }
    if (BSP_BUTTON1()) {
      SPIN_ABOUT_A_QUARTER_SECOND;  /* debounce */
      changeChannel();
    } else {
      checkChangeChannel();
    }
    BSP_ENTER_CRITICAL_SECTION(intState);
//    if (sBlinky) {
      if (sBlinky++ >= 0x3000) {
        sBlinky = 1;
        BSP_TOGGLE_LED1();
        max7219_display((uint8_t *)&disp_anim[disp_anim_frame++][0]);
        disp_anim_frame %= 4;
      }
//    }
    BSP_EXIT_CRITICAL_SECTION(intState);
  }
}

/* Runs in ISR context. Reading the frame should be done in the */
/* application thread not in the ISR thread. */
static uint8_t sCB(linkID_t lid)
{
  if (lid) {
    sPeerFrameSem++;
    sBlinky = 0;
  } else {
    sJoinSem++;
  }

  /* leave frame to be read by application. */
  return 0;
}

static void processMessage(linkID_t lid, uint8_t *msg, uint8_t len)
{
  /* do something useful */
  if (len) {
///////////////////////////////////////////////////////////////////////////////
// FIXME: STUB
///////////////////////////////////////////////////////////////////////////////
    max7219_display((uint8_t *) &disp_num[msg[0] & 0x7][0]);
    NWK_DELAY(500);
    max7219_display((uint8_t *) &disp_num[msg[1] & 0x7][0]);
    NWK_DELAY(500);
///////////////////////////////////////////////////////////////////////////////
  }
}

static void changeChannel(void)
{
#ifdef FREQUENCY_AGILITY
  freqEntry_t freq;

  if (++sChannel >= NWK_FREQ_TBL_SIZE) {
    sChannel = 0;
  }
  freq.logicalChan = sChannel;
  SMPL_Ioctl(IOCTL_OBJ_FREQ, IOCTL_ACT_SET, &freq);
//  BSP_TURN_OFF_LED1();
//  sBlinky = 1;
#endif
}

/* implement auto-channel-change policy here... */
static void  checkChangeChannel(void)
{
#ifdef FREQUENCY_AGILITY
  int8_t dbm, inARow = 0;

  uint8_t i;

  memset(sSample, 0x0, SSIZE);
  for (i=0; i<SSIZE; ++i) {
    /* quit if we need to service an app frame */
    if (sPeerFrameSem || sJoinSem) {
      return;
    }
    NWK_DELAY(1);
    SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_RSSI, (void *)&dbm);
    sSample[i] = dbm;

    if (dbm > INTERFERNCE_THRESHOLD_DBM) {
      if (++inARow == IN_A_ROW) {
        changeChannel();
        break;
      }
    } else {
      inARow = 0;
    }
  }
#endif
}
