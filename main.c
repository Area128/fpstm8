#include "config.h"
#include "ram.h"
#include "diskio.h"

static uint8_t CRC;
static uint8_t f_ram[128];
static uint8_t rx_buffer[BLOCK_SIZE];
static volatile uint8_t RAM_SEG_LEN;
static void (*flash_write_block)(uint16_t addr, const uint8_t *buf) =
        (void (*)(uint16_t, const uint8_t *)) f_ram;

/**
 * Write RAM_SEG section length into RAM_SEG_LEN
 */
inline void get_ram_section_length() {
    __asm__("mov _RAM_SEG_LEN, #l_RAM_SEG");
}

/**
 * Calculate CRC-8-CCIT.
 * Polynomial: x^8 + x^2 + x + 1 (0x07)
 *
 * @param data input byte
 * @param crc initial CRC
 * @return CRC value
 */
inline uint8_t crc8_update(uint8_t data, uint8_t crc) {
    crc ^= data;
    for (uint8_t i = 0; i < 8; i++)
        crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
    return crc;
}

/**
 * Enter bootloader and perform firmware update
 */
inline void copy_to_flash(uint16_t len) {
    /* unlock flash */
    FLASH_PUKR = FLASH_PUKR_KEY1;
    FLASH_PUKR = FLASH_PUKR_KEY2;
    while (!(FLASH_IAPSR & (1 << FLASH_IAPSR_PUL)));

    /* get main firmware */
    for (uint16_t i = 0; i < len; i += BLOCK_SIZE) {
        disk_readp (rx_buffer, BIN_SECTOR + (i >> 9), i & 0x1ff, BLOCK_SIZE);
        flash_write_block(BOOT_ADDR + i, rx_buffer);
    }

    /* lock flash */
    FLASH_IAPSR &= ~(1 << FLASH_IAPSR_PUL);

#if AUTO_REBOOT 
    /* reboot */
    IWDG_KR = IWDG_KEY_ENABLE;
#endif
    /* or just wait */
    PB_ODR &= ~(1 << 5);

    for (;;) ;
}

/**
 * Copy ram_flash_write_block routine into RAM
 */
inline void ram_cpy() {
    get_ram_section_length();
    for (uint8_t i = 0; i < RAM_SEG_LEN; i++)
        f_ram[i] = ((uint8_t *) ram_flash_write_block)[i];
}

inline void bootloader_check() {
    PB_DDR |= (1 << 5);
    PB_CR1 |= (1 << 5);
    PB_ODR |= (1 << 5);

#ifdef BOOT_ON_PIN_STATE
    #if BOOT_ON_PIN_STATE
        if (!(BOOT_PIN_IDR & (1 << BOOT_PIN))) return;
    #else
		#if BOOT_PIN_PULLUP
			BOOT_PIN_CR1 = 1 << BOOT_PIN; 	// enable pullup
			dly_100us(); 					// wait for a definite pin state
			if ((BOOT_PIN_IDR & (1 << BOOT_PIN))) {
				// BOOT_PIN_CR1 = 0x00;
				return;
			}
		#else
	        if ((BOOT_PIN_IDR & (1 << BOOT_PIN))) return;
		#endif
    #endif
#endif

    if (disk_initialize()) return;                          // disk init failed

    // read header sector from card (len, CRC etc)
    if (disk_readp (rx_buffer, HDR_SECTOR, 0, BLOCK_SIZE)) return;   // disk read failed

    // Header format: "LEN\nXXCRC\nY\0DST\nAA"
	// LEN: 0-5
	// CRC: 6-11
	// DST: 12-17
    // Length is LE int16_t in bytes 4 & 5 
    // uint16_t len = (rx_buffer[5] << 8) + rx_buffer[4]; 
	// same as line above but compiles 1 byte shorter
	uint16_t len = rx_buffer[5];
	len <<= 8;
	len += rx_buffer[4];
    // if (len > 0xA000 - BOOT_ADDR) return;                   // file too big

    // Destination address is LE int16_t in bytes 16 & 17 
    // uint16_t dest_addr = (rx_buffer[17] << 8) + rx_buffer[16];
	uint16_t dest_addr = rx_buffer[17];
	dest_addr <<= 8;
	dest_addr += rx_buffer[16];
    if (dest_addr != BOOT_ADDR) return;  	                // wrong boot address

#if SKIP_SAME_BINARY
    // CRC is LE int8_t in byte 11
    BYTE crc_in = rx_buffer[10];

    CRC = 0;
    for (uint16_t i = 0; i < len; i++) {
        CRC = crc8_update(_MEM_(BOOT_ADDR + i), CRC);
    }

    // if CRC's match, boot
    if (crc_in == CRC) return;                                 // we already have this version
#endif
    PB_ODR &= ~(1 << 5);

    // point of no return
    ram_cpy();
    copy_to_flash(len);
}

void bootloader_main() {

    CLK_CKDIVR = 0; // CPU clock prescaler = 1 (full speed)

    bootloader_check();

    // PB_ODR &= ~(1 << 5);

    BOOT();
    for (;;) ;
}
