
#include <global.h>
#include <string.h>

uint64_t sys_time_us;
uint64_t sync_time_us;

void update_leds(void);

int sampling_phase = 0;
int adc_backoff = 60;
uint16_t adc1_raw[512];
uint16_t adc2_raw[COUNT_OF(adc1_raw)];

#define ADC_DELAY_WINDOW_SIZE 768
struct adc_stat_buffer {
    uint32_t adc1_mean[ADC_DELAY_WINDOW_SIZE];
    uint32_t adc1_var[ADC_DELAY_WINDOW_SIZE];
    uint32_t adc2_mean[ADC_DELAY_WINDOW_SIZE];
    uint32_t adc2_var[ADC_DELAY_WINDOW_SIZE];
};

enum {
    MEAS_PB_CAL = 0,
    MEAS_PB_OPEN_FWD  = 1,
    MEAS_PB_OPEN_FLIP = 2,
    MEAS_PB_LOAD_FWD  = 3,
    MEAS_PB_LOAD_FLIP = 4,
    MEAS_PB_SHORT_FWD  = 5,
    MEAS_PB_SHORT_FLIP = 6,
    _MEAS_PB_COUNT = 7
} meas_pb_now = 0;

struct {
    uint32_t mcu_serial[3];
    struct adc_stat_buffer adc_stats[_MEAS_PB_COUNT];
} host_data;

/* Bootup values. Will be overwritten with defaults from web interface HTML on launch. */
const int default_sampling_phase_var = 2;
const int default_sampling_width = 2;
const int default_stim_offset = 104;
const int default_oversampling = 64;
int sampling_phase_var = default_sampling_phase_var;
int sampling_width = default_sampling_width;
int stim_offset = default_stim_offset;
int oversampling = default_oversampling;

enum dbg_settings_mask {
    DBG_MESH_FLIP_N = 1,
    DBG_MESH_FLIP = 2,
    DBG_RF_TERM = 4,
    DBG_RF_TERM_SHORT = 8,
};

volatile uint32_t dbg_settings[2] = {
    (default_sampling_phase_var<<4) |
    (default_sampling_width<<12) |
    (default_stim_offset<<20),
    (default_oversampling<<0)};

void meas_pb_config();
void meas_pb_config() {
    struct flags {
        bool flip,
             flip_n,
             term,
             term_short;
    };
    struct flags config[_MEAS_PB_COUNT] = {
            [MEAS_PB_CAL]           = {.flip=1, .flip_n=1, .term=0, .term_short=0},
            [MEAS_PB_OPEN_FWD]      = {.flip=0, .flip_n=1, .term=0, .term_short=0},
            [MEAS_PB_OPEN_FLIP]     = {.flip=1, .flip_n=0, .term=0, .term_short=0},
            [MEAS_PB_LOAD_FWD]      = {.flip=0, .flip_n=1, .term=1, .term_short=0},
            [MEAS_PB_LOAD_FLIP]     = {.flip=1, .flip_n=0, .term=1, .term_short=0},
            [MEAS_PB_SHORT_FWD]     = {.flip=0, .flip_n=1, .term=1, .term_short=1},
            [MEAS_PB_SHORT_FLIP]    = {.flip=1, .flip_n=0, .term=1, .term_short=1},
        };

    struct flags now = config[meas_pb_now];

    if (now.flip) {
        GPIOC->BSRR = 1<<4;
    } else {
        GPIOC->BRR = 1<<4;
    }

    if (now.flip_n) {
        GPIOB->BSRR = 1<<1;
    } else {
        GPIOB->BRR = 1<<1;
    }

    if (now.term) {
        GPIOC->BSRR = 1<<14;
    } else {
        GPIOC->BRR = 1<<14;
    }

    if (now.term_short) {
        GPIOC->BSRR = 1<<15;
    } else {
        GPIOC->BRR = 1<<15;
    }
}

