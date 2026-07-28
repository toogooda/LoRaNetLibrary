#include "LoRaHelper.h"

uint16_t LoraMsg::messageCounter = 1;

SX126x::SX126x(int spiSelect, int reset, int busy, int txen, int rxen) {
  SX126x_SPI_SELECT = spiSelect;
  SX126x_RESET = reset;
  SX126x_BUSY = busy;
  SX126x_TXEN = txen;
  SX126x_RXEN = rxen;
  txActive = false;
  debugPrint = false;
}

void SX126x::DebugPrint(bool enable) {
  debugPrint = enable;
}

void SX126x::WaitForIdle(unsigned long timeout, const char *text, bool stop) {
  unsigned long start = millis();
  delayMicroseconds(1);
  while (digitalRead(SX126x_BUSY)) {
    delayMicroseconds(1);
    if (millis() - start >= timeout) {
      if (debugPrint && text[0] != '\0') {
        Serial.print("BUSY Timeout during ");
        Serial.println(text);
      }
      if (stop) {
        while (1) { delay(1); }
      }
      break;
    }
  }
}

int16_t SX126x::Reset(void) {
  if (SX126x_RESET != -1) {
    pinMode(SX126x_RESET, OUTPUT);
    delay(10);
    digitalWrite(SX126x_RESET, LOW);
    delay(20);
    digitalWrite(SX126x_RESET, HIGH);
    delay(10);
    WaitForIdle(BUSY_WAIT, "Reset", true);
  }
  return ERR_NONE;
}

void SX126x::SetRxEnable(void) {
  if ((SX126x_TXEN != -1) && (SX126x_RXEN != -1)) {
    digitalWrite(SX126x_RXEN, HIGH);
    digitalWrite(SX126x_TXEN, LOW);
  }
}

void SX126x::SetTxEnable(void) {
  if ((SX126x_TXEN != -1) && (SX126x_RXEN != -1)) {
    digitalWrite(SX126x_RXEN, LOW);
    digitalWrite(SX126x_TXEN, HIGH);
  }
}

int16_t SX126x::SetDio2AsRfSwitchCtrl(uint8_t enable) {
  uint8_t data = enable;
  SPIwriteCommand(SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &data, 1);
  return ERR_NONE;
}

int16_t SX126x::SetDio3AsTcxoCtrl(float voltage, uint32_t delay) {
  uint8_t buf[4];
  if (fabs(voltage - 1.6) <= 0.001) {
    buf[0] = SX126X_DIO3_OUTPUT_1_6;
  } else if (fabs(voltage - 1.7) <= 0.001) {
    buf[0] = SX126X_DIO3_OUTPUT_1_7;
  } else if (fabs(voltage - 1.8) <= 0.001) {
    buf[0] = SX126X_DIO3_OUTPUT_1_8;
  } else if (fabs(voltage - 2.2) <= 0.001) {
    buf[0] = SX126X_DIO3_OUTPUT_2_2;
  } else if (fabs(voltage - 2.4) <= 0.001) {
    buf[0] = SX126X_DIO3_OUTPUT_2_4;
  } else if (fabs(voltage - 2.7) <= 0.001) {
    buf[0] = SX126X_DIO3_OUTPUT_2_7;
  } else if (fabs(voltage - 3.0) <= 0.001) {
    buf[0] = SX126X_DIO3_OUTPUT_3_0;
  } else {
    buf[0] = SX126X_DIO3_OUTPUT_3_3;
  }

  uint32_t delayValue = (float)delay / 15.625;
  buf[1] = (uint8_t)((delayValue >> 16) & 0xFF);
  buf[2] = (uint8_t)((delayValue >> 8) & 0xFF);
  buf[3] = (uint8_t)(delayValue & 0xFF);

  SPIwriteCommand(SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, buf, 4);
  return ERR_NONE;
}

int16_t SX126x::Calibrate(uint8_t calibParam) {
  uint8_t data = calibParam;
  SPIwriteCommand(SX126X_CMD_CALIBRATE, &data, 1);
  return ERR_NONE;
}

int16_t SX126x::SetRegulatorMode(uint8_t mode) {
  uint8_t data = mode;
  SPIwriteCommand(SX126X_CMD_SET_REGULATOR_MODE, &data, 1);
  return ERR_NONE;
}

int16_t SX126x::SetBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress) {
  uint8_t buf[2];
  buf[0] = txBaseAddress;
  buf[1] = rxBaseAddress;
  SPIwriteCommand(SX126X_CMD_SET_BUFFER_BASE_ADDRESS, buf, 2);
  return ERR_NONE;
}

