
#include "main.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
uint32_t sysQuantum = 0;
uint32_t millis = 0;
uint32_t seconds = 0;
uint32_t minutes = 0;
uint32_t _EREG_ = 0;

uint32_t millis_tmp = 100;
uint32_t seconds_tmp = 1000;
uint32_t minutes_tmp = 60;

at24cxx_t at24cxx;

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Init_SysTick(void);
void Init_PeripheralClock(void);
void Init_NVIC(void);
void Init_IWDG(void);
void Init_GPIO(void);
void Init_TIM7(void);
void Init_RTC(void);
void Init_RTC_Alarm(uint32_t);
void Init(void);

// *************************************************************
void SysQuantum_Handler(void);
void Millis_Handler(void);
void Seconds_Handler(void);
void Minutes_Handler(void);
void Cron_Handler(void);

// *************************************************************
void BasicTimer_IT_Handler(TIM_TypeDef*);
void RTC_Alarm_Handler(void);
void Flags_Handler(void);

// *************************************************************
void Toggle_LED(void);
void SetRTCAlarm(uint32_t);


int main(void)
{
// =============================================================
// -------------------------------------------------------------  
  Init();
  
  
  //SetRTCAlarm(15);

 
// ------------------------- Main Loop -------------------------  
// -------------------------------------------------------------  
  while(1)
  {
    Delay_Handler();
    
    Cron_Handler();

    Flags_Handler();
	}
// -------------------------------------------------------------  
}








