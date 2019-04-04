# nRF9160

Nordicsemi nRF9160 is an ARM Cortex-M33 SiP (system in package) with LTE modem.

For nRF9160-DK connect micro usb cable to J4, for other boards connect UART pins as follows:

| nRF9160          | UART-to-USB adapter  |
| ----------------- | -------------------- |
| P0.29 (TX)        | RX                   |

UART baud rate: 115200

Connect SWD pins as follows:

| nRF9160           | Segger JLINK adapter |
| ----------------- | -------------------- |
| SWDIO             | SWDIO                |
| SWDCLK            | SWCLK                |
| Ground            | Ground               |

This app depends on the [secure bootloader for nRF9160](https://github.com/osfive/nrf9160-boot).

### Under Linux
    $ sudo apt install gcc-arm-linux-gnueabi
    $ export CROSS_COMPILE=arm-linux-gnueabi-
### Under FreeBSD
    $ sudo pkg install arm-none-eabi-gcc arm-none-eabi-binutils
    $ export CROSS_COMPILE=arm-none-eabi-
### Build
    $ git clone --recursive https://github.com/osfive/nrf9160
    $ cd nrf9160
    $ make

### Build openocd
    $ git clone https://github.com/bukinr/openocd-nrf9160
    $ cd openocd-nrf9160
    $ ./bootstrap
    $ ./configure --enable-jlink
    $ gmake

### Program the chip
    $ export OPENOCD_PATH=/path/to/openocd-nrf9160
    $ sudo ${OPENOCD_PATH}/src/openocd -s ${OPENOCD_PATH}/tcl \
      -f interface/jlink.cfg -c 'transport select swd; adapter_khz 1000' \
      -f target/nrf9160.cfg -c "program nrf9160.elf 0 reset verify exit"

![alt text](https://raw.githubusercontent.com/osfive/nrf9160/master/images/nrf9160-dk.jpg)
![alt text](https://raw.githubusercontent.com/osfive/nrf9160/master/images/nrf9160.jpg)
