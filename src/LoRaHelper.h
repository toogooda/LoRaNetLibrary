#pragma once
#include <Arduino.h>
#include <SPI.h>

// Hardware / Timeout Defaults
#ifndef SPI_Speed
#define SPI_Speed 500000
#endif

#ifndef ON_AIR_TIMEOUT
#define ON_AIR_TIMEOUT 1000 // ms to send packet
#endif

#ifndef BUSY_WAIT
#define BUSY_WAIT 5000
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

// SX126X Model
#define SX1261_TRANCEIVER                             0x01
#define SX1262_TRANCEIVER                             0x02
#define SX1268_TRANCEIVER                             0x08

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
#define SX126X_REG_HOPPING_ENABLE                     0x0385
#define SX126X_REG_PACKECT_LENGTH                     0x0386
#define SX126X_REG_NB_HOPPING_BLOCKS                  0x0387
#define SX126X_REG_NB_SYMBOLS0                        0x0388
#define SX126X_REG_FREQ0                              0x038A
#define SX126X_REG_NB_SYMBOLS15                       0x03E2
#define SX126X_REG_FREQ15                             0x03E4
#define SX126X_REG_DIOX_OUTPUT_ENABLE                 0x0580
#define SX126X_REG_DIOX_INPUT_ENABLE                  0x0583
#define SX126X_REG_DIOX_PILL_UP_CONTROL               0x0584
#define SX126X_REG_DIOX_PULL_DOWN_CONTROL             0x0585
#define SX126X_REG_WHITENING_INITIAL_MSB              0x06B8
#define SX126X_REG_WHITENING_INITIAL_LSB              0x06B9
#define SX126X_REG_CRC_INITIAL_MSB                    0x06BC
#define SX126X_REG_CRC_INITIAL_LSB                    0x06BD
#define SX126X_REG_CRC_POLYNOMIAL_MSB                 0x06BE
#define SX126X_REG_CRC_POLYNOMIAL_LSB                 0x06BF
#define SX126X_REG_SYNC_WORD_0                        0x06C0
#define SX126X_REG_SYNC_WORD_1                        0x06C1
#define SX126X_REG_SYNC_WORD_2                        0x06C2
#define SX126X_REG_SYNC_WORD_3                        0x06C3
#define SX126X_REG_SYNC_WORD_4                        0x06C4
#define SX126X_REG_SYNC_WORD_5                        0x06C5
#define SX126X_REG_SYNC_WORD_6                        0x06C6
#define SX126X_REG_SYNC_WORD_7                        0x06C7
#define SX126X_REG_NODE_ADDRESS                       0x06CD
#define SX126X_REG_BROADCAST_ADDRESS                  0x06CE
#define SX126X_REG_IQ_POLARITY_SETUP                  0x0736
#define SX126X_REG_LORA_SYNC_WORD_MSB                 0x0740
#define SX126X_REG_LORA_SYNC_WORD_LSB                 0x0741
#define SX126X_REG_RANDOM_NUMBER_0                    0x0786
#define SX126X_REG_RANDOM_NUMBER_1                    0x0787
#define SX126X_REG_RANDOM_NUMBER_2                    0x0788
#define SX126X_REG_RANDOM_NUMBER_3                    0x0789
#define SX126X_REG_TX_MODULATION                      0x0889
#define SX126X_REG_RX_GAIN                            0x08AC
#define SX126X_REG_TX_CLAMP_CONFIG                    0x08D8
#define SX126X_REG_DIO3_OUTPUT_VOLTAGE_CONTROL        0x0920
#define SX126X_REG_EVENT_MASK                         0x0944

#define SX126X_SLEEP_START_COLD                       0b00000000
#define SX126X_SLEEP_START_WARM                       0b00000100
#define SX126X_SLEEP_RTC_OFF                          0b00000000
#define SX126X_SLEEP_RTC_ON                           0b00000001

