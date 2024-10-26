#ifndef LORA_H
#define LORA_H

#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SPI_FREQ 500000


/* SX1278 defaults */

// Registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_OCP                  0x0b
#define REG_LNA                  0x0c
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_RX_NB_BYTES          0x13
#define REG_PKT_SNR_VALUE        0x19
#define REG_PKT_RSSI_VALUE       0x1a
#define REG_RSSI_VALUE           0x1b
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_MODEM_CONFIG_3       0x26
#define REG_FREQ_ERROR_MSB       0x28
#define REG_FREQ_ERROR_MID       0x29
#define REG_FREQ_ERROR_LSB       0x2a
#define REG_RSSI_WIDEBAND        0x2c
#define REG_DETECTION_OPTIMIZE   0x31
#define REG_INVERTIQ             0x33
#define REG_DETECTION_THRESHOLD  0x37
#define REG_SYNC_WORD            0x39
#define REG_INVERTIQ2            0x3b
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42
#define REG_PA_DAC               0x4d

// Modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05
#define MODE_RX_SINGLE           0x06
#define MODE_CAD                 0x07

// PA config
#define PA_BOOST                 0x80

// IRQ masks
#define IRQ_TX_DONE_MASK           0x08
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20
#define IRQ_RX_DONE_MASK           0x40
#define IRQ_CAD_DONE_MASK          0x04
#define IRQ_CAD_DETECTED_MASK      0x01

#define RF_MID_BAND_THRESHOLD    525E6
#define RSSI_OFFSET_HF_PORT      157
#define RSSI_OFFSET_LF_PORT      164

#define MAX_PKT_LENGTH           255

#define PA_OUTPUT_RFO_PIN          0
#define PA_OUTPUT_PA_BOOST_PIN     1


class LoRa
{
private:
    const char* spiDev;
    uint8_t csPin;
    int spi_fd;
    long _frequency;
    int _implicitHeaderMode;
    void (*_onTxDone)();


    int spiInit();
    int spiSend(uint8_t *tx, uint8_t *rx, uint8_t len);
public:
    LoRa(const char* spiDevice, uint8_t csPin);
    int begin(long frequency);
    int end();

    int readRegister(uint8_t reg);
    void writeRegister(uint8_t address, uint8_t value);

    void sleep();
    void idle();
    void setOCP(uint8_t mA);
    void setFrequency(long frequency);
    void setTxPower(int level, int outputPin=PA_OUTPUT_PA_BOOST_PIN);
    bool isTransmitting();
    int beginPacket(int implicitHeader=false);
    int endPacket(bool async=false);

    void setSignalBandwidth(long sbw);
    long getSignalBandwidth();
    int getSpreadingFactor();
    void setSpreadingFactor(int sf);
    void setLdoFlag();
    size_t write(uint8_t byte);
    size_t write(const uint8_t *buffer, size_t size);


    void implicitHeaderMode();
    void explicitHeaderMode();

};

#endif