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
    
    // lora.idle();
    printf("0x%02X\n", lora.readRegister(REG_MODEM_CONFIG_1) >> 4);
    // lora.sleep();
    printf("0x%02X\n", lora.readRegister(REG_MODEM_CONFIG_2) >> 4);
    
    uint8_t dat = 200;
    uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (int i = 0; i < 1000; i++){
        printf("Writing packet %d\n", i);
        lora.beginPacket();
        lora.write(data, 10);
        lora.endPacket();
        usleep(500000); 
    }
        
    
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