#define SX126X_STANDBY_RC                             0x00
#define SX126X_STANDBY_XOSC                           0x01

#define SX126X_REGULATOR_LDO                          0x00
#define SX126X_REGULATOR_DC_DC                        0x01

#define SX126X_CALIBRATE_IMAGE_OFF                    0b00000000
#define SX126X_CALIBRATE_IMAGE_ON                     0b00000001
#define SX126X_CALIBRATE_ADC_BULK_P_OFF               0b00000000
#define SX126X_CALIBRATE_ADC_BULK_P_ON                0b00000010
#define SX126X_CALIBRATE_ADC_BULK_N_OFF               0b00000000
#define SX126X_CALIBRATE_ADC_BULK_N_ON                0b00000100
#define SX126X_CALIBRATE_ADC_PULSE_OFF                0b00000000
#define SX126X_CALIBRATE_ADC_PULSE_ON                 0b00001000
#define SX126X_CALIBRATE_PLL_OFF                      0b00000000
#define SX126X_CALIBRATE_PLL_ON                       0b00010000
#define SX126X_CALIBRATE_RC13M_OFF                    0b00000000
#define SX126X_CALIBRATE_RC13M_ON                     0b00100000
#define SX126X_CALIBRATE_RC64K_OFF                    0b00000000
#define SX126X_CALIBRATE_RC64K_ON                     0b01000000
#define SX126X_CALIBRATE_ALL                          0b01111111

#define SX126X_CALIB_BAND_430_440                     0x6B
#define SX126X_CALIB_BAND_470_510                     0x75
#define SX126X_CALIB_BAND_779_787                     0xC1
#define SX126X_CALIB_BAND_863_870                     0xD7
#define SX126X_CALIB_BAND_902_928                     0xE1

#define SX126X_PA_RAMP_10U                            0x00
#define SX126X_PA_RAMP_20U                            0x01
#define SX126X_PA_RAMP_40U                            0x02
#define SX126X_PA_RAMP_80U                            0x03
#define SX126X_PA_RAMP_200U                           0x04
#define SX126X_PA_RAMP_800U                           0x05
#define SX126X_PA_RAMP_1700U                          0x06
#define SX126X_PA_RAMP_3400U                          0x07

#define SX126X_DIO3_OUTPUT_1_6                        0x00
#define SX126X_DIO3_OUTPUT_1_7                        0x01
#define SX126X_DIO3_OUTPUT_1_8                        0x02
#define SX126X_DIO3_OUTPUT_2_2                        0x03
#define SX126X_DIO3_OUTPUT_2_4                        0x04
#define SX126X_DIO3_OUTPUT_2_7                        0x05
#define SX126X_DIO3_OUTPUT_3_0                        0x06
#define SX126X_DIO3_OUTPUT_3_3                        0x07

#define RADIO_TCXO_SETUP_TIME                         5000

#define SX126X_PACKET_TYPE_GFSK                       0x00
#define SX126X_PACKET_TYPE_LORA                       0x01

#define SX126X_LORA_BW_7_8                            0x00
#define SX126X_LORA_BW_10_4                           0x08
#define SX126X_LORA_BW_15_6                           0x01
#define SX126X_LORA_BW_20_8                           0x09
#define SX126X_LORA_BW_31_25                          0x02
#define SX126X_LORA_BW_41_7                           0x0A
#define SX126X_LORA_BW_62_5                           0x03
#define SX126X_LORA_BW_125_0                          0x04
#define SX126X_LORA_BW_250_0                          0x05
#define SX126X_LORA_BW_500_0                          0x06

#define SX126X_LORA_SF_5                              0x05
#define SX126X_LORA_SF_6                              0x06
#define SX126X_LORA_SF_7                              0x07
#define SX126X_LORA_SF_8                              0x08
#define SX126X_LORA_SF_9                              0x09
#define SX126X_LORA_SF_10                             0x0A
#define SX126X_LORA_SF_11                             0x0B
#define SX126X_LORA_SF_12                             0x0C

