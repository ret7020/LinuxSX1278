/*
This is a simple SX1278 register reader to check wiring and setup. This example just read version register (0x42) and prints it. Value have to be 0x12
*/

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GPIO_CS_PIN 49
#define SPI_DEVICE "/dev/spidev0.0"
#define VERSION_REG 0x42
#define SPI_FREQ 500000

void delay_ns(long nanoseconds)
{
	struct timespec req = {0, nanoseconds};
	nanosleep(&req, NULL);
}


int spi_init(int *spi_fd)
{
	uint8_t mode = SPI_MODE_0;
	uint32_t speed = SPI_FREQ;
	uint8_t bits = 8;

	*spi_fd = open(SPI_DEVICE, O_RDWR);
	if (*spi_fd < 0)
	{
		perror("Unable to open SPI device");
		return -1;
	}
	ioctl(*spi_fd, SPI_IOC_WR_MODE, &mode);
	ioctl(*spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	ioctl(*spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	return 0;
}

uint8_t sx1278_read_register(int spi_fd, uint8_t reg)
{
	uint8_t tx[2] = {reg & 0x7F, 0x00};
	uint8_t rx[2] = {0, 0};

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = 2,
		.speed_hz = SPI_FREQ,
		.bits_per_word = 8,
	};

	if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
	{
		perror("SPI transfer failed");
		return 0;
	}
	return rx[1];
}

int main()
{
	int spi_fd;

	gpio_export(GPIO_CS_PIN);
	gpio_set_direction(GPIO_CS_PIN, "out");
	if (spi_init(&spi_fd) < 0)
	{
		fprintf(stderr, "Failed to initialize SPI\n");
		return 1;
	}

	gpio_write(GPIO_CS_PIN, 0);
	delay_ns(100);

	uint8_t version = sx1278_read_register(spi_fd, VERSION_REG);

	delay_ns(100);
	gpio_write(GPIO_CS_PIN, 1);

	printf("SX1278 Version: 0x%02X\n", version);

	close(spi_fd);

	return 0;
}
