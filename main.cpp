#include "Lora.h"

#define SPI_DEVICE "/dev/spidev0.0"
#define CS_PIN 49
#define DEFAULT_RF_FREQ 433E6

int main(){
    LoRa lora(SPI_DEVICE, CS_PIN);
    if (!lora.begin(DEFAULT_RF_FREQ)){ printf("Lora init - OK\n"); } 
    else {
        printf("Lora init - FAIL\n");
        return -1;
    }

    

    return 0;
}