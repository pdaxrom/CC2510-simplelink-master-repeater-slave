#include "bsp.h"
#include "mrfi.h"
#include "bsp_leds.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "nwk_pll.h"

#define SPIN_ABOUT_A_SECOND   NWK_DELAY(1000)

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

  while (SMPL_SUCCESS != SMPL_Init((uint8_t (*)(linkID_t))0))
  {
    BSP_TOGGLE_LED1();
    SPIN_ABOUT_A_SECOND;
  }

  BSP_TURN_OFF_LED1();

  while (1)
    FHSS_ACTIVE( nwk_pllBackgrounder( false ) ); /* manage FHSS */
}
