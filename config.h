#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "stm8.h"

/* application address (flash base address = 0x8000) */
/* The same address needs to be set in init.s */
#define BOOT_ADDR           0x8800

/* flash block size */
#define BLOCK_SIZE			64

/* SD card sector that holds the header */
#define HDR_SECTOR			3

/* SD card sector that holds the binary */
#define BIN_SECTOR			4

/* Skip update if the crc of the flash matches that on the header */
#define SKIP_SAME_BINARY	1	// +97 bytes

/* needed primarily during debug when SWIM resets can leave the SD card in an unknown state */
#define SD_FINISH_LAST_READ	1	// +6 bytes

/* Enable support for older cards */
#define SUPPORT_OLD_CARDS	1	// +103 bytes

/* 
	Reboot at the end of update 
	** WARNING ** 
	Potential for infinite loop of flash updates.
	Use with SKIP_SAME_BINARY or other combination that prevents 
	MCU from repeatedly writing to flash.
*/
#define AUTO_REBOOT			0	// +4 bytes

/* boot pin */
/*
 * Undefined = boot pin is disabled
 * 0 = Update on LOW
 * 1 = Update on HIGH
 * 
 * External pull up or down needed to prevent spurious updates
 */
#define BOOT_ON_PIN_STATE		0	// (ifdef) +5 bytes 

#if !BOOT_ON_PIN_STATE
	/* Use internal pull up */
	#define BOOT_PIN_PULLUP			1	// +7 bytes
#endif
#define BOOT_PIN				3
#define BOOT_PIN_PORT			PORTC

/* card CS pin */
#define CS_PIN				4
#define CS_PIN_PORT			PORTB


/* internal RC oscillator, CKDIVR = 0 */
#define F_CPU               16000000UL

/* not configured by user */
#define STR(x)              #x
#define STRX(x)             STR(x)
#define BOOT()              __asm__("jp " STRX(BOOT_ADDR))

/* SPI */
// pin definitions
#define SCK_PIN             5
#define MOSI_PIN            6
#define MISO_PIN            7

#define PORTA   PA_ODR
#define PORTB   PB_ODR
#define PORTC   PC_ODR
#define PORTD   PD_ODR

#define BOOT_PIN_IDR          *(&BOOT_PIN_PORT + 0x01)
#define BOOT_PIN_CR1          *(&BOOT_PIN_PORT + 0x03)

#define CS_PIN_ODR          CS_PIN_PORT
#define CS_PIN_IDR          *(&CS_PIN_PORT + 0x01)
#define CS_PIN_DDR          *(&CS_PIN_PORT + 0x02)
#define CS_PIN_CR1          *(&CS_PIN_PORT + 0x03)

#endif /* CONFIG_H */
