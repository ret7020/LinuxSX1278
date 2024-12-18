#include "Lora.h"
#include "gpio.h"

void bitWrite(uint8_t *x, char n, char value)
{
    if (value)
        *x |= (1 << n);
    else
        *x &= ~(1 << n);
}

// #define DEBUG

LoRa::LoRa(const char *spiDevice, uint8_t chipSelectPin, uint8_t resetPin, uint8_t _dio0Pin)
{
#ifdef DEBUG
    printf("[SX1278] device: %s\n", spiDevice);
#endif
    spiDev = spiDevice;
    csPin = chipSelectPin;
    rstPin = resetPin;
    dio0Pin = _dio0Pin;
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

int LoRa::spiSend(uint8_t *tx, uint8_t *rx, uint8_t len)
{
    setPin(csPin, 0);
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .speed_hz = SPI_FREQ,
        .bits_per_word = 8,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        return -1;
    setPin(csPin, 1);
    return 0;
}

// LoRa low-level operations
int LoRa::readRegister(uint8_t address)
{
    uint8_t regNew = address & 0x7F;
    uint8_t tx[2] = {regNew, 0x00};
    uint8_t rx[2] = {0, 0};
    spiSend(tx, rx, 2);
    return rx[1];
}

void LoRa::writeRegister(uint8_t address, uint8_t value)
{
    uint8_t regNew = address | 0x80;
    uint8_t tx[2] = {regNew, value};
    uint8_t rx[2] = {0, 0};
    spiSend(tx, rx, 2);
}

void LoRa::sleep() { writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP); }

void LoRa::idle() { writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY); }

