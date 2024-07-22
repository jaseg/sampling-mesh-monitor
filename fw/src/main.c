
#include <global.h>
#include <string.h>

uint64_t sys_time_us;
uint64_t sync_time_us;

void update_leds(void);


int main(void) {
    /* Enable HSE w/ 8 MHz crystal */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {
        /* Wait for HSE to stabilize */
    }

    /* We're already in voltage scaling range 1. Enter boost mode (set main internal VDD from 1.20V to 1.28V) */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    PWR->CR5 &= ~PWR_CR5_R1MODE;

    /* Increase flash wait states to 5 required for operation above 136 MHz */
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY_Msk) | (4<<FLASH_ACR_LATENCY_Pos);
    while ((FLASH->ACR & FLASH_ACR_LATENCY_Msk) != (4<<FLASH_ACR_LATENCY_Pos)) {
        /* wait for flash controller to acknowledge change. */
    }
    
    /* Configure PLL with multiplier 42, divisor 2 for "R" output, and enable "R" (sysclk) output */
    RCC->PLLCFGR = (RCC->PLLCFGR & ~RCC_PLLCFGR_PLLN_Msk) | (42<<RCC_PLLCFGR_PLLN_Pos) | (3<<RCC_PLLCFGR_PLLSRC_Pos) | RCC_PLLCFGR_PLLREN;
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {
        /* wait for PLL to stabilize. */
    }
    
    /* Switch SYSCLK to PLL source. */
    RCC->CFGR |= (3<<RCC_CFGR_SW_Pos);
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != (3<<RCC_CFGR_SWS_Pos)) {
        /* wait for RCC to switch over. */
    }

    /* Enable peripheral clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; 

    /* GPIOA:
     *  0 -  DRV_CEN
     *  1 -  (testpoint)
     *  2 -  (testpoint)
     *  3 -  (testpoint)
     *  4 -  V_LVC_DAC
     *  5 -  VRES_BIAS_DAC
     *  6 -  VBIAS_DAC
     *  7 -  V_ADC2
     *  8 -  VRES_N
     *  9 -  VRES_P
     * 10 -  (testpoint)
     * 11 -  USB_D-
     * 12 -  USB_D+
     * 13 -  SWDIO
     * 14 -  SWCLK
     * 15 -  VBUS_SENSE
     */
    GPIOA->MODER &= (~(CLEAR(15))); /* Clear JTAG TDI pin mode */
    GPIOA->MODER |= OUT(0) | IN(1) | IN(2) | IN(3) | ANALOG(4) | ANALOG(5) | ANALOG(6) | ANALOG(7) | ANALOG(8) |
        ANALOG(9) | IN(10) | IN(11) | IN(12) | AF(13) | AF(14) | IN(15);
    GPIOA->AFR[1] = AFRH(13, 0) | /* SWDIO */
                    AFRH(14, 0);  /* SWCLK */
    GPIOA->OSPEEDR = 0xffffffff;
    GPIOA->BSRR = (1<<0); /* Clear DRV_CEN */

    /* GPIOB: 
     *  0 -  V_ADC1
     *  1 -  MESH_FLIP_N
     *  2 -  (testpoint)
     *  3 -  SWO
     *  4 -  (testpoint)
     *  5 -  (testpoint)
     *  6 -  SFP2
     *  7 -  SFP1
     *  8 -  (testpoint)
     *  9 -  SFP3
     * 10 -  SFP4
     * 11 -  V_TIA
     * 12 -  PULSE_A+
     * 13 -  PULSE_A-
     * 14 -  SAMPLE_A+
     * 15 -  SAMPLE_A-
     */

    GPIOB->MODER = ANALOG(0) | OUT(1) | IN(2) | AF(3) | IN(4) | IN(5) | IN(6) | IN(7) | IN(8) | IN(9) | IN(10) |
        ANALOG(11) | AF(12) | AF(13) | AF(14) | AF(15);
    GPIOB->AFR[0] = AFRL( 3, 0);  /* SWO */
    GPIOB->AFR[1] = AFRH(12, 13) | /* HRTIM1 Channel C1 */
                    AFRH(13, 13) | /* HRTIM1 Channel C2 */
                    AFRH(14, 13) | /* HRTIM1 Channel D1 */
                    AFRH(15, 13);  /* HRTIM1 Channel D2 */
    GPIOB->OSPEEDR = 0xffffffff;
    GPIOB->BSRR = (1<<1); /* Clear MESH_FLIP_N */

    /* GPIOC:
     *  0 -  LED1
     *  1 -  LED2
     *  2 -  LED3
     *  3 -  LED4
     *  4 -  MESH_FLIP
     *  5 -  (nc)
     *  6 -  SFP6
     *  7 -  VRES_EN
     *  8 -  I2C3_SCL
     *  9 -  I2C3_SDA
     * 10 -  (testpoint)
     * 11 -  (testpoint)
     * 12 -  (testpoint)
     * 13 -  (testpoint)
     * 14 -  RF_TERM
     * 15 -  RF_TERM/~SHORT
     */

    GPIOC->MODER = OUT(0) | OUT(1) | OUT(2) | OUT(3) | OUT(4) | IN(5) | IN(6) | OUT(7) | AF(8) | AF(9) | IN(10) | IN(11)
        | IN(12) | IN(13) | OUT(14) | OUT(15);
    GPIOC->AFR[1] = AFRH(8, 8) | /* I2C3 SCL */
                    AFRH(9, 8);  /* I2C3 SDA */

    GPIOC->BSRR = 0xf | (1<<4) | (1<<7) | (1<<14) | (1<<15); /* Clear all outputs on this bank */

    /* GPIOD:
     *  2 -  SFP5
     *  
     * (nothing to configure)
     */

    /* GPIOF:
     *  0 -  RCC_OSC_IN
     *  1 -  RCC_OSC_OUT
     *  
     * (nothing to configure)
     */

    /* GPIOG:
     * 10 -  SWD NRST
     *  
     * (nothing to configure)
     */

    SystemCoreClockUpdate();

    sys_time_us = 0;
    sync_time_us = 0;
    SysTick->LOAD = SystemCoreClock / SYSTICK_INTERVAL_US; /* 10ms interrupt interval */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    NVIC_EnableIRQ(SysTick_IRQn);

    while (23) {
        update_leds();
    }
}

void update_leds() {
    int blink_flag = (sys_time_us & (512*1024-1)) < (384*1024);

    GPIOC->BSRR = (blink_flag<<3) | (((!blink_flag)<<3)<<16);
}


void SysTick_Handler() {
    sys_time_us += SYSTICK_INTERVAL_US;
    sync_time_us += SYSTICK_INTERVAL_US;
}

uint64_t get_sync_time() {
    return sync_time_us + SysTick->VAL * SYSTICK_INTERVAL_US / SysTick->LOAD;
}

void HardFault_Handler() {
    asm volatile ("bkpt");
}

void delay_us(int duration_us) {
    while (duration_us--) {
        for (int i=0; i<60; i++) {
            asm volatile ("nop");
        }
    }
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	for (; n; n--) {
        *d++ = *s++;
    }
	return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
    return memcpy(dest, src, n);
}

void *memset(void *dest, int c, size_t n)
{
    unsigned char *d = dest;
    while (n--) {
        *d++ = c;
    }
	return dest;
}

size_t strlen(const char *s)
{
    const char *start = s;
    while (*s) {
        s++;
    }
    return s - start;
}

void __libc_init_array (void) __attribute__((weak));
void __libc_init_array () {
}