#define SX126X_LORA_CR_4_5                            0x01
#define SX126X_LORA_CR_4_6                            0x02
#define SX126X_LORA_CR_4_7                            0x03
#define SX126X_LORA_CR_4_8                            0x04

#define SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_OFF        0x00
#define SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_ON         0x01

#define SX126X_LORA_HEADER_EXPLICIT                   0x00
#define SX126X_LORA_HEADER_IMPLICIT                   0x01

#define SX126X_LORA_CRC_OFF                           0x00
#define SX126X_LORA_CRC_ON                            0x01
#define SX126X_LORA_IQ_STANDARD                       0x00
#define SX126X_LORA_IQ_INVERTED                       0x01

#define SX126X_IRQ_TX_DONE                            0b0000000000000001
#define SX126X_IRQ_RX_DONE                            0b0000000000000010
#define SX126X_IRQ_PREAMBLE_DETECTED                  0b0000000000000100
#define SX126X_IRQ_SYNC_WORD_VALID                    0b0000000000001000
#define SX126X_IRQ_HEADER_VALID                       0b0000000000010000
#define SX126X_IRQ_HEADER_ERR                         0b0000000000100000
#define SX126X_IRQ_CRC_ERR                            0b0000000001000000
#define SX126X_IRQ_CAD_DONE                           0b0000000010000000
#define SX126X_IRQ_CAD_DETECTED                       0b0000000100000000
#define SX126X_IRQ_TIMEOUT                            0b0000001000000000
#define SX126X_IRQ_NONE                               0b0000000000000000
#define SX126X_IRQ_ALL                                0b0000001111111111

#define SX126X_STATUS_MODE_STDBY_RC                   0b00100000
#define SX126X_STATUS_MODE_STDBY_XOSC                 0b00110000
#define SX126X_STATUS_MODE_FS                         0b01000000
#define SX126X_STATUS_MODE_RX                         0b01010000
#define SX126X_STATUS_MODE_TX                         0b01100000
#define SX126X_STATUS_DATA_AVAILABLE                  0b00000100
#define SX126X_STATUS_CMD_TIMEOUT                     0b00000110
#define SX126X_STATUS_CMD_INVALID                     0b00001000
#define SX126X_STATUS_CMD_FAILED                      0b00001010
#define SX126X_STATUS_TX_DONE                         0b00001100
#define SX126X_STATUS_SPI_FAILED                      0b11111111

#define SX126X_SYNC_WORD_PUBLIC                       0x3444
#define SX126X_SYNC_WORD_PRIVATE                      0x1424

#define SX126x_TXMODE_ASYNC                           0x01
#define SX126x_TXMODE_SYNC                            0x02
#define SX126x_TXMODE_BACK2RX                         0x04

// Default Radio Parameters
#define RF_FREQUENCY 915000000    // Hz center frequency
#define TX_OUTPUT_POWER 22        // dBm tx output power
#define LORA_BANDWIDTH 6          // 500Khz
#define LORA_SPREADING_FACTOR 11  // SF11
#define LORA_CODINGRATE 1         // 4/5
#define LORA_PREAMBLE_LENGTH 8
#define LORA_PAYLOADLENGTH 0

// SX126x Driver Class
class SX126x {
public:
  SX126x(int spiSelect, int reset, int busy, int txen = -1, int rxen = -1);

  int16_t  begin(uint32_t frequencyInHz, int8_t txPowerInDbm, float tcxoVoltage = 0.0, bool useRegulatorLDO = false);
  void     LoRaConfig(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint16_t preambleLength, uint8_t payloadLen, bool crcOn, bool invertIrq);
  uint8_t  Receive(uint8_t *pData, uint16_t len);
  bool     Send(uint8_t *pData, uint8_t len, uint8_t mode);
  bool     ReceiveMode(void);
  void     GetPacketStatus(int8_t *rssiPacket, int8_t *snrPacket);
  void     SetTxPower(int8_t txPowerInDbm);
  uint32_t GetRandomNumber(void);
  void     DebugPrint(bool enable);

