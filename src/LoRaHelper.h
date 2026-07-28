#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <math.h>

// Option B: Architecture Auto-Detection for SPI Speed
#ifndef SPI_Speed
  #if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    #define SPI_Speed 2000000 // 2 MHz for ESP32 Gateway
  #else
    #define SPI_Speed 500000  // 500 kHz for ATmega644PA Field Nodes
  #endif
#endif

#ifndef ON_AIR_TIMEOUT
#define ON_AIR_TIMEOUT 1000 // 1000 ms max transmit timeout
#endif

#ifndef BUSY_WAIT
#define BUSY_WAIT 5000      // 5000 ms max BUSY wait timeout
#endif

// Centralized Network RF Parameters (Single Source of Truth)
#ifndef RF_FREQUENCY
#define RF_FREQUENCY          915000000UL // 915 MHz Center Frequency
#endif
#ifndef TX_OUTPUT_POWER
#define TX_OUTPUT_POWER       22          // 22 dBm Tx Output Power
#endif
#ifndef LORA_BANDWIDTH
#define LORA_BANDWIDTH        6           // 500 kHz Bandwidth
#endif
#ifndef LORA_SPREADING_FACTOR
#define LORA_SPREADING_FACTOR 11          // SF11
#endif
#ifndef LORA_CODINGRATE
#define LORA_CODINGRATE       1           // 4/5 Coding Rate
#endif
#ifndef LORA_PREAMBLE_LENGTH
#define LORA_PREAMBLE_LENGTH  8           // 8 Preamble Symbols
#endif
#ifndef LORA_PAYLOADLENGTH
#define LORA_PAYLOADLENGTH    0           // 0 = Variable Length (Explicit Header)
#endif

// Return values
#define ERR_NONE                        0
#define ERR_PACKET_TOO_LONG             1
#define ERR_UNKNOWN                     2
#define ERR_TX_TIMEOUT                  3
#define ERR_RX_TIMEOUT                  4
#define ERR_CRC_MISMATCH                5
#define ERR_WRONG_MODEM                 6
#define ERR_INVALID_BANDWIDTH           7
#define ERR_INVALID_SPREADING_FACTOR    8
#define ERR_INVALID_CODING_RATE         9
#define ERR_INVALID_FREQUENCY_DEVIATION 10
#define ERR_INVALID_BIT_RATE            11
#define ERR_INVALID_RX_BANDWIDTH        12
#define ERR_INVALID_DATA_SHAPING        13
#define ERR_INVALID_SYNC_WORD           14
#define ERR_INVALID_OUTPUT_POWER        15
#define ERR_INVALID_MODE                16
#define ERR_INVALID_TRANCEIVER          17

// SX126X physical layer properties
#define XTAL_FREQ                       ( double )32000000
#define FREQ_DIV                        ( double )pow( 2.0, 25.0 )
#define FREQ_DIV_2_25                   FREQ_DIV
#define FREQ_STEP                       ( double )( XTAL_FREQ / FREQ_DIV_2_25 )

// SX126X SPI commands
#define SX126X_CMD_NOP                                0x00
#define SX126X_CMD_SET_SLEEP                          0x84
#define SX126X_CMD_SET_STANDBY                        0x80
#define SX126X_CMD_SET_FS                             0xC1
#define SX126X_CMD_SET_TX                             0x83
#define SX126X_CMD_SET_RX                             0x82
#define SX126X_CMD_STOP_TIMER_ON_PREAMBLE             0x9F
#define SX126X_CMD_SET_RX_DUTY_CYCLE                  0x94
#define SX126X_CMD_SET_CAD                            0xC5
#define SX126X_CMD_SET_TX_CONTINUOUS_WAVE             0xD1
#define SX126X_CMD_SET_TX_INFINITE_PREAMBLE           0xD2
#define SX126X_CMD_SET_REGULATOR_MODE                 0x96
#define SX126X_CMD_CALIBRATE                          0x89
#define SX126X_CMD_CALIBRATE_IMAGE                    0x98
#define SX126X_CMD_SET_PA_CONFIG                      0x95
#define SX126X_CMD_SET_RX_TX_FALLBACK_MODE            0x93

// register and buffer access commands
#define SX126X_CMD_WRITE_REGISTER                     0x0D
#define SX126X_CMD_READ_REGISTER                      0x1D
#define SX126X_CMD_WRITE_BUFFER                       0x0E
#define SX126X_CMD_READ_BUFFER                        0x1E

// DIO and IRQ control
#define SX126X_CMD_SET_DIO_IRQ_PARAMS                 0x08
#define SX126X_CMD_GET_IRQ_STATUS                     0x12
#define SX126X_CMD_CLEAR_IRQ_STATUS                   0x02
#define SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL         0x9D
#define SX126X_CMD_SET_DIO3_AS_TCXO_CTRL              0x97

