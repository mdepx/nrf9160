# nRF9160

Nordicsemi nRF9160 is an ARM Cortex-M33 SiP (system in package) with support for LTE-M, NB-IoT and GPS.

This is a demo app that establishes a connection to a HTTP server using Nordic BSD sockets.

This app does not require zephyr, and uses [MDEPX](https://github.com/machdep/mdepx) rtos.

Note: The latest modem firmware (version 1.3.0) is required.

For nRF9160-DK connect micro usb cable to J4 usb socket.

This app depends on the [secure bootloader for nRF9160](https://github.com/machdep/nrf9160-boot).

### Set up your environment
    $ sudo apt install gcc-arm-linux-gnueabi
    $ export CROSS_COMPILE=arm-linux-gnueabi-

### Get sources and build the project
    $ git clone --recursive https://github.com/machdep/nrf9160
    $ cd nrf9160
    $ make

## Program the chip using nrfjprog
    $ make dtb
    $ make flash

![alt text](https://raw.githubusercontent.com/machdep/nrf9160/master/images/nrf9160-dk.jpg)

See the [firmware](https://github.com/machdep/md009) for an alternative board:
![alt text](https://raw.githubusercontent.com/machdep/nrf9160/master/images/md009.jpg)
