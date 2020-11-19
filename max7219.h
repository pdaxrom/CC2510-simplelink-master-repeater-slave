#ifndef __MAX7219_H__
#define __MAX7219_H__

void max7219_init(uint8_t intensivity);
void max7219_setIntensivity(uint8_t intensivity);
void max7219_display(uint8_t* array);

#endif /* __MAX7219_H__ */
