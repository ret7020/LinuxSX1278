#include "Lora.h"

#define SPI_DEVICE "/dev/spidev0.0"
#define CS_PIN 49
#define DEFAULT_RF_FREQ 433E6
#define SIGNAL_SF 125E3
#define SIGNAL_BW 12
// #define TX_CHECK

int main()
{
	LoRa lora(SPI_DEVICE, CS_PIN);
	if (!lora.begin(DEFAULT_RF_FREQ))
		printf("Lora init - OK\n");
	else
	{
		printf("Lora init - FAIL\n");
		return -1;
	}
	// lora.setSignalBandwidth(SIGNAL_SF);
	// lora.setSpreadingFactor(SIGNAL_BW);
	// lora.setSyncWord(0x12);
	// lora.writeRegister(REG_DIO_MAPPING_1, 0x00);

#ifdef TX_CHECK
	uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	for (int i = 0; i < 100; i++)
	{
		printf("Writing packet %d\n", i);
		lora.beginPacket();
		lora.write(10);
		lora.endPacket();
		usleep(300000);
	}

#else
	while (1){
		int packetSize = lora.parsePacket();
		if (packetSize)
		{
			// read packet
			while (lora.available())
			{
				printf("%c", (char)lora.read());
			}
			printf("\n");

			// print RSSI of packet
			printf("RSSI: %d\n", lora.packetRssi());
		}

	}

#endif

	lora.end();

	return 0;
}
