#include "at24c32.h"

const uint8_t WordSize = 8;
const uint8_t PageSize = 32;

bool Init_I2C_AT24Cxx (void)
{
  GPIO_InitTypeDef i2c_ec;
  // I2C1 SCL
  i2c_ec.GPIO_Pin = GPIO_Pin_6;
  i2c_ec.GPIO_Mode = GPIO_Mode_AF_OD;
  i2c_ec.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOB, &i2c_ec);
  // I2C1 SDA
  i2c_ec.GPIO_Pin = GPIO_Pin_7;
  GPIO_Init(GPIOB, &i2c_ec);
  // I2C Transport
  I2C_InitTypeDef i2c_tr;
  i2c_tr.I2C_ClockSpeed = 100000;
  i2c_tr.I2C_Mode = I2C_Mode_I2C;
  i2c_tr.I2C_DutyCycle = I2C_DutyCycle_2;
  i2c_tr.I2C_OwnAddress1 = 0;
  i2c_tr.I2C_Ack = I2C_Ack_Disable;
  i2c_tr.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_Init(I2C1, &i2c_tr);
  // DMA Receive from I2C
  DMA_InitTypeDef dma_i2c_tr;
  dma_i2c_tr.DMA_PeripheralBaseAddr = (int)&I2C1->DR;
  dma_i2c_tr.DMA_MemoryBaseAddr = 0;
  dma_i2c_tr.DMA_DIR = DMA_DIR_PeripheralSRC;
  dma_i2c_tr.DMA_BufferSize = 0;
  dma_i2c_tr.DMA_PeripheralInc = DMA_MemoryInc_Disable;
  dma_i2c_tr.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dma_i2c_tr.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  dma_i2c_tr.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  dma_i2c_tr.DMA_Mode = DMA_Mode_Normal;
  dma_i2c_tr.DMA_Priority = DMA_Priority_High;
  dma_i2c_tr.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel7, &dma_i2c_tr); 
  // DMA Transmit to I2C
  dma_i2c_tr.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_Init(DMA1_Channel6, &dma_i2c_tr);
  
  return true;
}



at24cxx_t AT24Cxx_Init(uint8_t __i2c_addr, uint16_t __capacity)
{
  at24cxx_t at24cxx;
  at24cxx.I2C_address = __i2c_addr;
  at24cxx.CapacityWords = __capacity;
  at24cxx.Lock = 0;
  return at24cxx;
}





void AT24Cxx_Read(at24cxx_t __at24cxx, uint16_t __address, uint16_t __words, uint8_t* __buffer)
{
  __words = ((__words + __address) > AT24C32_Capacity) ? (AT24C32_Capacity - __address) : __words;
  __at24cxx.Lock = 1;
  I2C_Cmd(I2C1, ENABLE);
  I2C_AcknowledgeConfig(I2C1, ENABLE);
  
  // ---------------------------------------------
  I2C_GenerateSTART(I2C1, ENABLE);
  while (!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_SB));
 
  I2C_Send7bitAddress(I2C1, __at24cxx.I2C_address, I2C_Direction_Transmitter);
  while (!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_ADDR));
  if ((I2C_ReadRegister(I2C1, I2C_Register_SR2) & I2C_SR2_MSL))
  {
    I2C_SendData(I2C1, ((__address >> 8) & 0xff));
    while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));
    I2C_SendData(I2C1, (__address & 0xff));
    while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));
  }
  I2C_GenerateSTOP(I2C1, ENABLE);
  
  Delay(10);  
  
  // ---------------------------------------------
  if (__words)
  {
    DMA_SetCurrDataCounter(DMA1_Channel7, __words);
    DMA1_Channel7->CMAR = (int)__buffer;
    DMA_Cmd(DMA1_Channel7, ENABLE);
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_SB));
   
    I2C_Send7bitAddress(I2C1, __at24cxx.I2C_address, I2C_Direction_Receiver);
    while (!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_ADDR));
    
    if ((I2C_ReadRegister(I2C1, I2C_Register_SR2) & I2C_SR2_MSL))
    {
      I2C_DMACmd(I2C1, ENABLE);
      while (!(DMA_GetFlagStatus(DMA1_FLAG_TC7)));
      DMA_ClearFlag(DMA1_FLAG_TC7);

      I2C_AcknowledgeConfig(I2C1, DISABLE);
      while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_RXNE));
    }
    
    I2C_GenerateSTOP(I2C1, ENABLE);
    DMA_Cmd(DMA1_Channel7, DISABLE);
    DMA1_Channel7->CMAR = 0;
  }
  
  I2C_Cmd(I2C1, DISABLE);
  __at24cxx.Lock = 0;
}





