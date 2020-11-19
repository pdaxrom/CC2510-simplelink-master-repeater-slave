#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "nwk_pll.h"

#ifndef APP_AUTO_ACK
#error ERROR: Must define the macro APP_AUTO_ACK for this application.
#endif

void toggleLED(uint8_t);

static void linkTo(void);

static uint8_t  sTid = 0;
static linkID_t sLinkID1 = 0;

#define SPIN_ABOUT_A_SECOND   NWK_DELAY(1000)
#define SPIN_ABOUT_A_HALF_SECOND   NWK_DELAY(500)
#define SPIN_ABOUT_A_QUARTER_SECOND   NWK_DELAY(250)

/* How many times to try a Tx and miss an acknowledge before doing a scan */
#define MISSES_IN_A_ROW  2

void main (void)
{
  BSP_Init();

#ifdef I_WANT_TO_CHANGE_DEFAULT_ROM_DEVICE_ADDRESS_PSEUDO_CODE
  {
    addr_t lAddr;

    createRandomAddress(&lAddr);
    SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &lAddr);
  }
#endif /* I_WANT_TO_CHANGE_DEFAULT_ROM_DEVICE_ADDRESS_PSEUDO_CODE */

  while (SMPL_SUCCESS != SMPL_Init(0)) {
    BSP_TOGGLE_LED1();
    SPIN_ABOUT_A_SECOND; /* calls nwk_pllBackgrounder for us */
  }

  BSP_TURN_OFF_LED1();
  
  linkTo();

  while (1) {
    FHSS_ACTIVE( nwk_pllBackgrounder( false ) );
  }
}

static void linkTo()
{
  uint8_t     msg[2];
  uint8_t     event, misses, done;

  while (SMPL_SUCCESS != SMPL_Link(&sLinkID1)) {
    BSP_TOGGLE_LED1();
    SPIN_ABOUT_A_SECOND; /* calls nwk_pllBackgrounder for us */
  }

  BSP_TURN_OFF_LED1();

#ifndef FREQUENCY_HOPPING
  SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);
#endif

  while (1) {
    FHSS_ACTIVE( nwk_pllBackgrounder( false ) );

///////////////////////////////////////////////////////////////////////////////
// FIXME: STUB code for external events
///////////////////////////////////////////////////////////////////////////////    
    event = 1;
    SPIN_ABOUT_A_SECOND; /* delay before event */
    SPIN_ABOUT_A_SECOND; /* delay before event */
    SPIN_ABOUT_A_SECOND; /* delay before event */
///////////////////////////////////////////////////////////////////////////////
    
    if (event) {
      uint8_t      noAck;
      smplStatus_t rc;

#ifndef FREQUENCY_HOPPING
      SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_AWAKE, 0);
#endif

///////////////////////////////////////////////////////////////////////////////
// FIXME: STUB code for message (2 bytes)
///////////////////////////////////////////////////////////////////////////////    
      msg[1] = ++sTid;
      msg[0] = event;
///////////////////////////////////////////////////////////////////////////////
      done = 0;
      while (!done) {
        noAck = 0;

        for (misses=0; misses < MISSES_IN_A_ROW; ++misses) {
          if (SMPL_SUCCESS == (rc=SMPL_SendOpt(sLinkID1, msg, sizeof(msg), SMPL_TXOPTION_ACKREQ))) {
            BSP_TOGGLE_LED1();
            SPIN_ABOUT_A_HALF_SECOND;
            BSP_TOGGLE_LED1();
            break;
          } if (SMPL_NO_ACK == rc) {
            noAck++;
          }
        }
        if (MISSES_IN_A_ROW == noAck) {
          BSP_TOGGLE_LED1();
          SPIN_ABOUT_A_QUARTER_SECOND;
          BSP_TOGGLE_LED1();
#ifdef FREQUENCY_AGILITY
          if (SMPL_SUCCESS != SMPL_Ping(sLinkID1)) {
            done = 1;
          }
#else
          done = 1;
#endif  /* FREQUENCY_AGILITY */
        } else {
          done = 1;
        }
      }

#ifndef FREQUENCY_HOPPING
      SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);
#endif
    }
  }
}
