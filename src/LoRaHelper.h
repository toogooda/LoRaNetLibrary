#pragma once
#include <Arduino.h>
#include <SPI.h>

// Item 1: SPI Clock Speed (Option B: Architecture Auto-Detection)
#ifndef SPI_Speed
  #if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
    #define SPI_Speed 2000000 // 2 MHz for ESP32 Gateway
  #else
    #define SPI_Speed 500000  // 500 kHz for ATmega644PA Field Nodes
  #endif
#endif

// Item 2: ON_AIR_TIMEOUT and BUSY_WAIT Header Macros
#ifndef ON_AIR_TIMEOUT
#define ON_AIR_TIMEOUT 1000 // 1000 ms max transmit timeout
#endif

#ifndef BUSY_WAIT
#define BUSY_WAIT 5000      // 5000 ms max BUSY wait timeout
#endif

// Item 6: Centralized Network RF Parameters (Single Source of Truth)
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

// Item 3: Crystal Frequency & Steps (including DualPIR macro)
#define XTAL_FREQ                       ( double )32000000
#define FREQ_DIV                        ( double )pow( 2.0, 25.0 )
#define FREQ_DIV_2_25                   FREQ_DIV
#define FREQ_STEP                       ( double )( XTAL_FREQ / FREQ_DIV_2_25 )

// SX126x Register Map & Opcodes
#define SX126X_CMD_SET_SLEEP            0x84
#define SX126X_CMD_SET_STANDBY          0x80
#define SX126X_CMD_SET_FS               0xC1
#define SX126X_CMD_SET_TX               0x83
#define SX126X_CMD_SET_RX               0x82
#define SX126X_CMD_STOP_TIMER_ON_PREAMBLE 0x9F
#define SX126X_CMD_SET_RX_DUTY_CYCLE    0x94
#define SX126X_CMD_SET_CAD              0xC5
#define SX126X_CMD_SET_TX_CONTINUOUS_WAVE 0xD1
#define SX126X_CMD_SET_TX_INFINITE_PREAMBLE 0xD2
#define SX126X_CMD_SET_REGULATOR_MODE   0x96
#define SX126X_CMD_CALIBRATE            0x89
#define SX126X_CMD_CALIBRATE_IMAGE      0x98
#define SX126X_CMD_SET_PA_CONFIG        0x95
#define SX126X_CMD_SET_RX_TX_FALLBACK_MODE 0x93

#define SX126X_CMD_WRITE_REGISTER       0x0D
#define SX126X_CMD_READ_REGISTER        0x1D
#define SX126X_CMD_WRITE_BUFFER         0x0E
#define SX126X_CMD_READ_BUFFER          0x1E

#define SX126X_CMD_SET_DIO_IRQ_PARAMS   0x08
#define SX126X_CMD_GET_IRQ_STATUS       0x12
#define SX126X_CMD_CLEAR_IRQ_STATUS     0x02
#define SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL 0x9D
#define SX126X_CMD_SET_DIO3_AS_TCXO_CTRL 0x97

#define SX126X_CMD_SET_RF_FREQUENCY     0x86
#define SX126X_CMD_SET_PACKET_TYPE      0x8A
#define SX126X_CMD_GET_PACKET_TYPE      0x11
#define SX126X_CMD_SET_TX_PARAMS        0x8E
#define SX126X_CMD_SET_MODULATION_PARAMS 0x8B
#define SX126X_CMD_SET_PACKET_PARAMS    0x8C
#define SX126X_CMD_GET_RX_BUFFER_STATUS 0x14
#define SX126X_CMD_GET_PACKET_STATUS    0x14
#define SX126X_CMD_GET_RSSI_INST        0x15
#define SX126X_CMD_GET_STATS            0x10
#define SX126X_CMD_RESET_STATS          0x00
#define SX126X_CMD_GET_DEVICE_ERRORS    0x17
#define SX126X_CMD_CLEAR_DEVICE_ERRORS  0x07

#define SX126X_REG_OCP                  0x08E7
#define SX126X_REG_IQ_POLARITY_SETUP    0x0736
#define SX126X_REG_TX_CLAMP_CONFIG      0x08D8
#define SX126X_REG_RTC_CONTROL          0x0902

#define SX126X_SYNC_WORD_PUBLIC         0x3444
#define SX126X_SYNC_WORD_PRIVATE        0x1424

#define ERR_NONE                        0
#define ERR_INVALID_READ                -1
#define ERR_INVALID_WRITE               -2

struct PortValue {
  char type[2];
  uint16_t value;
};

class SX126x {
public:
  SX126x(int spiSelect, int reset, int busy, int txen = -1, int rxen = -1);
  int16_t begin(uint32_t frequency = RF_FREQUENCY, int8_t txPower = TX_OUTPUT_POWER, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);
  int16_t beginFarmDefaults();
  int16_t LoRaConfig(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint16_t preambleLength, uint8_t payloadLen, bool crcOn = true, bool invertIrq = false);
  
  int16_t Send(uint8_t *pData, uint8_t len, uint8_t mode = 0);
  int16_t Receive(uint8_t *pData, uint8_t len);
  
  void SetRxEnable();
  void SetTxEnable();
  void SetRx(uint32_t timeout = 0xFFFFFF);
  void SetTx(uint32_t timeoutInMs = 1000);
  
  uint8_t GetPacketStatus(int8_t *rssiPacket, int8_t *snrPacket);
  uint8_t GetRxBufferStatus(uint8_t *payloadLen, uint8_t *startAddress);
  uint16_t GetIrqStatus();
  void ClearIrqStatus(uint16_t irqMask);
  
  void DebugPrint(bool debug);
  bool ReceiveMode();

private:
  int _spiSelect;
  int _reset;
  int _busy;
  int _txen;
  int _rxen;
  bool debugPrint;
  uint8_t PacketParams[6];
  
  void WaitForIdle(unsigned long timeout = 5000, const char *text = "", bool stop = false);
  void WriteRegister(uint16_t reg, uint8_t *data, uint8_t len);
  void ReadRegister(uint16_t reg, uint8_t *data, uint8_t len);
  void WriteBuffer(uint8_t *data, uint8_t len, uint8_t offset = 0);
  void ReadBuffer(uint8_t *data, uint8_t len, uint8_t offset = 0);
  void WriteCommand(uint8_t cmd, uint8_t *data, uint8_t len);
  void WriteCommand2(uint8_t cmd, uint8_t *data, uint8_t len);
  void ReadCommand(uint8_t cmd, uint8_t *data, uint8_t len);
  void FixInvertedIQ(uint8_t invertIQ);
  void SetPaConfig(uint8_t paDutyCycle, uint8_t hpDutyCycle, uint8_t deviceSelect, uint8_t paLut);
  void SetOvercurrentProtection(float currentLimit);
};

// Item 9: Standard 128-Byte Buffer Size
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
  
  // Item 10, 11, 12: Source MAC & Modification Methods
  uint8_t getFromByte(const uint8_t byteNumber) const;
  void getFromAddress(uint8_t* address) const;
  bool setPortValue(const char type[2], uint16_t newValue);
  uint16_t getMessageID() const;
  
  void printMessage() const;
};

// Item 5: Global lora Transceiver Instance for Gateway Multi-File Arch
extern SX126x lora;
