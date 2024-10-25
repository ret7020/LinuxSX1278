#ifndef LORA_H
#define LORA_H

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SPI_FREQ 500000

class LoRa
{
private:
    const char* spiDev;
    uint8_t csPin;
public:
    LoRa(const char* spiDevice, uint8_t csPin);
    int begin(long frequency);
};

#endif