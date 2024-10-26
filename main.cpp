#include "Lora.h"

#define SPI_DEVICE "/dev/spidev0.0"
#define CS_PIN 49
#define DEFAULT_RF_FREQ 433E6

int main()
{
    LoRa lora(SPI_DEVICE, CS_PIN);
    if (!lora.begin(DEFAULT_RF_FREQ)) printf("Lora init - OK\n");
    else
    {
        printf("Lora init - FAIL\n");
        return -1;
    }

    uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    lora.setSignalBandwidth(62.5E3);
    lora.setSpreadingFactor(9);
    for (int i = 0; i < 100; i++){
        printf("Writing packet %d\n", i);
        lora.beginPacket();
        lora.write(data, 10);
        lora.endPacket();
        usleep(300000); 
    }
        

    lora.end();

    return 0;
}