void debug_plot_hook(void);
void debug_plot_hook(void){
    sampling_phase_var = (dbg_settings[0]>>4) & 0xff;
    sampling_width = (dbg_settings[0]>>12) & 0xff;
    stim_offset = (dbg_settings[0]>>20) & 0xff;
    oversampling = dbg_settings[1] & 0x1ff;
    if (oversampling == 0) {
        oversampling = 1;
    }
    const int full = HRTIM1->sMasterRegs.MPER;
    const int half = full/2;

    /* The sampling pulse waveform is generated as follows:
     *
     * Through the master timer overflow and compare 1 events, a square wave is generated that sets on master overflow
     * and resets at half period. Within this square wave, two pulses are inserted: A negative going pulse referenced to
     * an offset before half period by the channel compares 1 and 2, and a positive going pulse referenced to an offset
     * before the period end by the channel compares 3 and 4.
     *
     * The pulse width is the sampling width.
     */
    /* Sampling pulse */
    HRTIM1->sTimerxRegs[3].CMP1xR = half - adc_backoff*0x20 - sampling_phase_var*0x20 - sampling_width*0x20;
    HRTIM1->sTimerxRegs[3].CMP2xR = half - adc_backoff*0x20 - sampling_phase_var*0x20;
    HRTIM1->sTimerxRegs[3].CMP3xR = full - adc_backoff*0x20 - sampling_phase_var*0x20 - sampling_width*0x20;
    HRTIM1->sTimerxRegs[3].CMP4xR = full - adc_backoff*0x20 - sampling_phase_var*0x20;

    /* ADC trigger */
    HRTIM1->sTimerxRegs[1].CMP3xR = half - adc_backoff*0x20;
    HRTIM1->sTimerxRegs[1].CMP4xR = full - adc_backoff*0x20;
    /* Debug output */
    HRTIM1->sTimerxRegs[5].CMP1xR = HRTIM1->sTimerxRegs[1].CMP3xR;
    HRTIM1->sTimerxRegs[5].CMP2xR = HRTIM1->sTimerxRegs[1].CMP4xR;
    //HRTIM1->sTimerxRegs[3].CMP4xR = HRTIM1->sTimerxRegs[3].CMP3xR + sampling_width * 0x20;
    //HRTIM1->sTimerxRegs[3].CMP1xR = 0; 
    //HRTIM1->sTimerxRegs[3].CMP2xR = 0; 
    //HRTIM1->sTimerxRegs[3].CMP3xR = global_sampling_offset * 0x20; 
    //int sampling_width = (dbg_settings[0]>>12) & 0xff;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    while ((DMA1_Channel1->CCR & DMA_CCR_EN) | (DMA1_Channel2->CCR & DMA_CCR_EN)) {
        /* Do nothing. */
    }
    DMA1_Channel2->CNDTR = oversampling+4;
    DMA1_Channel1->CNDTR = oversampling+4;

    DMA1_Channel2->CCR |= DMA_CCR_EN;
    DMA1_Channel1->CCR |= DMA_CCR_EN;
}

