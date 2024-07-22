
#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>

/* The IRQ header must be included before stm32_device.h since ST defines a bunch of messy macros there. */
#include <stm32_irqs.h> /* Header generated from stm32***_startup.s in Makefile */

#include <stm32g4xx.h>
#include <core_cm4.h>

#define DMA1_Channel       DMA1_Channel1
#define DMA_ISR_FLAGS_Pos(channel) (4 * ((channel) - 1))
#define DMA_ISR_FLAGS_CH(channel) (0xf << DMA_ISR_FLAGS_Pos(channel))

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define APB1_PRESC (1<<(APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos]))
#define AHB_PRESC  (1<<(AHBPrescTable[(RCC->CFGR & RCC_CFGR_HPRE_Msk) >> RCC_CFGR_HPRE_Pos]))

#define HSE_VALUE 8000000
#define HSI_VALUE 16000000

#define AFRL(pin, val) ((val) << ((pin)*4))
#define AFRH(pin, val) ((val) << (((pin)-8)*4))
#define AF(pin) (2<<(2*(pin)))
#define OUT(pin) (1<<(2*(pin)))
#define IN(pin) (0)
#define ANALOG(pin) (3<<(2*(pin)))
#define CLEAR(pin) (3<<(2*(pin)))
#define PULLUP(pin) (1<<(2*pin))
#define PULLDOWN(pin) (2<<(2*pin))
#define BSRR_CLEAR(pin) ((1<<pin)<<16)
#define BSRR_SET(pin) (1<<pin)
#define BSRR_VALUE(pin, value) ((((!(value))<<pin)<<16) | ((!!(value))<<pin))

#ifndef SYSTICK_INTERVAL_US
#define SYSTICK_INTERVAL_US 10000
#endif /* SYSTICK_INTERVAL_US */


enum ErrorCode {
    ERR_SUCCESS = 0,
    ERR_TIMEOUT,
    ERR_PHYSICAL_LAYER,
    ERR_FRAMING,
    ERR_PROTOCOL,
    ERR_DMA,
    ERR_BUSY,
    ERR_BUFFER_OVERFLOW,
    ERR_RX_OVERRUN,
    ERR_TX_OVERRUN,
};

typedef enum ErrorCode ErrorCode;

enum board_config {
    /* The board assumes one of three configurations depending on connected periphery.
     *
     */

    BCFG_DISPLAY,
    /* When an I2C 1602 display is connected to the LCD connector on powerup, the board assumes its display
     * configuration. In this configuration, the board scans the buttons connected on the Buttons connector and acts as
     * a peripheral on the RS-485 bus, relaying information between the bus and the HCI peripherals. In addition to the
     * LCD, the board controls three 8-digit 7-segment LED displays connected on the Buttons connector along with the
     * buttons.
     *
     * RS-485 board address: BADDR_DISPLAY
     */

    BCFG_MOTOR,
    /* When no LCD is connected and the BT0 input on the buttons connector is open, the board assumes its
     * motor configuration. In this configuration, the board controls a motor connected through the Buttons and LCD
     * connectors, and the USB control interface is enabled. The board acts as the host on the RS-485 bus. 
     *
     * RS-485 board address: BADDR_MOTOR
     */

    BCFG_MEAS,
    /* When no LCD is connected and the BT0 input on the buttons connector is tied to ground, the board assumes its
     * senesor configuration. In this configuration, the board periodically. The board acts as a peripheral on the
     * RS-485 bus, relaying measurements on the bus when requested by the host. The board's bus address is set by pins
     * BT1 and BT0 of the buttons connector. Both pins have pullups enabled, and will read zero when tied to ground on
     * the connector. The address is the binary value of {BT2, BT1} added to BADDR_MES_BASE. With both pins open, the
     * address is 11 (decimal), with BT1 tied to ground it is 10 (decimal).
     *
     * RS-485 board address: BADDR_MEAS_BASE + [BT2:1]
     */
};

enum board_addr {
    BADDR_DISPLAY = 1,
    BADDR_MOTOR = 2,
    BADDR_MEAS_BASE = 8,
};

void delay_us(int duration_us);
uint64_t get_sync_time(void);

extern enum board_config board_config;
extern uint64_t sys_time_us;
extern uint64_t sync_time_us;

#endif /* __GLOBAL_H__ */