//////////////////////////// INIT ////////////////////////////// 
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Init_SysTick(void)
{
  SysTick->LOAD = 720U - 1U;
  SysTick->CTRL |= SysTick_CTRL_ENABLE | SysTick_CTRL_TICKINT | SysTick_CTRL_CLKSOURCE;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Init_PeripheralClock(void)
{
  FLASH_SetLatency(FLASH_ACR_LATENCY_2);
  FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
  
  RCC_AHBPeriphClockCmd((
      RCC_AHBPeriph_DMA1
    | RCC_AHBPeriph_CRC
  ), ENABLE);
  
  RCC_APB1PeriphClockCmd((
      RCC_APB1Periph_PWR
    | RCC_APB1Periph_TIM7
    | RCC_APB1Periph_I2C1
  ), ENABLE);
  
  RCC_APB2PeriphClockCmd((
      RCC_APB2Periph_GPIOC
    | RCC_APB2Periph_GPIOB
    | RCC_APB2Periph_AFIO
  ), ENABLE);
  
  DBGMCU_Config((
      DBGMCU_TIM7_STOP
    | DBGMCU_IWDG_STOP
    | DBGMCU_WWDG_STOP
  ), ENABLE);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Init_NVIC(void)
{
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Init_IWDG(void)
{
  RCC_LSICmd(ENABLE);
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler(IWDG_Prescaler_128);
  IWDG_SetReload(625U - 1U);
  IWDG_ReloadCounter();
  IWDG_Enable();
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Init_GPIO(void)
{
  // LED
  GPIO_InitTypeDef led_0;
  led_0.GPIO_Pin = GPIO_Pin_8;
  led_0.GPIO_Mode = GPIO_Mode_Out_PP;
  led_0.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &led_0);  
  GPIO_PinLockConfig(GPIOC, GPIO_Pin_8);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Init_TIM7(void)
{
  TIM_TimeBaseInitTypeDef tim_7;
  tim_7.TIM_CounterMode = TIM_CounterMode_Up;
  tim_7.TIM_Prescaler = 36000U - 1U;
  tim_7.TIM_Period = 10000U - 1U;

  TIM_TimeBaseInit(TIM7, &tim_7);
  TIM_Cmd(TIM7, ENABLE);
  TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
  TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

  NVIC_InitTypeDef tim_7_pr;
  tim_7_pr.NVIC_IRQChannel = TIM7_IRQn;
  tim_7_pr.NVIC_IRQChannelSubPriority = 0;
  tim_7_pr.NVIC_IRQChannelPreemptionPriority = 15;
  tim_7_pr.NVIC_IRQChannelCmd = ENABLE;
  
  NVIC_Init(&tim_7_pr);  
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Init_RTC(void)
{
  RCC_LSEConfig(RCC_LSE_ON);
  
  PWR_BackupAccessCmd(ENABLE);
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
  RCC_RTCCLKCmd(ENABLE);
//  RTC_EnterConfigMode();
//  RTC_SetCounter(1550043676);
//  RTC_WaitForLastTask();
//  RTC_ExitConfigMode();
  PWR_BackupAccessCmd(DISABLE);
  
  NVIC_InitTypeDef rtc_nvic;
  rtc_nvic.NVIC_IRQChannel = RTC_IRQn;
  rtc_nvic.NVIC_IRQChannelPreemptionPriority = 14;
  rtc_nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&rtc_nvic);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Init(void)
{
  Init_SysTick();
  Init_PeripheralClock();
  Init_IWDG();
  Init_NVIC();
  Init_GPIO();
  Init_TIM7();
  Init_RTC();
  if (!Init_I2C_AT24Cxx())
  {
    Error_Handler("Init_I2C_AT24Cxx Error!");
  }
  at24cxx = AT24Cxx_Init(0xa0, AT24C32_Capacity);
}
//////////////////////////////////////////////////////////////// 







//////////////////////////// CRON ////////////////////////////// 
// *************************************************************
void SysQuantum_Handler(void)
{
}

// *************************************************************
void Millis_Handler(void)
{
}

// *************************************************************
void Seconds_Handler(void)
{
  IWDG_ReloadCounter();
}

// *************************************************************
void Minutes_Handler(void)
{
}

// *************************************************************
void Cron_Handler()
{
  CronStart:
  if (SysTick->CTRL & (1 << SysTick_CTRL_COUNTFLAG_Pos))
  { 
    sysQuantum++;
    SysQuantum_Handler();
  }
  
  if (sysQuantum >= millis_tmp)
  {
    millis++;
    millis_tmp = sysQuantum + 100;
    Millis_Handler();
  }
  
  if (millis >= seconds_tmp)
  {
    seconds++;
    seconds_tmp += 1000;
    Seconds_Handler();
  }
  
  if (seconds >= minutes_tmp)
  {
    minutes++;
    minutes_tmp += 60;
    Minutes_Handler();
  }
  
  while (sysQuantum < delay_tmp)
  {
    goto CronStart;
  }
  // !!!!!!!!! The bug!
  delay_tmp = 0;
}
//////////////////////////////////////////////////////////////// 






//////////////////////////// FLAGS ///////////////////////////// 
// *************************************************************
void BasicTimer_IT_Handler(TIM_TypeDef* TIMx)
{
  if (TIMx == TIM7)
  {
    Toggle_LED();
    dataBuffer[0] = 0x11223344;
    CRC_ResetDR();
    for (int i = 0; i < 0x20; i++)
    {
      dataBuffer[i + 1] = CRC_CalcCRC(dataBuffer[0] + 0xaabbccdd);
    }
    
    AT24Cxx_Write(at24cxx, 0x0040, 40, &((uint8_t*)dataBuffer)[0]);
    
//    for (int i = 0; i < 0x0400; i++)
//    {
//      dataBuffer[i] = 0xffffffff;
//    }
    
//    AT24Cxx_Write(at24cxx, 0x0000, 0x1000, &((uint8_t*)dataBuffer)[0]);

    AT24Cxx_Read(at24cxx, 0x0040, 0x1000, (uint8_t*)dataBuffer);
  }
}

// *************************************************************
void RTC_Alarm_Handler(void)
{
  RTC_ITConfig(RTC_IT_ALR, DISABLE);
}

// *************************************************************
void Flags_Handler(void)
{
  if (GetFlag(&_EREG_, _BT7F_))
  {
    SetFlag(&_EREG_, _BT7F_, FLAG_CLEAR);
    BasicTimer_IT_Handler(TIM7);
  }

  if (GetFlag(&_EREG_, _ALF_))
  {
    SetFlag(&_EREG_, _ALF_, FLAG_CLEAR);
    RTC_Alarm_Handler();
  }

  if (GetFlag(&_EREG_, _RDF_))
  {
    SetFlag(&_EREG_, _RDF_, FLAG_CLEAR);
    Display_Handler(TM1637);
  }
}
//////////////////////////////////////////////////////////////// 






//////////////////////////////////////////////////////////////// 
// *************************************************************
void Toggle_LED(void)
{
  BitAction BitVal = (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_8)) ? Bit_RESET : Bit_SET;
  GPIO_WriteBit(GPIOC, GPIO_Pin_8, BitVal);
}

// *************************************************************
void SetRTCAlarm(uint32_t __alarm_val)
{
  RTC_WaitForLastTask();
  RTC_ITConfig(RTC_IT_ALR, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  RTC_SetAlarm(RTC_GetCounter() + __alarm_val);
  RTC_WaitForLastTask();
  PWR_BackupAccessCmd(DISABLE);
}

//////////////////////////////////////////////////////////////// 
