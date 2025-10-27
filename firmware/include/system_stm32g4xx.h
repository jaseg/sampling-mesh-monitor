#ifndef __STM32_SYSTEM_H__
#define __STM32_SYSTEM_H__

extern uint32_t SystemCoreClock;
extern const uint8_t AHBPrescTable[16];
extern const uint8_t APBPrescTable[8];

void SystemInit(void);
void SystemCoreClockUpdate();

#endif /* __STM32_SYSTEM_H__ */
