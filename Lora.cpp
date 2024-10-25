#include "Lora.h"

LoRa::LoRa(const char* spiDevice, uint8_t csPin){
    printf("device: %s\n", spiDevice);
    spiDev = spiDevice;
    csPin = csPin;
}

int LoRa::begin(long frequency)
{
    printf("Begin");
    return 0;

}