int16_t SX126x::SetPaConfig(uint8_t paDutyCycle, uint8_t hpMax, uint8_t deviceSelect, uint8_t paLut) {
  uint8_t buf[4];
  buf[0] = paDutyCycle;
  buf[1] = hpMax;
  buf[2] = deviceSelect;
  buf[3] = paLut;
  SPIwriteCommand(SX126X_CMD_SET_PA_CONFIG, buf, 4);
  return ERR_NONE;
}

void SX126x::SetOvercurrentProtection(float currentLimit) {
  uint8_t raw = (uint8_t)(currentLimit / 2.5);
  WriteRegister(SX126X_REG_OCP_CONFIGURATION, &raw, 1);
}

void SX126x::SetPowerConfig(int8_t txPowerInDbm, uint8_t rampTime) {
  uint8_t buf[2];
  buf[0] = txPowerInDbm;
  buf[1] = rampTime;
  SPIwriteCommand(SX126X_CMD_SET_TX_PARAMS, buf, 2);
}

int16_t SX126x::SetRfFrequency(uint32_t frequencyInHz) {
  uint32_t frf = (uint32_t)((double)frequencyInHz / FREQ_STEP);
  uint8_t buf[4];
  buf[0] = (frf >> 24) & 0xFF;
  buf[1] = (frf >> 16) & 0xFF;
  buf[2] = (frf >> 8) & 0xFF;
  buf[3] = frf & 0xFF;
  SPIwriteCommand(SX126X_CMD_SET_RF_FREQUENCY, buf, 4);
  return ERR_NONE;
}

int16_t SX126x::SetStandby(uint8_t standbyConfig) {
  uint8_t data = standbyConfig;
  SPIwriteCommand(SX126X_CMD_SET_STANDBY, &data, 1);
  return ERR_NONE;
}

int16_t SX126x::SetSleep(uint8_t sleepConfig) {
  uint8_t data = sleepConfig;
  SPIwriteCommand(SX126X_CMD_SET_SLEEP, &data, 1);
  return ERR_NONE;
}

int16_t SX126x::SetPacketType(uint8_t packetType) {
  uint8_t data = packetType;
  SPIwriteCommand(SX126X_CMD_SET_PACKET_TYPE, &data, 1);
  return ERR_NONE;
}

int16_t SX126x::SetModulationParams(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint8_t lowDataRateOptimize) {
  uint8_t modParams[4];
  modParams[0] = spreadingFactor;
  modParams[1] = bandwidth;
  modParams[2] = codingRate;
  modParams[3] = lowDataRateOptimize;
  SPIwriteCommand(SX126X_CMD_SET_MODULATION_PARAMS, modParams, 4);
  return ERR_NONE;
}

int16_t SX126x::SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
  uint8_t buf[8];
  buf[0] = (irqMask >> 8) & 0xFF;
  buf[1] = irqMask & 0xFF;
  buf[2] = (dio1Mask >> 8) & 0xFF;
  buf[3] = dio1Mask & 0xFF;
  buf[4] = (dio2Mask >> 8) & 0xFF;
  buf[5] = dio2Mask & 0xFF;
  buf[6] = (dio3Mask >> 8) & 0xFF;
  buf[7] = dio3Mask & 0xFF;
  SPIwriteCommand(SX126X_CMD_SET_DIO_IRQ_PARAMS, buf, 8);
  return ERR_NONE;
}

void SX126x::FixInvertedIQ(uint8_t invertIQ) {
  uint8_t regVal = 0;
  ReadRegister(SX126X_REG_IQ_POLARITY_SETUP, &regVal, 1);
  if (invertIQ == 1) {
    regVal &= ~(1 << 2);
  } else {
    regVal |= (1 << 2);
  }
  WriteRegister(SX126X_REG_IQ_POLARITY_SETUP, &regVal, 1);
}

