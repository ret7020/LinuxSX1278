#include "Lora.h"
#include "gpio.h"

LoRa::LoRa(const char *spiDevice, uint8_t chipSelectPin)
{
    printf("device: %s\n", spiDevice);
    spiDev = spiDevice;
    csPin = chipSelectPin;
}

int LoRa::spiInit()
{
    uint8_t mode = SPI_MODE_0;
    uint32_t speed = SPI_FREQ;
    uint8_t bits = 8;

    spi_fd = open(spiDev, O_RDWR);
    if (spi_fd < 0)
        return -1;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    return 0;
}

int LoRa::readRegister(uint8_t reg)
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

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) return 0;
    
    return rx[1];
}

int LoRa::begin(long frequency)
{
    spiInit();
    setPin(csPin, 0);
    usleep(10000);
    uint8_t version = readRegister(0x42);
    printf("SX1278 Version: 0x%02X\n", version);
    usleep(10000);
    setPin(csPin, 1);

    return 0;
}

int LoRa::end(){
    close(spi_fd);
}