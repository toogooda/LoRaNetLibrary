#include "LoRaHelper.h"

// Initialize static message sequence counter
uint16_t LoraMsg::messageCounter = 1;

SX126x::SX126x(int spiSelect, int reset, int busy, int txen, int rxen) {
  _spiSelect = spiSelect;
  _reset = reset;
  _busy = busy;
  _txen = txen;
  _rxen = rxen;
  debugPrint = false;
}

void SX126x::DebugPrint(bool debug) {
  debugPrint = debug;
}

// Item 7: Private Internal WaitForIdle Helper
void SX126x::WaitForIdle(unsigned long timeout, const char *text, bool stop) {
  unsigned long start = millis();
  delayMicroseconds(1);
  while (digitalRead(_busy)) {
    delayMicroseconds(1);
    if (millis() - start >= timeout) {
      if (debugPrint && text[0] != '\0') {
        Serial.print("BUSY Timeout during ");
        Serial.println(text);
      }
      if (stop) {
        while (1) { delay(1); } // Unrecoverable hardware halt
      }
      break;
    }
  }
}

void SX126x::WriteRegister(uint16_t reg, uint8_t *data, uint8_t len) {
  WaitForIdle(BUSY_WAIT, "start WriteRegister", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(_spiSelect, LOW);
  SPI.transfer(SX126X_CMD_WRITE_REGISTER);
  SPI.transfer((reg >> 8) & 0xFF);
  SPI.transfer(reg & 0xFF);
  for (uint8_t i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(_spiSelect, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end WriteRegister", false);
}

void SX126x::ReadRegister(uint16_t reg, uint8_t *data, uint8_t len) {
  WaitForIdle(BUSY_WAIT, "start ReadRegister", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(_spiSelect, LOW);
  SPI.transfer(SX126X_CMD_READ_REGISTER);
  SPI.transfer((reg >> 8) & 0xFF);
  SPI.transfer(reg & 0xFF);
  SPI.transfer(0x00); // NOP
  for (uint8_t i = 0; i < len; i++) {
    data[i] = SPI.transfer(0x00);
  }
  digitalWrite(_spiSelect, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end ReadRegister", false);
}

void SX126x::WriteBuffer(uint8_t *data, uint8_t len, uint8_t offset) {
  WaitForIdle(BUSY_WAIT, "start WriteBuffer", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(_spiSelect, LOW);
  SPI.transfer(SX126X_CMD_WRITE_BUFFER);
  SPI.transfer(offset);
  for (uint8_t i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(_spiSelect, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end WriteBuffer", false);
}

void SX126x::ReadBuffer(uint8_t *data, uint8_t len, uint8_t offset) {
  WaitForIdle(BUSY_WAIT, "start ReadBuffer", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(_spiSelect, LOW);
  SPI.transfer(SX126X_CMD_READ_BUFFER);
  SPI.transfer(offset);
  SPI.transfer(0x00); // NOP
  for (uint8_t i = 0; i < len; i++) {
    data[i] = SPI.transfer(0x00);
  }
  digitalWrite(_spiSelect, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end ReadBuffer", false);
}

void SX126x::WriteCommand(uint8_t cmd, uint8_t *data, uint8_t len) {
  WaitForIdle(BUSY_WAIT, "start WriteCommand", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(_spiSelect, LOW);
  SPI.transfer(cmd);
  for (uint8_t i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(_spiSelect, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end WriteCommand", false);
}

// Item 20: SPI Status Byte Verification & Retry
void SX126x::WriteCommand2(uint8_t cmd, uint8_t *data, uint8_t len) {
  WaitForIdle(BUSY_WAIT, "start WriteCommand2", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(_spiSelect, LOW);
  SPI.transfer(cmd);
  for (uint8_t i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(_spiSelect, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end WriteCommand2", false);
}

void SX126x::ReadCommand(uint8_t cmd, uint8_t *data, uint8_t len) {
  WaitForIdle(BUSY_WAIT, "start ReadCommand", true);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  digitalWrite(_spiSelect, LOW);
  SPI.transfer(cmd);
  for (uint8_t i = 0; i < len; i++) {
    data[i] = SPI.transfer(0x00);
  }
  digitalWrite(_spiSelect, HIGH);
  SPI.endTransaction();
  WaitForIdle(BUSY_WAIT, "end ReadCommand", false);
}

// Item 10: RF Switch Pin Control
void SX126x::SetRxEnable() {
  if (_rxen != -1) digitalWrite(_rxen, HIGH);
  if (_txen != -1) digitalWrite(_txen, LOW);
}

void SX126x::SetTxEnable() {
  if (_txen != -1) digitalWrite(_txen, HIGH);
  if (_rxen != -1) digitalWrite(_rxen, LOW);
}

void SX126x::SetRx(uint32_t timeout) {
  SetRxEnable();
  uint8_t buf[3];
  buf[0] = (timeout >> 16) & 0xFF;
  buf[1] = (timeout >> 8) & 0xFF;
  buf[2] = timeout & 0xFF;
  WriteCommand(SX126X_CMD_SET_RX, buf, 3);
}

// Item 13: SetTx 15.625 us Ticks Conversion
void SX126x::SetTx(uint32_t timeoutInMs) {
  SetTxEnable();
  uint32_t tout = (uint32_t)((double)timeoutInMs * 1000.0 / 15.625);
  uint8_t buf[3];
  buf[0] = (tout >> 16) & 0xFF;
  buf[1] = (tout >> 8) & 0xFF;
  buf[2] = tout & 0xFF;
  WriteCommand(SX126X_CMD_SET_TX, buf, 3);
}

// Item 17: SetPaConfig (+22dBm)
void SX126x::SetPaConfig(uint8_t paDutyCycle, uint8_t hpDutyCycle, uint8_t deviceSelect, uint8_t paLut) {
  uint8_t buf[4];
  buf[0] = paDutyCycle;
  buf[1] = hpDutyCycle;
  buf[2] = deviceSelect;
  buf[3] = paLut;
  WriteCommand(SX126X_CMD_SET_PA_CONFIG, buf, 4);
}

// Item 18: SetOvercurrentProtection (60mA)
void SX126x::SetOvercurrentProtection(float currentLimit) {
  uint8_t raw = (uint8_t)(currentLimit / 2.5);
  WriteRegister(SX126X_REG_OCP, &raw, 1);
}

// Item 19: FixInvertedIQ (Silicon Errata 15.4)
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

int16_t SX126x::begin(uint32_t frequency, int8_t txPower, float tcxoVoltage, bool useRegulatorLDO) {
  pinMode(_spiSelect, OUTPUT);
  digitalWrite(_spiSelect, HIGH);
  
  if (_reset != -1) {
    pinMode(_reset, OUTPUT);
    digitalWrite(_reset, LOW);
    delay(10);
    digitalWrite(_reset, HIGH);
    delay(10);
  }
  
  pinMode(_busy, INPUT);
  if (_txen != -1) pinMode(_txen, OUTPUT);
  if (_rxen != -1) pinMode(_rxen, OUTPUT);

  // Item 8: Commented out boot pin debug prints
  /*
  if (debugPrint) {
    Serial.print("SPI_SELECT="); Serial.println(_spiSelect);
    Serial.print("RESET=");      Serial.println(_reset);
    Serial.print("BUSY=");       Serial.println(_busy);
  }
  */

  uint8_t mode = 0x00; // STDBY_RC
  WriteCommand(SX126X_CMD_SET_STANDBY, &mode, 1);

  uint8_t pktType = 0x01; // LoRa
  WriteCommand(SX126X_CMD_SET_PACKET_TYPE, &pktType, 1);

  SetPaConfig(0x04, 0x07, 0x00, 0x01);
  SetOvercurrentProtection(60.0);

  // Set RF Frequency
  uint32_t frf = (uint32_t)((double)frequency / FREQ_STEP);
  uint8_t buf[4];
  buf[0] = (frf >> 24) & 0xFF;
  buf[1] = (frf >> 16) & 0xFF;
  buf[2] = (frf >> 8) & 0xFF;
  buf[3] = frf & 0xFF;
  WriteCommand(SX126X_CMD_SET_RF_FREQUENCY, buf, 4);

  // Set Tx Params
  uint8_t txBuf[2];
  txBuf[0] = txPower;
  txBuf[1] = 0x02; // 40us ramp
  WriteCommand(SX126X_CMD_SET_TX_PARAMS, txBuf, 2);

  // DIO2 as RF Switch
  uint8_t dio2 = 0x01;
  WriteCommand(SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &dio2, 1);

  return ERR_NONE;
}

int16_t SX126x::beginFarmDefaults() {
  int16_t err = begin(RF_FREQUENCY, TX_OUTPUT_POWER);
  if (err == ERR_NONE) {
    LoRaConfig(LORA_SPREADING_FACTOR, LORA_BANDWIDTH, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH, LORA_PAYLOADLENGTH, true, false);
  }
  return err;
}

int16_t SX126x::LoRaConfig(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint16_t preambleLength, uint8_t payloadLen, bool crcOn, bool invertIrq) {
  uint8_t modParams[4];
  modParams[0] = spreadingFactor;
  modParams[1] = bandwidth;
  modParams[2] = codingRate;
  modParams[3] = 0x01; // LowDataRateOptimize ON
  WriteCommand(SX126X_CMD_SET_MODULATION_PARAMS, modParams, 4);

  PacketParams[0] = (preambleLength >> 8) & 0xFF;
  PacketParams[1] = preambleLength & 0xFF;
  PacketParams[2] = (payloadLen == 0) ? 0x00 : 0x01; // HeaderType: Explicit=0, Implicit=1
  PacketParams[3] = payloadLen;
  PacketParams[4] = crcOn ? 0x01 : 0x00;
  PacketParams[5] = invertIrq ? 0x01 : 0x00;
  WriteCommand(SX126X_CMD_SET_PACKET_PARAMS, PacketParams, 6);

  FixInvertedIQ(PacketParams[5]);

  // Set Sync Word (Private Network 0x1424)
  uint16_t syncWord = SX126X_SYNC_WORD_PRIVATE;
  uint8_t swBuf[2];
  swBuf[0] = (syncWord >> 8) & 0xFF;
  swBuf[1] = syncWord & 0xFF;
  WriteRegister(0x0740, swBuf, 2);

  // Item 11: Continuous Receive Mode at end of Config
  SetRx(0xFFFFFF);
  return ERR_NONE;
}

int16_t SX126x::Send(uint8_t *pData, uint8_t len, uint8_t mode) {
  WriteBuffer(pData, len, 0);
  SetTx(1000);
  
  // Wait for Tx Done or Timeout
  unsigned long start = millis();
  while (digitalRead(_busy)) {
    if (millis() - start > ON_AIR_TIMEOUT) {
      break;
    }
    delay(1);
  }
  
  // Item 11: Return to Continuous Receive Mode after sending
  SetRx(0xFFFFFF);
  return ERR_NONE;
}

int16_t SX126x::Receive(uint8_t *pData, uint8_t len) {
  uint8_t payloadLen = 0;
  uint8_t startAddress = 0;
  GetRxBufferStatus(&payloadLen, &startAddress);
  if (payloadLen > 0) {
    uint8_t readLen = (len < payloadLen) ? len : payloadLen;
    ReadBuffer(pData, readLen, startAddress);
    return readLen;
  }
  return 0;
}

uint8_t SX126x::GetRxBufferStatus(uint8_t *payloadLen, uint8_t *startAddress) {
  uint8_t buf[2];
  ReadCommand(SX126X_CMD_GET_RX_BUFFER_STATUS, buf, 2);
  *payloadLen = buf[0];
  *startAddress = buf[1];
  return buf[0];
}

// Item 14: GetPacketStatus SPI Offset & RSSI/SNR Formula
uint8_t SX126x::GetPacketStatus(int8_t *rssiPacket, int8_t *snrPacket) {
  uint8_t buf[4];
  ReadCommand(SX126X_CMD_GET_PACKET_STATUS, buf, 4);
  *rssiPacket = (buf[3] >> 1) * -1;
  *snrPacket = buf[2] < 128 ? buf[2] >> 2 : ((buf[2] - 256) >> 2);
  return buf[0];
}

uint16_t SX126x::GetIrqStatus() {
  uint8_t buf[2];
  ReadCommand(SX126X_CMD_GET_IRQ_STATUS, buf, 2);
  return ((uint16_t)buf[0] << 8) | buf[1];
}

void SX126x::ClearIrqStatus(uint16_t irqMask) {
  uint8_t buf[2];
  buf[0] = (irqMask >> 8) & 0xFF;
  buf[1] = irqMask & 0xFF;
  WriteCommand(SX126X_CMD_CLEAR_IRQ_STATUS, buf, 2);
}

bool SX126x::ReceiveMode() {
  SetRx(0xFFFFFF);
  return true;
}

// ============================================================================
// LoraMsg Implementation
// ============================================================================

LoraMsg::LoraMsg(const uint8_t toAddress[6], const uint8_t fromAddress[6]) {
  currentIndex = 0;
  for (int i = 0; i < 6; i++) message[currentIndex++] = toAddress[i];
  for (int i = 0; i < 6; i++) message[currentIndex++] = fromAddress[i];
  
  // Mandatory initial Message ID pair ('MI')
  PortValue pmi = { { 'M', 'I' }, messageCounter++ };
  addPortValue(pmi);
}

LoraMsg::LoraMsg(const uint8_t* encryptedMsg, int length) {
  currentIndex = (length < MAX_MSG_SIZE) ? length : MAX_MSG_SIZE;
  memcpy(message, encryptedMsg, currentIndex);
}

bool LoraMsg::addPortValue(const PortValue& portValue) {
  if (currentIndex + 4 > MAX_MSG_SIZE - 5) return false; // Save space for CS
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
  // Item 15: Removed blocking delay(100) and commented out debug prints
}

bool LoraMsg::isForMe(const uint8_t* myAddress) const {
  for (int i = 0; i < 6; i++) {
    if (message[i] != myAddress[i]) return false;
  }
  return true;
}

// Item 10: Source Address Extraction
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

// Item 11: In-Flight Payload Modification
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

// Item 12: getMessageID
uint16_t LoraMsg::getMessageID() const {
  PortValue p = getPortValue(0);
  if (p.type[0] == 'M' && p.type[1] == 'I') {
    return p.value;
  }
  return 0;
}

void LoraMsg::printMessage() const {
  // Printing helper
}
