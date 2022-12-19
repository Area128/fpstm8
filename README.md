# fpstm8 - Field Programmable STM8
An SD card bootloader for STM8S and STM8L microcontrollers. This bootloader is heavily based on the Serial bootloader by lujji. Original repository [here](https://github.com/lujji/stm8-bootloader) and his excellent write-up [here](https://lujji.github.io/blog/serial-bootloader-for-stm8).

The low level SD control code comes from the [PetitFS project](http://elm-chan.org/fsw/ff/00index_p.html).

Ideal for distributing firmware updates for STM8 based creations that (can) have a microSD socket.

## Features

* **small** - ~980 - ~1200 bytes (16-19 blocks) depending on configuration
* **fast** - uploads 4k binary in 1 second (500kHz SPI clock)
* **configurable** - can be adjusted for parts with different flash block size

## Configuration

The default configuration targets low-density devices (STM8S003). To compile for a different target `MCU` and `FAMILY` makefile variables are adjusted accordingly.

Bootloader configuration is located in `config.h`.
* **BLOCK_SIZE** - flash block size according to device datasheet. This should be set to 64 for low-density devices or 128 for devices with >8k flash.
* **BOOT_ADDR** - application boot address.
* **HDR_SECTOR** - SD card sector that holds the header (see SD Image section)
* **BIN_SECTOR** - SD card sector that holds the binary (see SD Image section)
* **SKIP_SAME_BINARY** - Safety check to stop unnecessary flash writes if the binary is already copied
* **SD_FINISH_LAST_READ** - More robust SD initialisation procedure
* **SUPPORT_OLD_CARDS** - Enable support for SDv1 & MMC cards
* **AUTO_REBOOT** - Reboot when update is complete, else wait for reset
* **BOOT_PIN** - entry jumper with selectable entry state.
* **CS_PIN** - SD card SPI CS pin.
* **~~RELOCATE_IVT~~** - Write-protection cannot be used with the RELOCATE_IVT option and since this is aimed at remote updates where the bootloader should be kept safe, it has been removed. 

### The boot process

The bootloader will do a normal boot unless all of the following checks (in this order) succeed:

* If BOOT_ON_PIN_STATE is defined, it will **check** the state of the BOOT_PIN. The Card Detect pin of the SD socket can be used as the BOOT_PIN but the appropriate SKIP_SAME_BINARY and AUTO_REBOOT settings should be used to avoid the device endlessly rewriting its flash if the card is left in the socket.
* It will **try** to initialise the SD card. This obviously fails if there is no SD card present and can also fail for a multitude of other reasons, most obvious one being unsupported SD card version. Suggested fix would be "try another card".
* It will **try** to read the header from the card. 
* It will **check** that the BOOT_ADDR found in the header matches its configuration. This is a safety net against incompatible "new" and "old" bootloader versions in the wild.
* If SKIP_SAME_BINARY is enabled, it will compare the CRC of the installed firmware with the one found in the header and **fail if they match**.

This is the point of no return. If it makes it this far, it will copy the binary from the SD card to the flash and reboot if AUTO_REBOOT is enabled or wait for a reset.

### Changing boot address
Boot address must be a multiple of BLOCK_SIZE. Address is set in 2 places:
 * config.h
 * init.s

Main application is compiled with `--code-loc <address>` option. 

## Build instructions
Build and flash the bootloader:

``` bash
$ make && make flash
```

Enable write-protection (UBC) on pages 0-19 _(TODO: must be adjusted for STML)_. Hint: Change the 2nd byte to adjust for the final size of your bootloader:

``` bash
$ make opt-set
```

## Uploading the firmware

There is a demo application inside `app` directory which toggles PD4 via interrupts as well as a utility to create a disk image for distribution. The utility (util/img_maker.sh) requires sudo access for setting up a loop device and python3 with the crc module installed.

* Update the Makefile to match the settings of your bootloader (BOOT_ADDR, HDR_SECTOR & BIN_SECTOR).
* Compile the application
* Create a disk image
* Write image to SD card
* Update the device according to the procedure defined by the bootloader settings

``` bash
$ cd app && make
$ make image
$ dd if=firmware.img of=/dev/sdXXX bs=8M
```

The image maker will create a new sparse disk image. The size of the image can be configured in the Makefile. It will create an MBR and a FAT partition and will copy the contents of the img_content folder to that partition. 

It will also calculate a header for the compiled application and will add it to the HDR_SECTOR sector.

It will finally copy the compiled application to the BIN_SECTOR sector.

A typical disk has the MBR on sector 0 and the first partition on sector 2048. This, very conveniently, leaves 2047 unused 512-byte sectors between the MBR and the first partition. Sectors 3 & 4 respectively, are the default settings in both the bootloader and the sample application. 

The image produced, will look like any other SD card with a FAT partition to any other device. The bootloader is configured to look for the hidden data in the unused space.


## TODO
* **Test**. This code has barely been tested on an STM8S. More extensive testing before releasing it in the wild, would be wise.
* **Simplify**. To streamline the development of the main app, a utility that combines the bootloader and app binaries for SWIM programming, would be handy.
* **Optimise**. The size of the bootloader can probably be reduced further. Besides re-writing parts in assembly, an obvious example would be the CRC calculation. The calculation could be skipped replaced with a single flash read if a unique identifier was stored in a fixed location or at the end of the app binary.