void AT24Cxx_Write(at24cxx_t __at24cxx, uint16_t __address, uint16_t __words, uint8_t* __buffer)
{
  __words = ((__words + __address) > AT24C32_Capacity) ? (AT24C32_Capacity - __address) : __words;
  __at24cxx.Lock = 1;
  uint16_t pages = __words/PageSize;
  uint16_t address = __address & 0xffe0;
  
  I2C_Cmd(I2C1, ENABLE);

  for (int x = 0; x < pages; x++)
  {
    // ---------------------------------------------
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_SB));
   
    I2C_Send7bitAddress(I2C1, __at24cxx.I2C_address, I2C_Direction_Transmitter);
    while (!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_ADDR));
    
    if ((I2C_ReadRegister(I2C1, I2C_Register_SR2) & I2C_SR2_MSL))
    {
      I2C_SendData(I2C1, ((address >> 8) & 0xff));
      while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));
      I2C_SendData(I2C1, (address & 0xff));
      while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));

//      for (int i = 0; i < PageSize; i++)
//      {
//        I2C_SendData(I2C1, __buffer[i + (x*PageSize)]);
//        while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));
//      }
      
      // DMA block. It can be replaced with normal cycle commented above.
      DMA_SetCurrDataCounter(DMA1_Channel6, PageSize);
      DMA1_Channel6->CMAR = (int)&__buffer[x*PageSize];
      DMA_Cmd(DMA1_Channel6, ENABLE);
      
      I2C_DMACmd(I2C1, ENABLE);
      while (!(DMA_GetFlagStatus(DMA1_FLAG_TC6)));
      DMA_ClearFlag(DMA1_FLAG_TC6);
      while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));
      
      I2C_DMACmd(I2C1, DISABLE);
      DMA_Cmd(DMA1_Channel6, DISABLE);
      DMA1_Channel6->CMAR = 0;
      // ------------ End of DMA block -------------
    }
    I2C_GenerateSTOP(I2C1, ENABLE);
    Delay(10);
    address += PageSize;
  }
  
  if (__words%PageSize)
  {
    address = (address != __address) ? __address &= 0xffe0 : __address;
    // ---------------------------------------------
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_SB));
   
    I2C_Send7bitAddress(I2C1, __at24cxx.I2C_address, I2C_Direction_Transmitter);
    while (!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_ADDR));
    
    if ((I2C_ReadRegister(I2C1, I2C_Register_SR2) & I2C_SR2_MSL))
    {
      I2C_SendData(I2C1, (((address + pages*PageSize) >> 8) & 0xff));
      while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));
      I2C_SendData(I2C1, ((address + pages*PageSize) & 0xff));
      while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));

//      for (int i = 0; i < __words%PageSize; i++)
//      {
//        I2C_SendData(I2C1, __buffer[i + (pages*PageSize)]);
//        while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));
//      }

      // DMA block. It can be replaced with normal cycle commented above.
      DMA_SetCurrDataCounter(DMA1_Channel6, __words%PageSize);
      DMA1_Channel6->CMAR = (int)&__buffer[pages*PageSize];
      DMA_Cmd(DMA1_Channel6, ENABLE);
      
      I2C_DMACmd(I2C1, ENABLE);
      while (!(DMA_GetFlagStatus(DMA1_FLAG_TC6)));
      DMA_ClearFlag(DMA1_FLAG_TC6);
      while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & I2C_SR1_TXE));
      
      I2C_DMACmd(I2C1, DISABLE);
      DMA_Cmd(DMA1_Channel6, DISABLE);
      DMA1_Channel6->CMAR = 0;
      // ------------ End of DMA block -------------
    }
    I2C_GenerateSTOP(I2C1, ENABLE);
    Delay(10);
  }
  
  I2C_Cmd(I2C1, DISABLE);
  __at24cxx.Lock = 0;
}