// RF, modulation and packet commands
#define SX126X_CMD_SET_RF_FREQUENCY                   0x86
#define SX126X_CMD_SET_PACKET_TYPE                    0x8A
#define SX126X_CMD_GET_PACKET_TYPE                    0x11
#define SX126X_CMD_SET_TX_PARAMS                      0x8E
#define SX126X_CMD_SET_MODULATION_PARAMS              0x8B
#define SX126X_CMD_SET_PACKET_PARAMS                  0x8C
#define SX126X_CMD_SET_CAD_PARAMS                     0x88
#define SX126X_CMD_SET_BUFFER_BASE_ADDRESS            0x8F
#define SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT          0xA0

#define SX126X_PA_CONFIG_SX1261                       0x01
#define SX126X_PA_CONFIG_SX1262                       0x00

// status commands
#define SX126X_CMD_GET_STATUS                         0xC0
#define SX126X_CMD_GET_RSSI_INST                      0x15
#define SX126X_CMD_GET_RX_BUFFER_STATUS               0x13
#define SX126X_CMD_GET_PACKET_STATUS                  0x14
#define SX126X_CMD_GET_DEVICE_ERRORS                  0x17
#define SX126X_CMD_CLEAR_DEVICE_ERRORS                0x07
#define SX126X_CMD_GET_STATS                          0x10
#define SX126X_CMD_RESET_STATS                        0x00

// SX126X register map
#define SX126X_REG_IQ_POLARITY_SETUP                  0x0736
#define SX126X_REG_LORA_SYNC_WORD_MSB                 0x0740
#define SX126X_REG_LORA_SYNC_WORD_LSB                 0x0741
#define SX126X_REG_OCP_CONFIGURATION                  0x08E7

#define SX126X_STANDBY_RC                             0x00
#define SX126X_STANDBY_XOSC                           0x01

#define SX126X_RX_TIMEOUT_NONE                        0x000000
#define SX126X_RX_TIMEOUT_INF                         0xFFFFFF

#define SX126X_TXMODE_SYNC                            0x00
#define SX126X_TXMODE_ASYNC                           0x01
#define SX126x_TXMODE_SYNC                            0x00
#define SX126x_TXMODE_ASYNC                           0x01

#define SX126X_PACKET_TYPE_LORA                       0x01

#define SX126X_LORA_BW_500                            0x06
#define SX126X_LORA_SF_11                             0x0B
#define SX126X_LORA_CR_4_5                            0x01

#define SX126X_LORA_HEADER_EXPLICIT                   0x00
#define SX126X_LORA_HEADER_IMPLICIT                   0x01

#define SX126X_LORA_CRC_OFF                           0x00
#define SX126X_LORA_CRC_ON                            0x01

#define SX126X_LORA_IQ_STANDARD                       0x00
#define SX126X_LORA_IQ_INVERTED                       0x01

#define SX126X_SYNC_WORD_PUBLIC                       0x3444
#define SX126X_SYNC_WORD_PRIVATE                      0x1424

#define SX126X_IRQ_TX_DONE                            0x0001
#define SX126X_IRQ_RX_DONE                            0x0002
#define SX126X_IRQ_PREAMBLE_DETECTED                  0x0004
#define SX126X_IRQ_SYNC_WORD_VALID                    0x0008
#define SX126X_IRQ_HEADER_VALID                       0x0010
#define SX126X_IRQ_HEADER_ERR                         0x0020
#define SX126X_IRQ_CRC_ERR                            0x0040
#define SX126X_IRQ_CAD_DONE                           0x0080
#define SX126X_IRQ_CAD_ACTIVITY_DETECTED              0x0100
#define SX126X_IRQ_TIMEOUT                            0x0200
#define SX126X_IRQ_ALL                                0x03FF
#define SX126X_IRQ_NONE                               0x0000

#define SX126X_PA_RAMP_200U                           0x02
#define SX126X_REGULATOR_LDO                          0x00
#define SX126X_REGULATOR_DC_DC                        0x01

#define SX126X_DIO3_OUTPUT_1_6                        0x00
#define SX126X_DIO3_OUTPUT_1_7                        0x01
#define SX126X_DIO3_OUTPUT_1_8                        0x02
#define SX126X_DIO3_OUTPUT_2_2                        0x03
#define SX126X_DIO3_OUTPUT_2_4                        0x04
#define SX126X_DIO3_OUTPUT_2_7                        0x05
#define SX126X_DIO3_OUTPUT_3_0                        0x06
#define SX126X_DIO3_OUTPUT_3_3                        0x07

#define RADIO_TCXO_SETUP_TIME                         320 // 320 * 15.625 us = 5 ms

#define SX126X_CALIBRATE_IMAGE_ON                     0x40
#define SX126X_CALIBRATE_ADC_BULK_P_ON                0x20
#define SX126X_CALIBRATE_ADC_BULK_N_ON                0x10
#define SX126X_CALIBRATE_ADC_PULSE_ON                 0x08
#define SX126X_CALIBRATE_PLL_ON                       0x04
#define SX126X_CALIBRATE_RC13M_ON                     0x02
#define SX126X_CALIBRATE_RC64K_ON                     0x01