// LoRa user methods
int LoRa::begin(long frequency)
{

    spiInit();
    setPin(csPin, 1);

    // Reset

    // perform reset
    printf("[SX1278] setPin status: %d\n", setPin(56, 1));
    usleep(10000);
    setPin(56, 1);
    usleep(10000);

    uint8_t version = readRegister(0x42);

#ifdef DEBUG
    printf("[SX1278] Version: 0x%02X\n", version);
#endif
    if (version != 0x12)
        return -1; // SX1278 version mismatch

    sleep();
    setFrequency(frequency);
    _frequency = frequency;

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
    // setPin(csPin, 1);

#ifdef DEBUG
    printf("[SX1278] Pushed into idle mode\n");
#endif

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
#ifdef DEBUG
    printf("[SX1278] Set freq: %ld\n", frequency);
#endif
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

bool LoRa::isTransmitting()
{
    if ((readRegister(REG_OP_MODE) & MODE_TX) == MODE_TX)
    {
        return true;
    }

    if (readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK)
    {
        // clear IRQ's
        writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
    }

    return false;
}

int LoRa::beginPacket(int implicitHeader)
{
    if (isTransmitting())
    {
        return 0;
    }

    // put in standby mode
    idle();

    if (implicitHeader)
    {
        implicitHeaderMode();
    }
    else
    {
        explicitHeaderMode();
    }

    // reset FIFO address and paload length
    writeRegister(REG_FIFO_ADDR_PTR, 0);
    writeRegister(REG_PAYLOAD_LENGTH, 0);

    return 1;
}

size_t LoRa::write(uint8_t byte)
{
    return write(&byte, sizeof(byte));
}

size_t LoRa::write(const uint8_t *buffer, size_t size)
{
    int currentLength = readRegister(REG_PAYLOAD_LENGTH);

    // check size
    if ((currentLength + size) > MAX_PKT_LENGTH)
    {
        size = MAX_PKT_LENGTH - currentLength;
    }

    // write data
    for (size_t i = 0; i < size; i++)
    {
        writeRegister(REG_FIFO, buffer[i]);
    }

    // update length
    writeRegister(REG_PAYLOAD_LENGTH, currentLength + size);

    return size;
}

int LoRa::endPacket(bool async)
{
    if ((async) && (_onTxDone))
        writeRegister(REG_DIO_MAPPING_1, 0x40); // DIO0 => TXDONE

    // put in TX mode
    writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

    if (!async)
    {
        // wait for TX done
        while ((readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0)
        {
        }
        // clear IRQ's
        writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
    }
#ifdef DEBUG
    printf("[SX1278] Sent packet\n");
#endif

    return 1;
}

void LoRa::setSpreadingFactor(int sf)
{
    if (sf < 6)
    {
        sf = 6;
    }
    else if (sf > 12)
    {
        sf = 12;
    }

    if (sf == 6)
    {
        writeRegister(REG_DETECTION_OPTIMIZE, 0xc5);
        writeRegister(REG_DETECTION_THRESHOLD, 0x0c);
    }
    else
    {
        writeRegister(REG_DETECTION_OPTIMIZE, 0xc3);
        writeRegister(REG_DETECTION_THRESHOLD, 0x0a);
    }

    writeRegister(REG_MODEM_CONFIG_2, (readRegister(REG_MODEM_CONFIG_2) & 0x0f) | ((sf << 4) & 0xf0));
    setLdoFlag();

#ifdef DEBUG
    printf("[SX1278] Set SF=%d\n", sf);
#endif
}

int LoRa::getSpreadingFactor()
{
    return readRegister(REG_MODEM_CONFIG_2) >> 4;
}

void LoRa::setSignalBandwidth(long sbw)
{
    int bw;

    if (sbw <= 7.8E3)
        bw = 0;
    else if (sbw <= 10.4E3)
        bw = 1;
    else if (sbw <= 15.6E3)
        bw = 2;
    else if (sbw <= 20.8E3)
        bw = 3;
    else if (sbw <= 31.25E3)
        bw = 4;
    else if (sbw <= 41.7E3)
        bw = 5;
    else if (sbw <= 62.5E3)
        bw = 6;
    else if (sbw <= 125E3)
        bw = 7;
    else if (sbw <= 250E3)
        bw = 8;
    else
        bw = 9;

    writeRegister(REG_MODEM_CONFIG_1, (readRegister(REG_MODEM_CONFIG_1) & 0x0f) | (bw << 4));
    setLdoFlag();

#ifdef DEBUG
    printf("[SX1278] Set BW=%d\n", bw);
#endif
}

long LoRa::getSignalBandwidth()
{
    uint8_t bw = (readRegister(REG_MODEM_CONFIG_1) >> 4);

    switch (bw)
    {
    case 0:
        return 7.8E3;
    case 1:
        return 10.4E3;
    case 2:
        return 15.6E3;
    case 3:
        return 20.8E3;
    case 4:
        return 31.25E3;
    case 5:
        return 41.7E3;
    case 6:
        return 62.5E3;
    case 7:
        return 125E3;
    case 8:
        return 250E3;
    case 9:
        return 500E3;
    }

    return -1;
}

void LoRa::setLdoFlag()
{
    // Section 4.1.1.5
    long symbolDuration = 1000 / (getSignalBandwidth() / (1L << getSpreadingFactor()));

    // Section 4.1.1.6
    bool ldoOn = symbolDuration > 16;

    uint8_t config3 = readRegister(REG_MODEM_CONFIG_3);
    bitWrite(&config3, 3, ldoOn);
    writeRegister(REG_MODEM_CONFIG_3, config3);
}

void LoRa::implicitHeaderMode()
{
    _implicitHeaderMode = 1;

    writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) | 0x01);

#ifdef DEBUG
    printf("[SX1278] Set ImplicitHeaderMode\n");
#endif
}

void LoRa::explicitHeaderMode()
{
    _implicitHeaderMode = 0;

    writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);

#ifdef DEBUG
    printf("[SX1278] Set ExplicitHeaderMode\n");
#endif
}