int main(void) {
    /* Enable HSE w/ 8 MHz crystal */
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {
        /* Wait for HSE to stabilize */
    }

    /* We're already in voltage scaling range 1. Enter boost mode (set main internal VDD from 1.20V to 1.28V) */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN| RCC_APB1ENR1_TIM5EN | RCC_APB1ENR1_TIM3EN;
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
    /* PLL clocks now:
     *   R = 168 MHz
     *   Q = 168 MHz
     *   P = 19.765 MHz
     *
     *   System clocks:
     *   All undivided at 168 MHz.
     */
    
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
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN | RCC_APB2ENR_HRTIM1EN | RCC_APB2ENR_TIM16EN;
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
    GPIOA->MODER &= (~(CLEAR(15) | CLEAR(1))); /* Clear JTAG TDI pin mode */
    GPIOA->MODER |= OUT(0) | OUT(1) | IN(2) | IN(3) | ANALOG(4) | ANALOG(5) | ANALOG(6) | ANALOG(7) | ANALOG(8) |
        ANALOG(9) | AF(10) | IN(11) | IN(12) | AF(13) | AF(14) | IN(15);
    GPIOA->AFR[1] = AFRH(13, 0) | /* SWDIO */
                    AFRH(14, 0) | /* SWCLK */
                    AFRH(10, 13); /* HRTIM1 Channel B1 */
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

    GPIOB->MODER = ANALOG(0) | OUT(1) | IN(2) | AF(3) | AF(4) | IN(5) | IN(6) | IN(7) | OUT(8) | IN(9) | IN(10) |
        ANALOG(11) | AF(12) | IN(13) | IN(14) | AF(15);
    GPIOB->AFR[0] = AFRL( 3, 0)  | /* SWO */
                    AFRL( 4, 1);   /* TIM16 CH1 */
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
     *  6 -  SFP6 - HRTIM1 CHF1 (Debug out)
     *  7 -  VRES_EN
     *  8 -  I2C1_SCL
     *  9 -  I2C1_SDA
     * 10 -  (testpoint)
     * 11 -  (testpoint)
     * 12 -  (testpoint)
     * 13 -  (testpoint)
     * 14 -  RF_TERM
     * 15 -  RF_TERM/~SHORT
     */

    GPIOC->MODER = OUT(0) | OUT(1) | OUT(2) | OUT(3) | OUT(4) | IN(5) | AF(6) | OUT(7) | AF(8) | AF(9) | OUT(10) | IN(11)
        | AF(12) | IN(13) | OUT(14) | OUT(15);
    GPIOC->AFR[0] = AFRL(6, 13); /* HRTIM1 CHF1 */
    GPIOC->AFR[1] = AFRH(8, 8) | /* I2C1 SCL */
                    AFRH(9, 8) | /* I2C1 SDA */
                    AFRH(12, 1); /* TIM5 CH2 */

    GPIOC->BRR = (1<<4) | (1<<7) | (1<<14) | (1<<15); /* Clear all outputs on this bank */
    GPIOC->BSRR = 0xf;

    const uint32_t *mcu_uid_reg = (const uint32_t *)UID_BASE;
    host_data.mcu_serial[0] = mcu_uid_reg[0];
    host_data.mcu_serial[1] = mcu_uid_reg[1];
    host_data.mcu_serial[2] = mcu_uid_reg[2];

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
    
    /* clock debug */
    TIM5->CCMR1 = (0x6<<TIM_CCMR1_OC2M_Pos) | (0x0<<TIM_CCMR1_CC2S_Pos);
    TIM5->CCER = TIM_CCER_CC2E;
    TIM5->ARR = 167;
    TIM5->CCR1 = 83;
    TIM5->CR1 = TIM_CR1_CEN;

    TIM3->CCMR1 = (0x6<<TIM_CCMR1_OC1M_Pos) | (0x0<<TIM_CCMR1_CC1S_Pos);
    TIM3->CCER = TIM_CCER_CC1E;
    TIM3->ARR = 167;
    TIM3->CCR1 = 83;
    TIM3->BDTR = TIM_BDTR_MOE;
    TIM3->CR1 = TIM_CR1_CEN;

    TIM16->CCMR1 = (0x6<<TIM_CCMR1_OC1M_Pos) | (0x0<<TIM_CCMR1_CC1S_Pos);
    TIM16->CCER = TIM_CCER_CC1E;
    TIM16->ARR = 167;
    TIM16->CCR1 = 83;
    TIM16->BDTR = TIM_BDTR_MOE;
    TIM16->CR1 = TIM_CR1_CEN;

    sys_time_us = 0;
    sync_time_us = 0;
    SysTick->LOAD = SYSTICK_INTERVAL_US / 1000 * SystemCoreClock / 1000; /* 10ms interrupt interval */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    //NVIC_EnableIRQ(SysTick_IRQn);

    /* Configure DAC1 to generate various reference voltages */
    VREFBUF->CSR = (2<<VREFBUF_CSR_VRS_Pos) | VREFBUF_CSR_ENVR;
    DAC1->CR = DAC_CR_EN1 | DAC_CR_EN2;
    /* MCR is already set up correctly to put both channels on the external pins with their buffers enabled.*/
    DAC1->DHR12R1 = 0xfff; /* V_LVC_DAC */
    DAC1->DHR12R2 = 0xa00; /* VRES_BIAS_DAC */

    DAC2->CR = DAC_CR_EN1;
    DAC2->DHR12R1 = 0xfff; /* VBIAS_DAC */

    HRTIM1->sMasterRegs.MCR = HRTIM_MCR_CONT;
    HRTIM1->sMasterRegs.MPER = 378 * 0x20; /* We have to translate all counter values to the 32x oversampled high-resolution clock */
    /* The master period should now be 168 HRTIM bus clock cycles, which at 168 MHz corresponds to 1.00 µs.
     * Note that the HRTIM doc is very unclear if we should set the register to 168 or 167 for that, let's just see.
     */
    HRTIM1->sMasterRegs.MCMP1R = HRTIM1->sMasterRegs.MPER / 2;

    /* On each channel: 
     *
     * Output 1 set on CMP1, reset on CMP2.
     * Output 2 set on CMP3, reset on CMP4.
     *
     * Outputs C2 and D1 are currently unused, since we have the 74LVC2G157s populated.
     */
    
    /* HRTIM1 receives 168 MHz from the bus clock. The equivalent high-resolution output frequency would be 5.376 GHz.
     * This means one full clock cycle is 5.952 ns, and one output subdivision, equivalent to one count in the HRTIM
     * registers, is 186.0 ps.
     */

    /* Channel C: Stimulus pulse */
    HRTIM1->sTimerxRegs[2].TIMxCR = HRTIM_TIMCR_MSTU;
    HRTIM1->sTimerxRegs[2].PERxR = HRTIM1->sMasterRegs.MPER;
    HRTIM1->sTimerxRegs[2].RSTxR = HRTIM_RSTR_MSTPER;
    HRTIM1->sTimerxRegs[2].SETx1R = HRTIM_SET1R_CMP2;
    HRTIM1->sTimerxRegs[2].RSTx1R = HRTIM_RST1R_CMP1;

    /* Channel D: Diode sampler pulse */
    HRTIM1->sTimerxRegs[3].TIMxCR = HRTIM_TIMCR_MSTU;
    HRTIM1->sTimerxRegs[3].PERxR = HRTIM1->sMasterRegs.MPER;
    HRTIM1->sTimerxRegs[3].RSTxR = HRTIM_RSTR_MSTPER;
    //HRTIM1->sTimerxRegs[3].CMP1xR = 0; 
    //HRTIM1->sTimerxRegs[3].CMP2xR = 0; 
    //HRTIM1->sTimerxRegs[3].CMP3xR = global_sampling_offset * 0x20; 
    //int sampling_width = (dbg_settings>>12) & 0xff;
    //HRTIM1->sTimerxRegs[3].CMP4xR = HRTIM1->sTimerxRegs[3].CMP3xR + sampling_width;
    //HRTIM1->sTimerxRegs[3].SETx1R = HRTIM_SET1R_CMP1;
    //HRTIM1->sTimerxRegs[3].RSTx1R = HRTIM_RST1R_CMP2;
    HRTIM1->sTimerxRegs[3].SETx2R = HRTIM_SET2R_MSTPER | HRTIM_SET2R_CMP2 | HRTIM_SET2R_CMP3;
    HRTIM1->sTimerxRegs[3].RSTx2R = HRTIM_RST2R_MSTCMP1 | HRTIM_RST2R_CMP1 | HRTIM_RST2R_CMP4;

    /* Channel B: ADC trigger */
    HRTIM1->sTimerxRegs[1].TIMxCR = HRTIM_TIMCR_MSTU;
    HRTIM1->sTimerxRegs[1].PERxR = HRTIM1->sMasterRegs.MPER;
    HRTIM1->sTimerxRegs[1].RSTxR = HRTIM_RSTR_MSTPER;
    //int sampling_phase_var = (dbg_settings[0]>>4) & 0x3f;
    //HRTIM1->sTimerxRegs[1].CMP3xR = (global_sampling_offset + sampling_phase_var) * 0x20;
    //HRTIM1->sTimerxRegs[1].CMP4xR = (global_sampling_offset + sampling_phase_var) * 0x20; 

    /* Channel F: Debug output */
    HRTIM1->sTimerxRegs[5].TIMxCR = HRTIM_TIMCR_MSTU;
    HRTIM1->sTimerxRegs[5].PERxR = HRTIM1->sMasterRegs.MPER;
    HRTIM1->sTimerxRegs[5].RSTxR = HRTIM_RSTR_MSTPER;
    HRTIM1->sTimerxRegs[5].SETx1R = HRTIM_SET1R_CMP1;
    HRTIM1->sTimerxRegs[5].RSTx1R = HRTIM_RST1R_CMP2;
    
    HRTIM1->sMasterRegs.MCR |= HRTIM_MCR_TCCEN | HRTIM_MCR_TDCEN | HRTIM_MCR_TBCEN | HRTIM_MCR_TFCEN;

    HRTIM1->sCommonRegs.ADC1R = HRTIM_ADC1R_AD1TBC3; /* hrtim_adc_trg1 signal (*NOT* ADC1!) */
    HRTIM1->sCommonRegs.ADC3R = HRTIM_ADC3R_AD3TBC4; /* hrtim_adc_trg3 signal (*NOT* ADC3!) */
    HRTIM1->sCommonRegs.OENR |= HRTIM_OENR_TC1OEN;
    HRTIM1->sCommonRegs.OENR |= HRTIM_OENR_TC2OEN;
    HRTIM1->sCommonRegs.OENR |= HRTIM_OENR_TD1OEN;
    HRTIM1->sCommonRegs.OENR |= HRTIM_OENR_TD2OEN;
    HRTIM1->sCommonRegs.OENR |= HRTIM_OENR_TF1OEN;

    /* Configure HRTIM1 to generate excitation and sampling pules */
    HRTIM1->sCommonRegs.DLLCR = HRTIM_DLLCR_CAL;

//    I2C3->TIMINGR = 0x00C0216C;
//    I2C3->CR1 = I2C_CR1_PE;
//
//    I2C3->TXDR = 0x03; /* Write starting register address: Channel A0 config */
//    I2C3->CR2 = (6 << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | (0xfc << I2C_CR2_SADD_Pos) | I2C_CR2_START;
//    while ((I2C3->ISR & I2C_ISR_BUSY) || (!(I2C3->ISR & I2C_ISR_TXE))) {
//        /* Wait for byte transmission to complete */
//    }
//
//    I2C3->TXDR = 0; /* Write starting register address: Channel power config */
//    while ((I2C3->ISR & I2C_ISR_BUSY) || (!(I2C3->ISR & I2C_ISR_TXE))) {
//        /* Wait for byte transmission to complete */
//    }
//
//    for (size_t n=0; n<4; n++) {
//        I2C3->TXDR = 0x7f; /* Channel x config: Max EQ peaking, max gain, max swing. */
//        while ((I2C3->ISR & I2C_ISR_BUSY) || (!(I2C3->ISR & I2C_ISR_TXE))) {
//            /* Wait for byte transmission to complete */
//        }
//    }

    ADC12_COMMON->CCR = (3<<ADC_CCR_CKMODE_Pos); /* Note: The ADC core just hung in both adcal and adrdy when using the
                                                    internal clock. I suspect a silicon bug. */
    /* The ADCs 1 and 2 are now clocked by adc_hclk/4, which is 42.0 MHz. Max for multiple ADC on is 52 MHz. */
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
    ADC1->SMPR1 = (0x2<<ADC_SMPR1_SMP0_Pos); /* Sampling time 0x2 = 12.5 ADC clock cycles */
    ADC1->CR |= ADC_CR_ADSTART;
    ADC1->IER = ADC_IER_EOCIE;
    NVIC_SetPriority(ADC1_2_IRQn, 3);
    //NVIC_EnableIRQ(ADC1_2_IRQn);

    ADC2->CFGR = ADC_CFGR_JQDIS
        | ADC_CFGR_DISCEN
        | (0x1 << ADC_CFGR_EXTEN_Pos)
        | (22 << ADC_CFGR_EXTSEL_Pos) /* HRTIM Channel A CCR 4 via hrtim_adc_trg3 signal */
        | (0x0 << ADC_CFGR_RES_Pos)
        | ADC_CFGR_DMACFG
        | ADC_CFGR_DMAEN;
    ADC2->SQR1 = (0x0<<ADC_SQR1_L_Pos) | (0x04 << ADC_SQR1_SQ1_Pos);
    ADC2->SMPR1 = (0x2<<ADC_SMPR1_SMP0_Pos); /* Sampling time 0x2 = 12.5 ADC clock cycles */
    ADC2->CR |= ADC_CR_ADSTART;
    /* The total ADC conversion time is (t_sampling + t_adc + 0.5) ADC clock cycles. t_sampling is the sampling time in
     * cycles, which we set to 12.5 ADC clock cycles. t_adc is one cycle per bit of resolution, which we
     * leave at the default of 12 bits.
     *
     * Thus, t_conv for us is 25 ADC clock cycles, corresponding to 0.595 µs.
     */

    DMAMUX1[0].CCR = (5<<DMAMUX_CxCR_DMAREQ_ID_Pos); /* ADC1 to DMAMUX1 */
    DMAMUX1[1].CCR = (36<<DMAMUX_CxCR_DMAREQ_ID_Pos); /* ADC2 to DMAMUX2 */

    DMA1_Channel1->CPAR = (uint32_t) &ADC1->DR;
    DMA1_Channel1->CMAR = (uint32_t) adc1_raw;
    DMA1_Channel1->CNDTR = oversampling+4;
    DMA1_Channel1->CCR = (1<<DMA_CCR_MSIZE_Pos)
        | (1<<DMA_CCR_PSIZE_Pos)
        | DMA_CCR_MINC
        | DMA_CCR_CIRC
        | DMA_CCR_TCIE
        | DMA_CCR_EN;

    DMA1_Channel2->CPAR = (uint32_t) &ADC2->DR;
    DMA1_Channel2->CMAR = (uint32_t) adc2_raw;
    DMA1_Channel2->CNDTR = oversampling+4;
    DMA1_Channel2->CCR = (1<<DMA_CCR_MSIZE_Pos)
        | (1<<DMA_CCR_PSIZE_Pos)
        | DMA_CCR_MINC
        | DMA_CCR_CIRC
        | DMA_CCR_EN;

	NVIC_SetPriority(DMA1_Channel1_IRQn, 2);
	NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    while (!(HRTIM1->sCommonRegs.ISR & HRTIM_ISR_DLLRDY)) {
        /* Wait for DLL calibration to complete. */
    }

    /* Start the high-resolution timer */
    debug_plot_hook();
    HRTIM1->sMasterRegs.MCR |= HRTIM_MCR_MCEN;
    while (23) {
        update_leds();
    }
}