  void     SetRxEnable(void);
  void     SetTxEnable(void);
  void     SetRx(uint32_t timeout);
  void     SetTx(uint32_t timeoutInMs);
  void     WaitForIdle(unsigned long timeout = 5000, const char *text = "", bool stop = false);

  int16_t  Reset(void);
  void     Wakeup(void);
  void     SetSleep(uint8_t mode);
  void     SetStandby(uint8_t mode);
  void     SetRfFrequency(uint32_t frequency);
  void     Calibrate(uint8_t calibParam);
  void     CalibrateImage(uint32_t frequency);
  void     SetRegulatorMode(uint8_t mode);
  void     SetBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress);
  void     SetPowerConfig(int8_t power, uint8_t rampTime);
  void     SetOvercurrentProtection(float currentLimit);
  void     SetPaConfig(uint8_t paDutyCycle, uint8_t hpMax, uint8_t deviceSel, uint8_t paLut);
  void     SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask);
  void     SetStopRxTimerOnPreambleDetect(bool enable);
  void     SetLoRaSymbNumTimeout(uint8_t SymbNum);
  void     SetPacketType(uint8_t packetType);
  void     SetModulationParams(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint8_t lowDataRateOptimize);
  uint8_t  GetStatus(void);
  uint16_t GetIrqStatus(void);
  void     ClearIrqStatus(uint16_t irq);
  uint8_t  GetRssiInst();
  void     GetRxBufferStatus(uint8_t *payloadLength, uint8_t *rxStartBufferPointer);

  uint8_t  ReadBuffer(uint8_t *rxData, uint8_t maxLen);
  void     WriteBuffer(uint8_t *txData, uint8_t txDataLen);
  void     WriteRegister(uint16_t reg, uint8_t* data, uint8_t numBytes, bool waitForBusy = true);
  void     ReadRegister(uint16_t reg, uint8_t* data, uint8_t numBytes, bool waitForBusy = true);
  void     WriteCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy = true);
  uint8_t  WriteCommand2(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy = true);
  void     ReadCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy = true);
  void     FixInvertedIQ(uint8_t iqConfig);
  void     SetDio3AsTcxoCtrl(float voltage, uint32_t delay);
  void     SetDio2AsRfSwitchCtrl(uint8_t enable);

private:
  uint8_t  PacketParams[6] = {0};
  bool     txActive;
  bool     debugPrint;
  int      SX126x_SPI_SELECT;
  int      SX126x_RESET;
  int      SX126x_BUSY;
  int      SX126x_TXEN;
  int      SX126x_RXEN;
};

extern SX126x lora;

// Binary Protocol PortValue Struct
struct PortValue {
  char type[2];
  uint16_t value;
};

// Binary Message Frame Serialization Class
class LoraMsg {
private:
  static const int MAX_MSG_SIZE = 256;
  uint8_t message[MAX_MSG_SIZE] = { 0 };
  int currentIndex;
  static uint16_t messageCounter;
  uint8_t toAddress[6];

  void addAddress(const uint8_t* address);

public:
  LoraMsg(const uint8_t* toAddress, const uint8_t* fromAddress);
  LoraMsg(const uint8_t* encryptedMessage, byte sizeOfMsg);
  bool addPortValue(const PortValue& portValue);
  uint8_t* getMessage();
  uint8_t getMessageLength();
  PortValue getPortValue(int index);
  uint8_t numberOfPortValues();
  void printMessage();
  uint8_t getFromByte(const uint8_t byteNumber);
  void getFromAddress(uint8_t* address);
  bool setPortValue(const char type[2], uint16_t newValue);
  uint16_t getMessageID();
  void encryptMessage();
  void decryptMessage();
  bool isForMe(const uint8_t* address);
};
