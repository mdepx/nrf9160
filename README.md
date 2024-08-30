# nRF9160

Nordicsemi nRF9160 is an ARM Cortex-M33 SiP (system in package) with support for LTE-M, NB-IoT and GPS.

This demo app establishes a connection to an HTTP server using Nordic BSD sockets.

This app does not require zephyr, instead it uses [MDEPX RTOS](https://github.com/machdep/mdepx).

Note: The latest modem firmware (version 1.3.3) is required.

Note: This was tested on nRF9160-DK v1.1.0 and nRF9161-DK v1.0.0.

For nRF9160-DK connect micro usb cable to J4 usb socket.

For nRF9161-DK use USB-C connector.

This app depends on the [secure bootloader for nRF9160](https://github.com/machdep/nrf9160-boot).

### Set up DT tools
    $ sudo apt install device-tree-compiler python3-intelhex

### Set up compiler
    $ sudo apt install gcc-arm-none-eabi
    $ export CROSS_COMPILE=arm-none-eabi-

### Get sources and build the project
    $ git clone --recursive https://github.com/machdep/nrf9160
    $ cd nrf9160
    $ make clean all

## Install nrfjprog

Download from nordicsemi.com, then (ubuntu example):

    $ sudo dpkg -i ./nrf-command-line-tools_10.24.2_amd64.deb

## Program the chip using nrfjprog
    $ make dtb
    $ make flash

![alt text](https://raw.githubusercontent.com/machdep/nrf9160/master/images/nrf9160dk_030123.jpg)

See the [firmware](https://github.com/machdep/md009) for an alternative board:
![alt text](https://raw.githubusercontent.com/machdep/nrf9160/master/images/md009.jpg)
