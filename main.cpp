#include "Lora.h"

#define SPI_DEVICE "/dev/spidev0.0"
#define CS_PIN 49
#define DEFAULT_RF_FREQ 433E6
#define SIGNAL_SF 12
#define SIGNAL_BW 125E3
#define RST_PIN 56
#define DIO0_PIN 57

#define TX_CHECK

LoRa lora(SPI_DEVICE, CS_PIN, RST_PIN, DIO0_PIN);

void RxCallback(int packetSize) 
{
	for (int i = 0; i < packetSize; i++) printf("%c", (char)lora.read());
	printf("\n");
}


int main()
{
	if (!lora.begin(DEFAULT_RF_FREQ))
		printf("Lora init - OK\n");
	else
	{
		printf("Lora init - FAIL\n");
		return -1;
	}
	#ifndef TX_CHECK
	lora.receive(); // Set SX1278 into rising interrupt on DIO0 when RXDONE
	lora.onReceive(RxCallback);
	#endif
	lora.setSignalBandwidth(SIGNAL_BW);
	lora.setSpreadingFactor(SIGNAL_SF);
	// lora.setSyncWord(0x12);
	// lora.writeRegister(REG_DIO_MAPPING_1, 0x00);

#ifdef TX_CHECK
	uint8_t data[4] = {100, 200, 250, 0};
	uint8_t packetId = 0;
	while (1)
	{
		printf("Writing packet %d\n", data[3]);
		lora.beginPacket();
		lora.write(data, 4);
		lora.endPacket();
		if (data[3] >= 255) data[3] = 0;
		data[3]++;
		usleep(300000); // 3 seconds
	}

#else

	while (1)
	{
		lora.rxTick(); // handle user-space interrupts
	}


	// OLD - non interrupt reading
	// while (1)
	// {
	// 	int packetSize = lora.parsePacket();
	// 	if (packetSize)
	// 	{
	// 		// read packet
	// 		while (lora.available())
	// 		{
	// 			printf("%c", (char)lora.read());
	// 		}
	// 		printf("\n");

	// 		// print RSSI of packet
	// 		printf("RSSI: %d\n", lora.packetRssi());
	// 	}
	// }

#endif

	lora.end();

	return 0;
}
