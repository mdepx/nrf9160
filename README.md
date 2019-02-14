# nRF9160

Nordicsemi nRF9160 is an ARM Cortex M33 SiP (system in package) with LTE modem.

Connect UART pins as follows:

| nRF9160          | UART-to-USB adapter  |
| ----------------- | -------------------- |
| P0.21 (RX)        | TX                   |
| P0.22 (TX)        | RX                   |

UART baud rate: 115200

Connect SWD pins as follows:

| nRF9160           | Segger JLINK adapter |
| ----------------- | -------------------- |
| SWDIO             | SWDIO                |
| SWDCLK            | SWCLK                |
| Ground            | Ground               |

### Check out repository
    $ export CROSS_COMPILE=arm-none-eabi-
    $ git clone --recursive https://github.com/osfive/nrf9160
    $ cd nrf9160

### Build under Linux
    $ bmake

### Build under FreeBSD
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

![alt text](https://raw.githubusercontent.com/osfive/nrf9160/master/images/nrf9160.jpg)
