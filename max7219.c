#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "nwk_pll.h"
#include "bsp_leds.h"
#include "max7219.h"

typedef enum {
  REG_NO_OP 		= 0x00 << 8,
  REG_DIGIT_0 		= 0x01 << 8,
  REG_DIGIT_1 		= 0x02 << 8,
  REG_DIGIT_2 		= 0x03 << 8,
  REG_DIGIT_3 		= 0x04 << 8,
  REG_DIGIT_4 		= 0x05 << 8,
  REG_DIGIT_5 		= 0x06 << 8,
  REG_DIGIT_6 		= 0x07 << 8,
  REG_DIGIT_7 		= 0x08 << 8,
  REG_DECODE_MODE 	= 0x09 << 8,
  REG_INTENSITY 	= 0x0A << 8,
  REG_SCAN_LIMIT 	= 0x0B << 8,
  REG_SHUTDOWN 		= 0x0C << 8,
  REG_DISPLAY_TEST 	= 0x0F << 8,
} MAX7219_REGISTERS;

static void max7219_clear(void);
static void sendData(uint16_t data);

static uint8_t framebuffer[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

///////////////////////////////////////////////////////////////////////////////
// 16bit SPI transfer emulation via GPIO
///////////////////////////////////////////////////////////////////////////////

#define DISP_PORT       P1
#define DISP_DDR        P1DIR
#define DISP_DOUT       6
#define DISP_CLK        5
#define DISP_CS         4

#define DISP_CS_HIGH()  DISP_PORT |= BV(DISP_CS);
#define DISP_CS_LOW()   DISP_PORT &= ~BV(DISP_CS);

#define DISP_CLK_HIGH() DISP_PORT |= BV(DISP_CLK);
#define DISP_CLK_LOW()  DISP_PORT &= ~BV(DISP_CLK);

#define DISP_DOUT_HIGH() DISP_PORT |= BV(DISP_DOUT);
#define DISP_DOUT_LOW() DISP_PORT &= ~BV(DISP_DOUT);

#define DISP_CONFIG()   DISP_DDR |= (BV(DISP_CS) | BV(DISP_CLK) | BV(DISP_DOUT));

static void sendData(uint16_t data)
{
  DISP_CS_LOW();

  int i;
  for (i = 0; i < 16; i++) {
    if (data & 0x8000) {
      DISP_DOUT_HIGH();
    } else {
      DISP_DOUT_LOW();
    }
    DISP_CLK_HIGH();
    DISP_CLK_LOW();
    data <<= 1;
  }

  DISP_CS_HIGH();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void max7219_init(uint8_t intensivity)
{
  DISP_CONFIG();
  NWK_DELAY(80);
  max7219_setIntensivity(intensivity);
  max7219_clear();
  max7219_display(framebuffer);
}

void max7219_setIntensivity(uint8_t intensivity)
{
  if (intensivity > 0x0F) {
    return;
  }
  sendData(REG_SHUTDOWN | 0x01);
  sendData(REG_DECODE_MODE | 0x00);
  sendData(REG_SCAN_LIMIT | 0x07);
  sendData(REG_INTENSITY | intensivity);
}

static void max7219_clear()
{
  sendData(REG_DIGIT_0 | 0x00);
  sendData(REG_DIGIT_1 | 0x00);
  sendData(REG_DIGIT_2 | 0x00);
  sendData(REG_DIGIT_3 | 0x00);
  sendData(REG_DIGIT_4 | 0x00);
  sendData(REG_DIGIT_5 | 0x00);
  sendData(REG_DIGIT_6 | 0x00);
  sendData(REG_DIGIT_7 | 0x00);
}

void max7219_display(uint8_t* array)
{
  sendData(REG_SHUTDOWN | 0x01);
  sendData(REG_DECODE_MODE | 0x00);
  sendData(REG_SCAN_LIMIT | 0x07);

  sendData(REG_DIGIT_0 | array[0]);
  sendData(REG_DIGIT_1 | array[1]);
  sendData(REG_DIGIT_2 | array[2]);
  sendData(REG_DIGIT_3 | array[3]);
  sendData(REG_DIGIT_4 | array[4]);
  sendData(REG_DIGIT_5 | array[5]);
  sendData(REG_DIGIT_6 | array[6]);
  sendData(REG_DIGIT_7 | array[7]);
}
