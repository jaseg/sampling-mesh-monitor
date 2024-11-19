
#include <global.h>
#include <string.h>

uint64_t sys_time_us;
uint64_t sync_time_us;

void update_leds(void);

uint16_t adc1_readings[512];
uint16_t adc2_readings[512];

void debug_plot_hook(void){
}

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
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_DAC1EN |
        RCC_AHB2ENR_DAC2EN | RCC_AHB2ENR_ADC12EN;
    /* SYSCFGEN for VREFBUF */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN | RCC_APB2ENR_HRTIM1EN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C3EN;

    /* GPIOA:
     *  0 -  DRV_CEN
     *  1 -  (testpoint)
     *  2 -  (testpoint)
     *  3 -  (testpoint)
     *  4 -  DAC1 Channel 1 / V_LVC_DAC
     *  5 -  DAC1 Channel 2 / VRES_BIAS_DAC
     *  6 -  DAC2 Channel 1 / VBIAS_DAC
     *  7 -  V_ADC2 -> ADC2 CH4
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
    GPIOA->BRR = (1<<0); /* Clear DRV_CEN */

    /* GPIOB: 
     *  0 -  V_ADC1 -> ADC1 CH15
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
     * 13 -  PULSE_A- (currently unused, 74LVC2G157 are populated)
     * 14 -  SAMPLE_A+ (currently unused, 74LVC2G157 are populated)
     * 15 -  SAMPLE_A-
     */

    GPIOB->MODER = ANALOG(0) | OUT(1) | IN(2) | AF(3) | IN(4) | IN(5) | IN(6) | IN(7) | IN(8) | IN(9) | IN(10) |
        ANALOG(11) | AF(12) | IN(13) | IN(14) | AF(15);
    GPIOB->AFR[0] = AFRL( 3, 0);  /* SWO */
    GPIOB->AFR[1] = AFRH(12, 13) | /* HRTIM1 Channel C1 */
                    AFRH(13, 13) | /* HRTIM1 Channel C2 */
                    AFRH(14, 13) | /* HRTIM1 Channel D1 */
                    AFRH(15, 13);  /* HRTIM1 Channel D2 */
    GPIOB->OSPEEDR = 0xffffffff;
    GPIOB->BSRR = (1<<1); /* Set MESH_FLIP_N, leave MESH_FLIP reset below. These two must be complementary. */

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
    SysTick->LOAD = SystemCoreClock / 1000 * SYSTICK_INTERVAL_US; /* 10ms interrupt interval */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    NVIC_EnableIRQ(SysTick_IRQn);

    /* Configure DAC1 to generate various reference voltages */
    VREFBUF->CSR = (2<<VREFBUF_CSR_VRS_Pos) | VREFBUF_CSR_ENVR;
    DAC1->CR = DAC_CR_EN1 | DAC_CR_EN2;
    /* MCR is already set up correctly to put both channels on the external pins with their buffers enabled.*/
    DAC1->DHR12R1 = 0xfff; /* V_LVC_DAC */
    DAC1->DHR12R2 = 0xa00; /* VRES_BIAS_DAC */

    DAC2->CR = DAC_CR_EN1;
    DAC2->DHR12R1 = 0xfff; /* VBIAS_DAC */

    /* Configure HRTIM1 to generate excitation and sampling pules */
    HRTIM1->sCommonRegs.DLLCR = HRTIM_DLLCR_CAL;

    while (!(HRTIM1->sCommonRegs.ISR & HRTIM_ISR_DLLRDY)) {
        /* Wait for DLL calibration to complete. */
    }

    HRTIM1->sMasterRegs.MPER = 168 * 0x20; /* We have to translate all counter values to the 32x oversampled high-resolution clock */
    HRTIM1->sMasterRegs.MCR = HRTIM_MCR_CONT | (2<<HRTIM_MCR_SYNC_OUT_Pos);

    /* On each channel: 
     *
     * Output 1 set on CMP1, reset on CMP2.
     * Output 2 set on CMP3, reset on CMP4.
     *
     * Outputs C2 and D1 are currently unused, since we have the 74LVC2G157s populated.
     */

    /* Channel C: Stimulus pulse */
    //HRTIM1->sTimerxRegs[2].TIMxCR = HRTIM_TIMCR_SYNCSTRT;
    HRTIM1->sTimerxRegs[2].RSTxR = HRTIM_RSTR_MSTPER;
    HRTIM1->sTimerxRegs[2].CMP1xR = 0x60; 
    HRTIM1->sTimerxRegs[2].CMP2xR = HRTIM1->sTimerxRegs[2].CMP1xR + 0x20*2; 
    HRTIM1->sTimerxRegs[2].CMP3xR = 0; 
    HRTIM1->sTimerxRegs[2].CMP4xR = 0; 
    HRTIM1->sTimerxRegs[2].SETx1R = HRTIM_SET1R_CMP1;
    HRTIM1->sTimerxRegs[2].SETx2R = HRTIM_SET2R_CMP3;
    HRTIM1->sTimerxRegs[2].RSTx1R = HRTIM_RST1R_CMP2;
    HRTIM1->sTimerxRegs[2].RSTx2R = HRTIM_RST2R_CMP4;

    /* Channel D: Diode sampler pulse */
    //HRTIM1->sTimerxRegs[3].TIMxCR = HRTIM_TIMCR_SYNCSTRT;
    HRTIM1->sTimerxRegs[3].RSTxR = HRTIM_RSTR_MSTPER;
    HRTIM1->sTimerxRegs[3].CMP1xR = 0; 
    HRTIM1->sTimerxRegs[3].CMP2xR = 0; 
    HRTIM1->sTimerxRegs[3].CMP3xR = 0x60; 
    HRTIM1->sTimerxRegs[3].CMP4xR = HRTIM1->sTimerxRegs[3].CMP3xR + 0x20*2; 
    HRTIM1->sTimerxRegs[3].SETx1R = HRTIM_SET1R_CMP1;
    HRTIM1->sTimerxRegs[3].SETx2R = HRTIM_SET2R_CMP3;
    HRTIM1->sTimerxRegs[3].RSTx1R = HRTIM_RST1R_CMP2;
    HRTIM1->sTimerxRegs[3].RSTx2R = HRTIM_RST2R_CMP4;

    /* Channel A: ADC trigger */
    HRTIM1->sTimerxRegs[0].RSTxR = HRTIM_RSTR_MSTPER;
    HRTIM1->sTimerxRegs[0].CMP3xR = 160 * 0x20; 
    HRTIM1->sTimerxRegs[0].CMP4xR = 160 * 0x20; 
    
    HRTIM1->sMasterRegs.MCR |= HRTIM_MCR_TCCEN | HRTIM_MCR_TDCEN | HRTIM_MCR_TACEN;

    HRTIM1->sCommonRegs.ADC1R = HRTIM_ADC1R_AD1TAC3;
    HRTIM1->sCommonRegs.ADC3R = HRTIM_ADC3R_AD3TAC4;
    HRTIM1->sCommonRegs.OENR = HRTIM_OENR_TC1OEN;
    HRTIM1->sCommonRegs.OENR = HRTIM_OENR_TC2OEN;
    HRTIM1->sCommonRegs.OENR = HRTIM_OENR_TD1OEN;
    HRTIM1->sCommonRegs.OENR = HRTIM_OENR_TD2OEN;

    HRTIM1->sMasterRegs.MCR |= HRTIM_MCR_MCEN;

    /* TODO: Schedule DLL calibration */

    I2C3->TIMINGR = 0x00C0216C;
    I2C3->CR1 = I2C_CR1_PE;

    I2C3->TXDR = 0x02; /* Write starting register address: Channel A0 config */
    I2C3->CR2 = (4 << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | (0xfc << I2C_CR2_SADD_Pos) | I2C_CR2_START;
    for (size_t n=0; n<4; n++) {
        while ((I2C1->ISR & I2C_ISR_BUSY) || (!(I2C1->ISR & I2C_ISR_TXE))) {
            /* Wait for byte transmission to complete */
        }

        I2C3->TXDR = 0x7f; /* Channel x config: Max EQ peaking, max gain, max swing. */
    }

    while ((I2C1->ISR & I2C_ISR_BUSY) || (!(I2C1->ISR & I2C_ISR_TXE))) {
        /* Wait for byte transmission to complete */
    }

    ADC12_COMMON->CCR = (3<<ADC_CCR_CKMODE_Pos); /* Note: The ADC core just hung in both adcal and adrdy when using the
                                                    internal clock. I suspect a silicon bug. */
    ADC1->CR = 0; /* Disable deep power down mode */
    ADC2->CR = 0;
    ADC1->CR = ADC_CR_ADVREGEN;
    ADC2->CR = ADC_CR_ADVREGEN;
    delay_us(50); /* Load-bearing nop. Wait for the regulators to settle */

    ADC1->CR |= ADC_CR_ADCAL; /* Run self-cal */
    ADC2->CR |= ADC_CR_ADCAL;
    while ((ADC1->CR & ADC_CR_ADCAL) || (ADC2->CR & ADC_CR_ADCAL)) {
        /* do nothing */
    }

    ADC1->CR |= ADC_CR_ADEN;
    ADC2->CR |= ADC_CR_ADEN;
    delay_us(1); /* Load-bearing nop. There seems to be some silicon erratum here. */
    while (!((ADC1->ISR & ADC_ISR_ADRDY) && (ADC2->ISR & ADC_ISR_ADRDY))) {
        /* do nothing */
    }

    ADC1->CFGR = ADC_CFGR_JQDIS
        | ADC_CFGR_DISCEN
        | (0x1 << ADC_CFGR_EXTEN_Pos)
        | (21 << ADC_CFGR_EXTSEL_Pos) /* HRTIM Channel A CCR 3 via hrtim_adc_trg1 signal */
        | (0x0 << ADC_CFGR_RES_Pos)
        | ADC_CFGR_DMACFG
        | ADC_CFGR_DMAEN;
    ADC1->SQR1 = (0x0<<ADC_SQR1_L_Pos) | (0x0f << ADC_SQR1_SQ1_Pos);
    ADC1->CR |= ADC_CR_ADSTART;

    ADC2->CFGR = ADC_CFGR_JQDIS
        | ADC_CFGR_DISCEN
        | (0x1 << ADC_CFGR_EXTEN_Pos)
        | (22 << ADC_CFGR_EXTSEL_Pos) /* HRTIM Channel A CCR 4 via hrtim_adc_trg3 signal */
        | (0x0 << ADC_CFGR_RES_Pos)
        | ADC_CFGR_DMACFG
        | ADC_CFGR_DMAEN;
    ADC2->SQR1 = (0x0<<ADC_SQR1_L_Pos) | (0x0f << ADC_SQR1_SQ1_Pos);
    ADC2->CR |= ADC_CR_ADSTART;

    DMAMUX1[0].CCR = (5<<DMAMUX_CxCR_DMAREQ_ID_Pos); /* ADC1 to DMAMUX1 */
    DMAMUX1[1].CCR = (36<<DMAMUX_CxCR_DMAREQ_ID_Pos); /* ADC2 to DMAMUX2 */

    DMA1_Channel1->CPAR = (uint32_t) &ADC1->DR;
    DMA1_Channel1->CMAR = (uint32_t) adc1_readings;
    DMA1_Channel1->CNDTR = COUNT_OF(adc1_readings);
    DMA1_Channel1->CCR = (1<<DMA_CCR_MSIZE_Pos)
        | (1<<DMA_CCR_PSIZE_Pos)
        | DMA_CCR_MINC
        | DMA_CCR_CIRC
        | DMA_CCR_TEIE
        | DMA_CCR_TCIE
        | DMA_CCR_EN;

    DMA1_Channel2->CPAR = (uint32_t) &ADC2->DR;
    DMA1_Channel2->CMAR = (uint32_t) adc2_readings;
    DMA1_Channel2->CNDTR = COUNT_OF(adc2_readings);
    DMA1_Channel2->CCR = (1<<DMA_CCR_MSIZE_Pos)
        | (1<<DMA_CCR_PSIZE_Pos)
        | DMA_CCR_MINC
        | DMA_CCR_CIRC
        | DMA_CCR_TEIE
        | DMA_CCR_TCIE
        | DMA_CCR_EN;

	NVIC_SetPriority(DMA1_Channel1_IRQn, 2);
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    int n = 0;
    int d = 1;
    int dir = 1;
    while (23) {
        update_leds();
        //if (n == 200000) {
        if (n == 10000) {
            n = 0;
                
            if (dir) {
                d++;
            } else {
                d--;
            }

            if (d == 0) {
                dir = !dir;
            } else if (d == 0x2ff) {
            //} else if (d == 0x20) {
                dir = !dir;
            }

            DAC1->DHR12R1 = 0xc00 + d;
            HRTIM1->sTimerxRegs[3].CMP3xR = 0x60 + d; 
            HRTIM1->sTimerxRegs[3].CMP4xR = HRTIM1->sTimerxRegs[3].CMP3xR + 0x20*2; 
        } else {
            n++;
        }
    }
}

void DMA1_Channel1_IRQHandler(void) {
    static int n = 0;
    DMA1->IFCR = DMA_IFCR_CTCIF1;

    n++;
    if (n == 3) {
        n = 0;
        debug_plot_hook();
    }
}


void update_leds() {
    int blink_flag = (sys_time_us & (512*1024-1)) < (384*1024);

    if (blink_flag) {
        GPIOC->BSRR = (1<<3);
    } else {
        GPIOC->BRR = (1<<3);
    }
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
