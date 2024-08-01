# nRF9160

Nordicsemi nRF9160 is an ARM Cortex-M33 SiP (system in package) with support for LTE-M, NB-IoT and GPS.

This demo app establishes a connection to an HTTP server using Nordic BSD sockets.

This app does not require zephyr, instead it uses [MDEPX RTOS](https://github.com/machdep/mdepx).

Note: The latest modem firmware (version 1.3.3) is required.
Note: This was tested on nRF9160-DK v1.1.0 and nRF9161-DK v1.0.0.

For nRF9160-DK connect micro usb cable to J4 usb socket.

This app depends on the [secure bootloader for nRF9160](https://github.com/machdep/nrf9160-boot).

### Set up your environment
    $ sudo apt install gcc-arm-linux-gnueabi
    $ export CROSS_COMPILE=arm-linux-gnueabi-

### Get sources and build the project
    $ git clone --recursive https://github.com/machdep/nrf9160
    $ cd nrf9160
    $ make clean all

## Program the chip using nrfjprog
    $ make dtb
    $ make flash

![alt text](https://raw.githubusercontent.com/machdep/nrf9160/master/images/nrf9160dk_030123.jpg)

See the [firmware](https://github.com/machdep/md009) for an alternative board:
![alt text](https://raw.githubusercontent.com/machdep/nrf9160/master/images/md009.jpg)
