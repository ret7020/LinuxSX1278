#include "Lora.h"

#define SPI_DEVICE "/dev/spidev0.0"
#define CS_PIN 49
#define DEFAULT_RF_FREQ 433E6

int main()
{
    LoRa lora(SPI_DEVICE, CS_PIN);
    if (!lora.begin(DEFAULT_RF_FREQ))
    {
        printf("Lora init - OK\n");
    }
    else
    {
        printf("Lora init - FAIL\n");
        return -1;
    }
        lora.beginPacket();
        uint8_t data = 100;
        lora.write(data);
        lora.endPacket();
    
    // for (int i = 0; i <= 1000; ++i)
    // {
    //     printf("Writing packet %d\n", i);
    //     lora.beginPacket();
    //     uint8_t data = 100;
    //     lora.write(data);
    //     lora.endPacket();

    //     usleep(100000);
    // }

    lora.end();

    return 0;
}