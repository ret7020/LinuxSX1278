#include "Lora.h"
#include "gpio.h"

#define DEBUG

LoRa::LoRa(const char *spiDevice, uint8_t chipSelectPin)
{
#ifdef DEBUG
    printf("device: %s\n", spiDevice);
#endif
    spiDev = spiDevice;
    csPin = chipSelectPin;
}

// SPI functions
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

int LoRa::spiSend(uint8_t *tx, uint8_t *rx)
{
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = sizeof(tx) / sizeof(tx[0]),
        .speed_hz = SPI_FREQ,
        .bits_per_word = 8,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        return -1;
    return 0;
}

// LoRa low-level operations
int LoRa::readRegister(uint8_t address)
{
    uint8_t regNew = address & 0x7F;
    uint8_t tx[2] = {regNew, 0x00};
    uint8_t rx[2] = {0, 0};
    spiSend(tx, rx);
    return rx[1];
}

void LoRa::writeRegister(uint8_t address, uint8_t value)
{
    uint8_t regNew = address | 0x80;
    uint8_t tx[2] = {regNew, value};
    uint8_t rx[2] = {0, 0};
    spiSend(tx, rx);
}

void LoRa::sleep() { writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP); }

void LoRa::idle() { writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY); }

// LoRa user methods
int LoRa::begin(long frequency)
{

    // TODO impplement reset via RST pin
    spiInit();
    setPin(csPin, 0);
    usleep(10000);
    uint8_t version = readRegister(0x42);

#ifdef DEBUG
    printf("SX1278 Version: 0x%02X\n", version);
#endif
    if (version != 0x12)
        return -1; // SX1278 version mismatch

    sleep();
    setFrequency(frequency);

    // set base addresses
    writeRegister(REG_FIFO_TX_BASE_ADDR, 0);
    writeRegister(REG_FIFO_RX_BASE_ADDR, 0);

    // set LNA boost
    writeRegister(REG_LNA, readRegister(REG_LNA) | 0x03);

    // set auto AGC
    writeRegister(REG_MODEM_CONFIG_3, 0x04);

    // set output power to 17 dBm
    setTxPower(17);

    // put in standby mode
    idle();
    usleep(10000);
    setPin(csPin, 1);

    return 0;
}

void LoRa::setOCP(uint8_t mA)
{
    uint8_t ocpTrim = 27;

    if (mA <= 120)
        ocpTrim = (mA - 45) / 5;
    else if (mA <= 240)
        ocpTrim = (mA + 30) / 10;

    writeRegister(REG_OCP, 0x20 | (0x1F & ocpTrim));
}

void LoRa::setFrequency(long frequency)
{
    _frequency = frequency;

    uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

    writeRegister(REG_FRF_MSB, (uint8_t)(frf >> 16));
    writeRegister(REG_FRF_MID, (uint8_t)(frf >> 8));
    writeRegister(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

void LoRa::setTxPower(int level, int outputPin)
{
    if (PA_OUTPUT_RFO_PIN == outputPin)
    {
        // RFO
        if (level < 0)
        {
            level = 0;
        }
        else if (level > 14)
        {
            level = 14;
        }

        writeRegister(REG_PA_CONFIG, 0x70 | level);
    }
    else
    {
        // PA BOOST
        if (level > 17)
        {
            if (level > 20)
            {
                level = 20;
            }

            // subtract 3 from level, so 18 - 20 maps to 15 - 17
            level -= 3;

            // High Power +20 dBm Operation (Semtech SX1276/77/78/79 5.4.3.)
            writeRegister(REG_PA_DAC, 0x87);
            setOCP(140);
        }
        else
        {
            if (level < 2)
            {
                level = 2;
            }
            // Default value PA_HF/LF or +17dBm
            writeRegister(REG_PA_DAC, 0x84);
            setOCP(100);
        }

        writeRegister(REG_PA_CONFIG, PA_BOOST | (level - 2));
    }
}

int LoRa::end()
{
    sleep();
    return close(spi_fd);
}