struct PortValue {
  char type[2];
  uint16_t value;
};

class SX126x {
public:
  SX126x(int spiSelect, int reset, int busy, int txen = -1, int rxen = -1);
  int16_t begin(uint32_t frequencyInHz = RF_FREQUENCY, int8_t txPowerInDbm = TX_OUTPUT_POWER, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);
  int16_t beginFarmDefaults();
  int16_t Reset(void);
  int16_t SetStandby(uint8_t standbyConfig);
  int16_t SetSleep(uint8_t sleepConfig);
  int16_t SetTx(uint32_t timeout);
  int16_t SetRx(uint32_t timeout);
  int16_t SetPacketType(uint8_t packetType);
  int16_t SetRfFrequency(uint32_t frequencyInHz);
  int16_t SetTxParams(int8_t txPowerInDbm, uint8_t rampTime = 0x02);
  int16_t SetBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress);
  int16_t SetModulationParams(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint8_t lowDataRateOptimize = 0x00);
  int16_t SetPacketParams(uint16_t preambleLength, uint8_t headerType, uint8_t payloadLength, uint8_t crcType, uint8_t invertIQ = 0x00);
  int16_t SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask = 0, uint16_t dio3Mask = 0);
  uint16_t GetIrqStatus(void);
  int16_t ClearIrqStatus(uint16_t clearIrqParams);
  int16_t SetDio2AsRfSwitchCtrl(uint8_t enable);
  int16_t SetDio3AsTcxoCtrl(float voltage, uint32_t delay);
  int16_t WriteRegister(uint16_t reg, uint8_t* data, uint8_t numBytes);
  int16_t ReadRegister(uint16_t reg, uint8_t* data, uint8_t numBytes);
  int16_t WriteBuffer(uint8_t offset, uint8_t* data, uint8_t numBytes);
  int16_t ReadBuffer(uint8_t offset, uint8_t* data, uint8_t numBytes);
  int16_t GetRxBufferStatus(uint8_t* payloadLength, uint8_t* rxStartBufferPointer);
  int16_t GetPacketStatus(int8_t* rssiPacket, int8_t* snrPacket);
  int16_t Calibrate(uint8_t calibParam);
  int16_t SetPaConfig(uint8_t paDutyCycle, uint8_t hpMax, uint8_t deviceSelect = 0x00, uint8_t paLut = 0x01);
  void LoRaConfig(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint16_t preambleLength, uint8_t payloadLen, bool crcOn = true, bool invertIq = false);
  bool Send(uint8_t *data, uint8_t len, uint8_t txMode = SX126x_TXMODE_SYNC);
  uint8_t Receive(uint8_t *data, uint16_t len);
  void SPIwriteCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes);
  void SPIreadCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes);
  void SPIwriteCommand(uint8_t cmd);
  void FixInvertedIQ(uint8_t invertIQ);
  void SetPowerConfig(int8_t txPowerInDbm, uint8_t rampTime = 0x02);
  int16_t SetRegulatorMode(uint8_t mode);
  void SetRxEnable(void);
  void SetTxEnable(void);
  void DebugPrint(bool enable);
  uint8_t GetStatus(void);
  bool ReceiveMode(void);

private:
  int SX126x_SPI_SELECT;
  int SX126x_RESET;
  int SX126x_BUSY;
  int SX126x_TXEN;
  int SX126x_RXEN;

  bool txActive;
  bool debugPrint;
  uint8_t PacketParams[6];
  void WaitForIdle(unsigned long timeout = 5000, const char *text = "", bool stop = false);
};

// Binary Message Frame Serialization Class (128-Byte Standard Buffer)
class LoraMsg {
private:
  static const int MAX_MSG_SIZE = 128;
  uint8_t message[MAX_MSG_SIZE];
  int currentIndex;
  static uint16_t messageCounter;

public:
  LoraMsg(const uint8_t toAddress[6], const uint8_t fromAddress[6]);
  LoraMsg(const uint8_t* encryptedMsg, int length);
  
  bool addPortValue(const PortValue& portValue);
  bool addPortValue(const char type[2], uint16_t value);
  
  PortValue getPortValue(int index) const;
  int numberOfPortValues() const;
  
  void encryptMessage();
  void decryptMessage();
  
  uint8_t* getMessage() { return message; }
  const uint8_t* getMessage() const { return message; }
  uint8_t getMessageLength() const { return currentIndex; }
  
  bool isForMe(const uint8_t* myAddress) const;
  
  uint8_t getFromByte(const uint8_t byteNumber) const;
  void getFromAddress(uint8_t* address) const;
  bool setPortValue(const char type[2], uint16_t newValue);
  uint16_t getMessageID() const;
  
  void printMessage() const;
};

extern SX126x lora;