int16_t SX126x::begin(uint32_t frequencyInHz, int8_t txPowerInDbm, float tcxoVoltage, bool useRegulatorLDO) {
  pinMode(SX126x_SPI_SELECT, OUTPUT);
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  
  if (SX126x_RESET != -1) {
    pinMode(SX126x_RESET, OUTPUT);
  }
  if (SX126x_BUSY != -1) {
    pinMode(SX126x_BUSY, INPUT);
  }
  if (SX126x_TXEN != -1) pinMode(SX126x_TXEN, OUTPUT);
  if (SX126x_RXEN != -1) pinMode(SX126x_RXEN, OUTPUT);

  if (txPowerInDbm > 22) txPowerInDbm = 22;
  if (txPowerInDbm < -3) txPowerInDbm = -3;

  Reset();

  uint8_t wk[2];
  ReadRegister(SX126X_REG_LORA_SYNC_WORD_MSB, wk, 2);
  uint16_t syncWord = (wk[0] << 8) + wk[1];
  if (syncWord != SX126X_SYNC_WORD_PUBLIC && syncWord != SX126X_SYNC_WORD_PRIVATE) {
    return ERR_INVALID_MODE;
  }

  SetStandby(SX126X_STANDBY_RC);
  SetDio2AsRfSwitchCtrl(true);

  if (tcxoVoltage > 0.0) {
    SetDio3AsTcxoCtrl(tcxoVoltage, RADIO_TCXO_SETUP_TIME);
  }

  Calibrate(SX126X_CALIBRATE_IMAGE_ON
          | SX126X_CALIBRATE_ADC_BULK_P_ON
          | SX126X_CALIBRATE_ADC_BULK_N_ON
          | SX126X_CALIBRATE_ADC_PULSE_ON
          | SX126X_CALIBRATE_PLL_ON
          | SX126X_CALIBRATE_RC13M_ON
          | SX126X_CALIBRATE_RC64K_ON);

  if (useRegulatorLDO) {
    SetRegulatorMode(SX126X_REGULATOR_LDO);
  } else {
    SetRegulatorMode(SX126X_REGULATOR_DC_DC);
  }

  SetBufferBaseAddress(0, 0);
  SetPaConfig(0x04, 0x07, 0x00, 0x01);
  SetOvercurrentProtection(60.0);
  SetPowerConfig(txPowerInDbm, SX126X_PA_RAMP_200U);
  SetRfFrequency(frequencyInHz);
  return ERR_NONE;
}

int16_t SX126x::beginFarmDefaults() {
  int16_t err = begin(RF_FREQUENCY, TX_OUTPUT_POWER);
  if (err == ERR_NONE) {
    LoRaConfig(LORA_SPREADING_FACTOR, LORA_BANDWIDTH, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH, LORA_PAYLOADLENGTH, true, false);
  }
  return err;
}

void SX126x::LoRaConfig(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint16_t preambleLength, uint8_t payloadLen, bool crcOn, bool invertIrq) {
  SetPacketType(SX126X_PACKET_TYPE_LORA);
  SetModulationParams(spreadingFactor, bandwidth, codingRate, 0x00);

  PacketParams[0] = (preambleLength >> 8) & 0xFF;
  PacketParams[1] = preambleLength & 0xFF;
  if (payloadLen) {
    PacketParams[2] = 0x01; // Implicit
    PacketParams[3] = payloadLen;
  } else {
    PacketParams[2] = 0x00; // Explicit
    PacketParams[3] = 0xFF;
  }

  PacketParams[4] = crcOn ? SX126X_LORA_IQ_INVERTED : SX126X_LORA_IQ_STANDARD;
  PacketParams[5] = invertIrq ? 0x01 : 0x00;

  FixInvertedIQ(PacketParams[5]);
  SPIwriteCommand(SX126X_CMD_SET_PACKET_PARAMS, PacketParams, 6);

  SetDioIrqParams(SX126X_IRQ_ALL, SX126X_IRQ_NONE, SX126X_IRQ_NONE, SX126X_IRQ_NONE);

  uint16_t syncWord = SX126X_SYNC_WORD_PRIVATE;
  uint8_t swBuf[2];
  swBuf[0] = (syncWord >> 8) & 0xFF;
  swBuf[1] = syncWord & 0xFF;
  WriteRegister(SX126X_REG_LORA_SYNC_WORD_MSB, swBuf, 2);

  SetRx(0xFFFFFF);
}

int16_t SX126x::SetRx(uint32_t timeout) {
  SetRxEnable();
  uint8_t buf[3];
  buf[0] = (timeout >> 16) & 0xFF;
  buf[1] = (timeout >> 8) & 0xFF;
  buf[2] = timeout & 0xFF;
  SPIwriteCommand(SX126X_CMD_SET_RX, buf, 3);
  return ERR_NONE;
}

int16_t SX126x::SetTx(uint32_t timeoutInMs) {
  SetTxEnable();
  uint32_t tout = (uint32_t)((double)timeoutInMs * 1000.0 / 15.625);
  uint8_t buf[3];
  buf[0] = (tout >> 16) & 0xFF;
  buf[1] = (tout >> 8) & 0xFF;
  buf[2] = tout & 0xFF;
  SPIwriteCommand(SX126X_CMD_SET_TX, buf, 3);
  return ERR_NONE;
}