void DMA1_Channel1_IRQHandler(void) {
    HRTIM1->sMasterRegs.MCR &= ~HRTIM_MCR_MCEN;
    GPIOA->BSRR = (1<<1);
    DMA1->IFCR = DMA_IFCR_CTCIF1;

    /* Note: We don't divide the mean by the number of samples here to maximize extracted data. We have enough bit
     * width anyway. */
    int mean1 = 0, sqmean1 = 0;
    int mean2 = 0, sqmean2 = 0;
    for (ssize_t i=2; i<oversampling+2; i++) {
        int entry = adc1_raw[i];
        mean1 += entry;
        sqmean1 += entry*entry;

        entry = adc2_raw[i];
        mean2 += entry;
        sqmean2 += entry*entry;
    }
    host_data.adc_stats[meas_pb_now].adc1_mean[sampling_phase] = mean1;
    host_data.adc_stats[meas_pb_now].adc1_var[sampling_phase] = sqmean1*oversampling - mean1*mean1;
    host_data.adc_stats[meas_pb_now].adc2_mean[sampling_phase] = mean2;
    host_data.adc_stats[meas_pb_now].adc2_var[sampling_phase] = sqmean2*oversampling - mean2*mean2;

    sampling_phase ++;
    if (sampling_phase == ADC_DELAY_WINDOW_SIZE) {
        sampling_phase = 0;
        meas_pb_now ++;

        if (meas_pb_now < _MEAS_PB_COUNT) {
            meas_pb_config();

        } else {
            GPIOC->BRR = 1<<2;
            debug_plot_hook();
            GPIOC->BSRR = 1<<2;

            meas_pb_now = 0;

            memset(&host_data.adc_stats, 0, sizeof(host_data.adc_stats));
            meas_pb_config();
        }
    }

    /* The stimulus pulse is a simple square waveform that has a 50 % duty cycle with a period of one master clock
     * period. The output is set on CMP2, and reset on CMP1. We adjust both at once such that the phase of the pulse
     * is controlled by sampling_phase, plus the offset from stim_offset.
     * 
     * We control the offset of the stimulus pulse instead of controlling the offset of the sampling pulse to keep the
     * phase offset between diode sampling and ADC sampling constant. If we controlled the sampling pulse offset, we
     * would be able to control the sampling pulse at the diode gate with the HRTIM's 1/32th clock cycle
     * super-resolution, but the ADC would always sample at a full clock cycle offset. This would introduce a variable
     * +/- 1/16th clock cycle offset between diode sampling and ADC sampling.
     *
     * Between the diode sampling gate and the ADC input there is an opamp which while well-compensated will still
     * take some time to settle. A variable ADC sampling offset into the settling time of the opamp will introduce an
     * error. We can avoid this by just moving the stimulus pulse instead.
     */
    HRTIM1->sTimerxRegs[2].CMP1xR = stim_offset*0x20 + sampling_phase; 
    HRTIM1->sTimerxRegs[2].CMP2xR = stim_offset*0x20 + HRTIM1->sMasterRegs.MPER/2 + sampling_phase;
    HRTIM1->sMasterRegs.MCR |= HRTIM_MCR_MCEN;
}

void update_leds() {
    int blink_flag = (sys_time_us % 1000000) > 100000;

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

void ADC1_2_IRQHandler() {
    ADC1->ISR = ADC_ISR_EOC;
    GPIOC->BSRR = (1<<10);
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
