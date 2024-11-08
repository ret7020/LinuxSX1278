# SX1278 lib for Linux

This is a lib for use SX1278 LoRa transceivers, tested on Ra-02 modules with Luckfox Pico Mini with buildroot image. This lib based on code from [this](https://github.com/sandeepmistry/arduino-LoRa) lib for Arduino frameworks.

This library use linux api for control GPIO pins (`/sys/class/gpio`) and for spi (`/dev/spidev0.0`).


## About RX

You can recieve new data via SPI polling:

```c
int packetSize = lora.parsePacket();
if (packetSize)
{
    // read packet
    while (lora.available()) ...
}
```

but it is not good for CPU and long range (some EMI can affect module sensitive), so you can use interrupts on DIO0 pin.

For now lib use edge interrupts, so it is not real interrupts