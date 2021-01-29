# Wiimote Emulator

Emulates a Bluetooth Wii controller in software.

![Raspberry Pi 3 running the emulator in Raspbian](rpi_ss.png)

### Features

  - Emulate the Wiimote's many features and extensions
  - Allows use of different input devices (keyboard etc.)

### Build/Install

The following dependencies/packages are required (if not already installed):

  - libdbus-1-dev
  - libglib2.0-dev
  - libsdl1.2-dev
  - libbluetooth-dev

Run the Makefile to build the emulator:

  > make

### Using the Emulator

  > ./wmemulator

With no arguments, the emulator will listen for incoming connections (similar to
syncing a real Wiimote). Pressing the sync button on a Wii should cause it to
connect.

You can also supply the address of a Wii to directly connect to it as long as
you have connected to it before (or you change your device's address to the
address of a trusted Wiimote).

  > ./wmemulator XX:XX:XX:XX:XX:XX