int LoRa::parsePacket(int size)
{
    int packetLength = 0;
    int irqFlags = readRegister(REG_IRQ_FLAGS);

    if (size > 0)
    {
        implicitHeaderMode();
        writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
    }
    else
    {
        explicitHeaderMode();
    }

    writeRegister(REG_IRQ_FLAGS, irqFlags);

    if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0)
    {
        _packetIndex = 0;

        if (_implicitHeaderMode)
            packetLength = readRegister(REG_PAYLOAD_LENGTH);
        else
            packetLength = readRegister(REG_RX_NB_BYTES);

        // set FIFO address to current RX address
        writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));

        // put in standby mode
        idle();
    }
    else if (readRegister(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE))
    {
        // not currently in RX mode

        // reset FIFO address
        writeRegister(REG_FIFO_ADDR_PTR, 0);

        // put in single RX mode
        writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
#ifdef DEBUG
        printf("[SX1278] Putting into RX mode done \n");
#endif
    }

    return packetLength;
}

int LoRa::available()
{
    return (readRegister(REG_RX_NB_BYTES) - _packetIndex);
}

int LoRa::read()
{
    if (!available())
        return -1;

    _packetIndex++;

    return readRegister(REG_FIFO);
}

void LoRa::onReceive(void (*callback)(int))
{
    // TODO not ported yet
    _onReceive = callback;

    if (callback)
    {
        exportPin(dio0Pin);
        setDirPin(dio0Pin, "in");
        setInterruptType(dio0Pin, "rising");

        char valuePath[50];
        snprintf(valuePath, sizeof(valuePath), "/sys/class/gpio/gpio%d/value", dio0Pin);
        dio0_fd = open(valuePath, O_RDONLY | O_NONBLOCK);

        if (dio0_fd < 0)
            printf("[SX1278] Err: Unable to open GPIO value file\n");

        dio0_pfd.fd = dio0_fd;
        dio0_pfd.events = POLLPRI;

        // Read first "init interrupt"
        char buf[3];
        lseek(dio0_fd, 0, SEEK_SET);
        ::read(dio0_fd, buf, sizeof(buf)); // poll.h read
    }
    else
    {
        unexportPin(dio0Pin);
    }
}

void LoRa::rxTick()
{
    int ret = poll(&dio0_pfd, 1, -1);
    if (ret > 0)
    {
        char buf[3];
        lseek(dio0_fd, 0, SEEK_SET);
        ::read(dio0_fd, buf, sizeof(buf)); // poll.h read
        int irqFlags = readRegister(REG_IRQ_FLAGS);

        // clear IRQ's
        writeRegister(REG_IRQ_FLAGS, irqFlags);

        if ((irqFlags & IRQ_CAD_DONE_MASK) != 0)
        {
            // TODO CAD not imlemented yet
        }
        else if ((irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0)
        {

            if ((irqFlags & IRQ_RX_DONE_MASK) != 0)
            {
                // received a packet
                _packetIndex = 0;

                // read packet length
                int packetLength = _implicitHeaderMode ? readRegister(REG_PAYLOAD_LENGTH) : readRegister(REG_RX_NB_BYTES);

                // set FIFO address to current RX address
                writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));

                if (_onReceive)
                {
                    _onReceive(packetLength);
                }
            }
            // else if ((irqFlags & IRQ_TX_DONE_MASK) != 0) // TODO maybe it works, but not tested yet
            // {
            //     if (_onTxDone)
            //     {
            //         _onTxDone();
            //     }
            // }
        }
    }
}

void LoRa::receive(int size)
{

    writeRegister(REG_DIO_MAPPING_1, 0x00); // DIO0 => RXDONE

    if (size > 0)
    {
        implicitHeaderMode();
        writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
    }
    else
    {
        explicitHeaderMode();
    }

    writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
}

int LoRa::packetRssi()
{
    return (readRegister(REG_PKT_RSSI_VALUE) - (_frequency < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT));
}

void LoRa::setSyncWord(int sw)
{
    writeRegister(REG_SYNC_WORD, sw);
}

int LoRa::end()
{
    sleep();
    return close(spi_fd);
}