bool SX126x::Send(uint8_t *pData, uint8_t len, uint8_t mode) {
  uint16_t irqStatus;
  bool rv = false;

  if (txActive == false) {
    txActive = true;
    SetStandby(SX126X_STANDBY_RC);

    PacketParams[2] = 0x00; // Explicit
    PacketParams[3] = len;
    SPIwriteCommand(SX126X_CMD_SET_PACKET_PARAMS, PacketParams, 6);

    ClearIrqStatus(SX126X_IRQ_ALL);
    WriteBuffer(0, pData, len);
    SetTx(0xFFFF);

    if (mode & SX126x_TXMODE_SYNC) {
      irqStatus = GetIrqStatus();
      while ((!(irqStatus & SX126X_IRQ_TX_DONE)) && (!(irqStatus & SX126X_IRQ_TIMEOUT))) {
        delay(1);
        irqStatus = GetIrqStatus();
      }
      txActive = false;
      SetRx(0xFFFFFF);
      if (irqStatus & SX126X_IRQ_TX_DONE) {
        rv = true;
      }
    } else {
      rv = true;
    }
  }
  return rv;
}

uint8_t SX126x::Receive(uint8_t *pData, uint16_t len) {
  uint8_t rxLen = 0;
  uint16_t irqRegs = GetIrqStatus();

  if (irqRegs & SX126X_IRQ_RX_DONE) {
    ClearIrqStatus(SX126X_IRQ_ALL);
    rxLen = ReadBuffer(0, pData, len);
  }

  return rxLen;
}

