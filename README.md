# SX1278 lib for Linux

This is a lib for use SX1278 LoRa transceivers, tested on Ra-02 modules with Luckfox Pico Mini with buildroot image. This lib based on code from [this](https://github.com/sandeepmistry/arduino-LoRa) lib for Arduino frameworks.

This library use linux api for control GPIO pins (`/sys/class/gpio`) and for spi (`/dev/spidev0.0`).