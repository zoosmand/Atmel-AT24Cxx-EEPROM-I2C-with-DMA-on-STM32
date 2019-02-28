#ifndef _AT24C32_H
#define _AT24C32_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

#define AT24C32_Capacity  0x1000
#define AT24C64_Capacity  0x2000

typedef struct
{
  uint8_t   I2C_address;
  uint16_t  CapacityWords;
  uint8_t   Lock;
} at24cxx_t;  


bool Init_I2C_AT24Cxx(void);
at24cxx_t AT24Cxx_Init(uint8_t, uint16_t);

void AT24Cxx_Read(at24cxx_t, uint16_t, uint16_t, uint8_t*);
void AT24Cxx_Write(at24cxx_t, uint16_t, uint16_t, uint8_t*);

#ifdef __cplusplus
}
#endif

#endif