int16_t SX126x::WriteRegister(uint16_t reg, uint8_t *data, uint8_t numBytes) {
  WaitForIdle(BUSY_WAIT, "start WriteRegister", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.transfer(SX126X_CMD_WRITE_REGISTER);
  SPI.transfer((reg >> 8) & 0xFF);
  SPI.transfer(reg & 0xFF);
  for (uint8_t i = 0; i < numBytes; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end WriteRegister", false);
  return ERR_NONE;
}

int16_t SX126x::ReadRegister(uint16_t reg, uint8_t *data, uint8_t numBytes) {
  WaitForIdle(BUSY_WAIT, "start ReadRegister", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.transfer(SX126X_CMD_READ_REGISTER);
  SPI.transfer((reg >> 8) & 0xFF);
  SPI.transfer(reg & 0xFF);
  SPI.transfer(0x00);
  for (uint8_t i = 0; i < numBytes; i++) {
    data[i] = SPI.transfer(0x00);
  }
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end ReadRegister", false);
  return ERR_NONE;
}

int16_t SX126x::WriteBuffer(uint8_t offset, uint8_t *data, uint8_t numBytes) {
  WaitForIdle(BUSY_WAIT, "start WriteBuffer", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.transfer(SX126X_CMD_WRITE_BUFFER);
  SPI.transfer(offset);
  for (uint8_t i = 0; i < numBytes; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end WriteBuffer", false);
  return ERR_NONE;
}

int16_t SX126x::ReadBuffer(uint8_t offset, uint8_t *data, uint8_t numBytes) {
  WaitForIdle(BUSY_WAIT, "start ReadBuffer", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.transfer(SX126X_CMD_READ_BUFFER);
  SPI.transfer(offset);
  SPI.transfer(0x00);
  for (uint8_t i = 0; i < numBytes; i++) {
    data[i] = SPI.transfer(0x00);
  }
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end ReadBuffer", false);
  return numBytes;
}

void SX126x::SPIwriteCommand(uint8_t cmd, uint8_t *data, uint8_t numBytes) {
  WaitForIdle(BUSY_WAIT, "start WriteCommand", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.transfer(cmd);
  for (uint8_t i = 0; i < numBytes; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end WriteCommand", false);
}

void SX126x::SPIreadCommand(uint8_t cmd, uint8_t *data, uint8_t numBytes) {
  WaitForIdle(BUSY_WAIT, "start ReadCommand", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.transfer(cmd);
  for (uint8_t i = 0; i < numBytes; i++) {
    data[i] = SPI.transfer(0x00);
  }
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end ReadCommand", false);
}

uint8_t SX126x::GetPacketStatus(int8_t *rssiPacket, int8_t *snrPacket) {
  uint8_t buf[4];
  SPIreadCommand(SX126X_CMD_GET_PACKET_STATUS, buf, 4);
  *rssiPacket = (buf[3] >> 1) * -1;
  *snrPacket = buf[2] < 128 ? buf[2] >> 2 : ((buf[2] - 256) >> 2);
  return buf[0];
}

uint16_t SX126x::GetIrqStatus(void) {
  uint8_t buf[2];
  SPIreadCommand(SX126X_CMD_GET_IRQ_STATUS, buf, 2);
  return ((uint16_t)buf[0] << 8) | buf[1];
}

int16_t SX126x::ClearIrqStatus(uint16_t clearIrqParams) {
  uint8_t buf[2];
  buf[0] = (clearIrqParams >> 8) & 0xFF;
  buf[1] = clearIrqParams & 0xFF;
  SPIwriteCommand(SX126X_CMD_CLEAR_IRQ_STATUS, buf, 2);
  return ERR_NONE;
}

bool SX126x::ReceiveMode(void) {
  if (txActive == false) {
    return true;
  }
  uint16_t irq = GetIrqStatus();
  if (irq & (SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT)) {
    SetRx(0xFFFFFF);
    txActive = false;
    return true;
  }
  return false;
}

// ============================================================================
// LoraMsg Implementation
// ============================================================================

LoraMsg::LoraMsg(const uint8_t toAddress[6], const uint8_t fromAddress[6]) {
  currentIndex = 0;
  for (int i = 0; i < 6; i++) message[currentIndex++] = toAddress[i];
  for (int i = 0; i < 6; i++) message[currentIndex++] = fromAddress[i];
  
  PortValue pmi = { { 'M', 'I' }, messageCounter++ };
  addPortValue(pmi);
}

LoraMsg::LoraMsg(const uint8_t* encryptedMsg, int length) {
  currentIndex = (length < MAX_MSG_SIZE) ? length : MAX_MSG_SIZE;
  memcpy(message, encryptedMsg, currentIndex);
}

bool LoraMsg::addPortValue(const PortValue& portValue) {
  if (currentIndex + 4 > MAX_MSG_SIZE - 5) return false;
  message[currentIndex++] = portValue.type[0];
  message[currentIndex++] = portValue.type[1];
  message[currentIndex++] = (portValue.value >> 8) & 0xFF;
  message[currentIndex++] = portValue.value & 0xFF;
  return true;
}

bool LoraMsg::addPortValue(const char type[2], uint16_t value) {
  PortValue pv;
  pv.type[0] = type[0];
  pv.type[1] = type[1];
  pv.value = value;
  return addPortValue(pv);
}

PortValue LoraMsg::getPortValue(int index) const {
  PortValue pv = { { 0, 0 }, 0 };
  int startIndex = 12 + index * 4;
  if (startIndex + 3 < currentIndex) {
    pv.type[0] = message[startIndex];
    pv.type[1] = message[startIndex + 1];
    pv.value = ((uint16_t)message[startIndex + 2] << 8) | message[startIndex + 3];
  }
  return pv;
}

int LoraMsg::numberOfPortValues() const {
  if (currentIndex < 16) return 0;
  return (currentIndex - 16) / 4;
}

void LoraMsg::encryptMessage() {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < currentIndex; i++) {
    crc ^= message[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) crc = (crc >> 1) ^ 0xA001;
      else crc >>= 1;
    }
  }
  PortValue pcs = { { 'C', 'S' }, crc };
  addPortValue(pcs);
}

void LoraMsg::decryptMessage() {
  // Removed blocking delays and active debug prints
}

bool LoraMsg::isForMe(const uint8_t* myAddress) const {
  for (int i = 0; i < 6; i++) {
    if (message[i] != myAddress[i]) return false;
  }
  return true;
}

uint8_t LoraMsg::getFromByte(const uint8_t byteNumber) const {
  if (byteNumber < 6) {
    return message[6 + byteNumber];
  }
  return 0;
}

void LoraMsg::getFromAddress(uint8_t* address) const {
  if (address != nullptr) {
    for (int i = 0; i < 6; i++) {
      address[i] = message[6 + i];
    }
  }
}

bool LoraMsg::setPortValue(const char type[2], uint16_t newValue) {
  int num = numberOfPortValues();
  for (int i = 0; i < num; i++) {
    int startIndex = 12 + i * 4;
    if (message[startIndex] == type[0] && message[startIndex + 1] == type[1]) {
      message[startIndex + 2] = (newValue >> 8) & 0xFF;
      message[startIndex + 3] = newValue & 0xFF;
      return true;
    }
  }
  return false;
}

uint16_t LoraMsg::getMessageID() const {
  PortValue p = getPortValue(0);
  if (p.type[0] == 'M' && p.type[1] == 'I') {
    return p.value;
  }
  return 0;
}

void LoraMsg::printMessage() const {
}
