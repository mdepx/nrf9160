# nRF9160

Nordicsemi nRF9160 is an ARM Cortex-M33 SiP (system in package) with support for LTE-M, NB-IoT and GPS.

Note: The latest firmware (version 1.2.0) is required.

For nRF9160-DK connect micro usb cable to J4, for other boards connect UART pins as follows:

| nRF9160          | UART-to-USB adapter  |
| ----------------- | -------------------- |
| P0.29 (TX)        | RX                   |
| P0.28 (RX)        | TX                   |

UART baud rate: 115200

Connect SWD pins as follows:

| nRF9160           | Segger JLINK adapter |
| ----------------- | -------------------- |
| SWDIO             | SWDIO                |
| SWDCLK            | SWCLK                |
| Ground            | Ground               |

This app depends on the [secure bootloader for nRF9160](https://github.com/machdep/nrf9160-boot).

### Under Linux
    $ sudo apt install gcc-arm-linux-gnueabi
    $ export CROSS_COMPILE=arm-linux-gnueabi-

### Under FreeBSD
    $ sudo pkg install arm-none-eabi-gcc arm-none-eabi-binutils
    $ export CROSS_COMPILE=arm-none-eabi-

### Build
    $ git clone --recursive https://github.com/machdep/nrf9160
    $ cd nrf9160
    $ make

## Program the chip using nrfjprog
    $ nrfjprog -f NRF91 --erasepage 0x40000-0x90000
    $ nrfjprog -f NRF91 --program obj/nrf9160.hex -r

## Program the chip using OpenOCD

### Build openocd
    $ sudo apt install pkg-config autotools-dev automake libtool
    $ git clone https://github.com/bukinr/openocd-nrf9160
    $ cd openocd-nrf9160
    $ ./bootstrap
    $ ./configure --enable-jlink
    $ make

### Invoke openocd
    $ export OPENOCD_PATH=/path/to/openocd-nrf9160
    $ sudo ${OPENOCD_PATH}/src/openocd -s ${OPENOCD_PATH}/tcl \
      -f interface/jlink.cfg -c 'transport select swd; adapter_khz 1000' \
      -f target/nrf9160.cfg -c "program nrf9160.elf 0 reset verify exit"

![alt text](https://raw.githubusercontent.com/machdep/nrf9160/master/images/nrf9160-dk.jpg)
![alt text](https://raw.githubusercontent.com/machdep/nrf9160/master/images/md009.jpg)
