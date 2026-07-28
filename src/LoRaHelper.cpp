#include "LoRaHelper.h"

// SX126x Member Implementations
SX126x::SX126x(int spiSelect, int reset, int busy, int txen, int rxen) {
  SX126x_SPI_SELECT = spiSelect;
  SX126x_RESET      = reset;
  SX126x_BUSY       = busy;
  SX126x_TXEN       = txen;
  SX126x_RXEN       = rxen;
  
  txActive          = false;
  debugPrint        = false;
  
  pinMode(SX126x_SPI_SELECT, OUTPUT);
  pinMode(SX126x_RESET, OUTPUT);
  pinMode(SX126x_BUSY, INPUT);
  if (SX126x_TXEN != -1) pinMode(SX126x_TXEN, OUTPUT);
  if (SX126x_RXEN != -1) pinMode(SX126x_RXEN, OUTPUT);

  SPI.begin();
}

int16_t SX126x::begin(uint32_t frequencyInHz, int8_t txPowerInDbm, float tcxoVoltage, bool useRegulatorLDO) {
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
  SetRegulatorMode(useRegulatorLDO ? 0x00 : 0x01);
  SetPaConfig(0x04, 0x07, 0x00, 0x01);
  SetDio2AsRfSwitchCtrl(true);

  if (tcxoVoltage > 0.0) {
    uint8_t tcxoParam = 0x00;
    if (abs(tcxoVoltage - 1.6) < 0.1) tcxoParam = 0x00;
    else if (abs(tcxoVoltage - 1.7) < 0.1) tcxoParam = 0x01;
    else if (abs(tcxoVoltage - 1.8) < 0.1) tcxoParam = 0x02;
    else if (abs(tcxoVoltage - 2.2) < 0.1) tcxoParam = 0x03;
    else if (abs(tcxoVoltage - 2.4) < 0.1) tcxoParam = 0x04;
    else if (abs(tcxoVoltage - 2.7) < 0.1) tcxoParam = 0x05;
    else if (abs(tcxoVoltage - 3.0) < 0.1) tcxoParam = 0x06;
    else if (abs(tcxoVoltage - 3.3) < 0.1) tcxoParam = 0x07;

    SetDio3AsTcxoCtrl(tcxoParam, 5000);

    uint16_t calibrateParam = 0x007F;
    SPIwriteCommand(SX126X_CMD_CALIBRATE, (uint8_t*)&calibrateParam, 2);
  }

  CalibrateImage((uint8_t)(frequencyInHz / 10000000), (uint8_t)((frequencyInHz / 10000000) + 2));
  SetRfFrequency(frequencyInHz);
  SetBufferBaseAddress(0x00, 0x00);

  uint8_t syncWordBuffer[2] = { (uint8_t)((SX126X_SYNC_WORD_PUBLIC >> 8) & 0xFF), (uint8_t)(SX126X_SYNC_WORD_PUBLIC & 0xFF) };
  WriteRegister(SX126X_REG_LORA_SYNC_WORD_MSB, syncWordBuffer, 2);

  SetDioIrqParams(SX126X_IRQ_ALL, SX126X_IRQ_ALL, SX126X_IRQ_NONE, SX126X_IRQ_NONE);

  return ERR_NONE;
}

int16_t SX126x::Reset(void) {
  digitalWrite(SX126x_RESET, LOW);
  delay(10);
  digitalWrite(SX126x_RESET, HIGH);
  delay(10);
  WaitingForBusy();
  return ERR_NONE;
}

int16_t SX126x::SetStandby(uint8_t standbyConfig) {
  SPIwriteCommand(SX126X_CMD_SET_STANDBY, &standbyConfig, 1);
  return ERR_NONE;
}

int16_t SX126x::SetSleep(uint8_t sleepConfig) {
  SPIwriteCommand(SX126X_CMD_SET_SLEEP, &sleepConfig, 1);
  return ERR_NONE;
}

int16_t SX126x::SetTx(uint32_t timeout) {
  uint8_t buf[3];
  buf[0] = (uint8_t)((timeout >> 16) & 0xFF);
  buf[1] = (uint8_t)((timeout >> 8) & 0xFF);
  buf[2] = (uint8_t)(timeout & 0xFF);
  SPIwriteCommand(SX126X_CMD_SET_TX, buf, 3);
  return ERR_NONE;
}

int16_t SX126x::SetRx(uint32_t timeout) {
  uint8_t buf[3];
  buf[0] = (uint8_t)((timeout >> 16) & 0xFF);
  buf[1] = (uint8_t)((timeout >> 8) & 0xFF);
  buf[2] = (uint8_t)(timeout & 0xFF);
  SPIwriteCommand(SX126X_CMD_SET_RX, buf, 3);
  return ERR_NONE;
}

int16_t SX126x::SetPacketType(uint8_t packetType) {
  SPIwriteCommand(SX126X_CMD_SET_PACKET_TYPE, &packetType, 1);
  return ERR_NONE;
}

int16_t SX126x::SetRfFrequency(uint32_t frequencyInHz) {
  uint32_t frf = (uint32_t)((double)frequencyInHz / FREQ_STEP);
  uint8_t buf[4];
  buf[0] = (uint8_t)((frf >> 24) & 0xFF);
  buf[1] = (uint8_t)((frf >> 16) & 0xFF);
  buf[2] = (uint8_t)((frf >> 8) & 0xFF);
  buf[3] = (uint8_t)(frf & 0xFF);
  SPIwriteCommand(SX126X_CMD_SET_RF_FREQUENCY, buf, 4);
  return ERR_NONE;
}

int16_t SX126x::SetTxParams(int8_t txPowerInDbm, uint8_t rampTime) {
  uint8_t buf[2];
  buf[0] = txPowerInDbm;
  buf[1] = rampTime;
  SPIwriteCommand(SX126X_CMD_SET_TX_PARAMS, buf, 2);
  return ERR_NONE;
}

int16_t SX126x::SetBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress) {
  uint8_t buf[2];
  buf[0] = txBaseAddress;
  buf[1] = rxBaseAddress;
  SPIwriteCommand(SX126X_CMD_SET_BUFFER_BASE_ADDRESS, buf, 2);
  return ERR_NONE;
}

int16_t SX126x::SetModulationParams(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint8_t lowDataRateOptimize) {
  uint8_t buf[4];
  buf[0] = spreadingFactor;
  buf[1] = bandwidth;
  buf[2] = codingRate;
  buf[3] = lowDataRateOptimize;
  SPIwriteCommand(SX126X_CMD_SET_MODULATION_PARAMS, buf, 4);
  return ERR_NONE;
}

int16_t SX126x::SetPacketParams(uint16_t preambleLength, uint8_t headerType, uint8_t payloadLength, uint8_t crcType, uint8_t invertIQ) {
  uint8_t buf[6];
  buf[0] = (uint8_t)((preambleLength >> 8) & 0xFF);
  buf[1] = (uint8_t)(preambleLength & 0xFF);
  buf[2] = headerType;
  buf[3] = payloadLength;
  buf[4] = crcType;
  buf[5] = invertIQ;
  SPIwriteCommand(SX126X_CMD_SET_PACKET_PARAMS, buf, 6);
  FixInvertedIQ(invertIQ);
  return ERR_NONE;
}

int16_t SX126x::SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
  uint8_t buf[8];
  buf[0] = (uint8_t)((irqMask >> 8) & 0xFF);
  buf[1] = (uint8_t)(irqMask & 0xFF);
  buf[2] = (uint8_t)((dio1Mask >> 8) & 0xFF);
  buf[3] = (uint8_t)(dio1Mask & 0xFF);
  buf[4] = (uint8_t)((dio2Mask >> 8) & 0xFF);
  buf[5] = (uint8_t)(dio2Mask & 0xFF);
  buf[6] = (uint8_t)((dio3Mask >> 8) & 0xFF);
  buf[7] = (uint8_t)(dio3Mask & 0xFF);
  SPIwriteCommand(SX126X_CMD_SET_DIO_IRQ_PARAMS, buf, 8);
  return ERR_NONE;
}

uint16_t SX126x::GetIrqStatus(void) {
  uint8_t buf[2];
  SPIreadCommand(SX126X_CMD_GET_IRQ_STATUS, buf, 2);
  return (buf[0] << 8) | buf[1];
}

int16_t SX126x::ClearIrqStatus(uint16_t clearIrqParams) {
  uint8_t buf[2];
  buf[0] = (uint8_t)((clearIrqParams >> 8) & 0xFF);
  buf[1] = (uint8_t)(clearIrqParams & 0xFF);
  SPIwriteCommand(SX126X_CMD_CLEAR_IRQ_STATUS, buf, 2);
  return ERR_NONE;
}

int16_t SX126x::SetDio2AsRfSwitchCtrl(uint8_t enable) {
  SPIwriteCommand(SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &enable, 1);
  return ERR_NONE;
}

int16_t SX126x::SetDio3AsTcxoCtrl(uint8_t tcxoVoltage, uint32_t timeout) {
  uint8_t buf[4];
  buf[0] = tcxoVoltage;
  buf[1] = (uint8_t)((timeout >> 16) & 0xFF);
  buf[2] = (uint8_t)((timeout >> 8) & 0xFF);
  buf[3] = (uint8_t)(timeout & 0xFF);
  SPIwriteCommand(SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, buf, 4);
  return ERR_NONE;
}

int16_t SX126x::WriteRegister(uint16_t reg, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[3];
  cmd[0] = SX126X_CMD_WRITE_REGISTER;
  cmd[1] = (uint8_t)((reg >> 8) & 0xFF);
  cmd[2] = (uint8_t)(reg & 0xFF);

  WaitingForBusy();
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  for (int i = 0; i < 3; i++) SPI.transfer(cmd[i]);
  for (int i = 0; i < numBytes; i++) SPI.transfer(data[i]);
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  WaitingForBusy();
  return ERR_NONE;
}

int16_t SX126x::ReadRegister(uint16_t reg, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[4];
  cmd[0] = SX126X_CMD_READ_REGISTER;
  cmd[1] = (uint8_t)((reg >> 8) & 0xFF);
  cmd[2] = (uint8_t)(reg & 0xFF);
  cmd[3] = 0x00;

  WaitingForBusy();
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  for (int i = 0; i < 4; i++) SPI.transfer(cmd[i]);
  for (int i = 0; i < numBytes; i++) data[i] = SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  WaitingForBusy();
  return ERR_NONE;
}

int16_t SX126x::WriteBuffer(uint8_t offset, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[2];
  cmd[0] = SX126X_CMD_WRITE_BUFFER;
  cmd[1] = offset;

  WaitingForBusy();
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  SPI.transfer(cmd[0]);
  SPI.transfer(cmd[1]);
  for (int i = 0; i < numBytes; i++) SPI.transfer(data[i]);
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  WaitingForBusy();
  return ERR_NONE;
}

int16_t SX126x::ReadBuffer(uint8_t offset, uint8_t* data, uint8_t numBytes) {
  uint8_t cmd[3];
  cmd[0] = SX126X_CMD_READ_BUFFER;
  cmd[1] = offset;
  cmd[2] = 0x00;

  WaitingForBusy();
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  for (int i = 0; i < 3; i++) SPI.transfer(cmd[i]);
  for (int i = 0; i < numBytes; i++) data[i] = SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  WaitingForBusy();
  return ERR_NONE;
}

int16_t SX126x::GetRxBufferStatus(uint8_t* payloadLength, uint8_t* rxStartBufferPointer) {
  uint8_t buf[2];
  SPIreadCommand(SX126X_CMD_GET_RX_BUFFER_STATUS, buf, 2);
  *payloadLength = buf[0];
  *rxStartBufferPointer = buf[1];
  return ERR_NONE;
}

int16_t SX126x::GetPacketStatus(int8_t* rssiPacket, int8_t* snrPacket) {
  uint8_t buf[3];
  SPIreadCommand(SX126X_CMD_GET_PACKET_STATUS, buf, 3);
  *rssiPacket = -buf[0] / 2;
  *snrPacket = ((int8_t)buf[1]) / 4;
  return ERR_NONE;
}

int16_t SX126x::CalibrateImage(uint8_t freq1, uint8_t freq2) {
  uint8_t buf[2] = { freq1, freq2 };
  SPIwriteCommand(SX126X_CMD_CALIBRATE_IMAGE, buf, 2);
  return ERR_NONE;
}

int16_t SX126x::SetPaConfig(uint8_t paDutyCycle, uint8_t hpMax, uint8_t deviceSelect, uint8_t paLut) {
  uint8_t buf[4] = { paDutyCycle, hpMax, deviceSelect, paLut };
  SPIwriteCommand(SX126X_CMD_SET_PA_CONFIG, buf, 4);
  return ERR_NONE;
}

void SX126x::LoRaConfig(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint16_t preambleLength, uint8_t payloadLen, bool crcOn, bool invertIq) {
  SetPacketType(SX126X_PACKET_TYPE_LORA);
  SetModulationParams(spreadingFactor, bandwidth, codingRate, SX126X_LORA_LOW_DATA_RATE_OPTIMIZE_OFF);
  SetPacketParams(preambleLength, payloadLen == 0 ? SX126X_LORA_HEADER_EXPLICIT : SX126X_LORA_HEADER_IMPLICIT, payloadLen, crcOn ? SX126X_LORA_CRC_ON : SX126X_LORA_CRC_OFF, invertIq ? SX126X_LORA_IQ_INVERTED : SX126X_LORA_IQ_STANDARD);
}

bool SX126x::Send(uint8_t *data, uint8_t len, uint8_t txMode) {
  SetStandby(SX126X_STANDBY_RC);
  SetBufferBaseAddress(0x00, 0x00);
  WriteBuffer(0x00, data, len);
  SetPacketParams(LORA_PREAMBLE_LENGTH, SX126X_LORA_HEADER_EXPLICIT, len, SX126X_LORA_CRC_ON, SX126X_LORA_IQ_STANDARD);
  ClearIrqStatus(SX126X_IRQ_ALL);
  
  if (SX126x_TXEN != -1) digitalWrite(SX126x_TXEN, HIGH);
  if (SX126x_RXEN != -1) digitalWrite(SX126x_RXEN, LOW);

  SetTx(0x000000);

  if (txMode == SX126X_TXMODE_SYNC) {
    uint32_t start = millis();
    while (!(GetIrqStatus() & SX126X_IRQ_TX_DONE)) {
      if (millis() - start > ON_AIR_TIMEOUT) {
        if (SX126x_TXEN != -1) digitalWrite(SX126x_TXEN, LOW);
        SetStandby(SX126X_STANDBY_RC);
        return false;
      }
      delay(1);
    }
    ClearIrqStatus(SX126X_IRQ_TX_DONE);
    if (SX126x_TXEN != -1) digitalWrite(SX126x_TXEN, LOW);
    SetStandby(SX126X_STANDBY_RC);
    return true;
  }
  return true;
}

uint8_t SX126x::Receive(uint8_t *data, uint8_t maxLen) {
  uint16_t irq = GetIrqStatus();
  if (irq & SX126X_IRQ_RX_DONE) {
    if (irq & SX126X_IRQ_CRC_ERR) {
      ClearIrqStatus(SX126X_IRQ_ALL);
      return 0;
    }
    uint8_t payloadLen = 0;
    uint8_t rxPointer = 0;
    GetRxBufferStatus(&payloadLen, &rxPointer);
    if (payloadLen > maxLen) payloadLen = maxLen;
    ReadBuffer(rxPointer, data, payloadLen);
    ClearIrqStatus(SX126X_IRQ_ALL);
    return payloadLen;
  }
  return 0;
}

void SX126x::SPIwriteCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes) {
  WaitingForBusy();
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  SPI.transfer(cmd);
  for (int i = 0; i < numBytes; i++) SPI.transfer(data[i]);
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  WaitingForBusy();
}

void SX126x::SPIreadCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes) {
  WaitingForBusy();
  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  SPI.transfer(cmd);
  SPI.transfer(0x00);
  for (int i = 0; i < numBytes; i++) data[i] = SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);
  WaitingForBusy();
}

void SX126x::SPIwriteCommand(uint8_t cmd) {
  SPIwriteCommand(cmd, NULL, 0);
}

void SX126x::FixInvertedIQ(uint8_t invertIQ) {
  uint8_t regVal = 0;
  ReadRegister(SX126X_REG_IQ_POLARITY_SETUP, &regVal, 1);
  if (invertIQ == SX126X_LORA_IQ_INVERTED) regVal &= 0xFB;
  else regVal |= 0x04;
  WriteRegister(SX126X_REG_IQ_POLARITY_SETUP, &regVal, 1);
}

void SX126x::SetRfTxPower(int8_t txPowerInDbm) {
  SetTxParams(txPowerInDbm, 0x02);
}

int16_t SX126x::SetRegulatorMode(uint8_t mode) {
  SPIwriteCommand(SX126X_CMD_SET_REGULATOR_MODE, &mode, 1);
  return ERR_NONE;
}

void SX126x::DebugPrint(bool enable) {
  debugPrint = enable;
}

uint8_t SX126x::GetStatus(void) {
  uint8_t status = 0;
  SPIreadCommand(SX126X_CMD_GET_STATUS, &status, 1);
  return status;
}

void SX126x::WaitingForBusy(void) {
  uint32_t start = millis();
  while (digitalRead(SX126x_BUSY) == HIGH) {
    if (millis() - start > BUSY_WAIT) break;
    delayMicroseconds(10);
  }
}

// LoraMsg Member Implementations
uint16_t LoraMsg::messageCounter = 0;

LoraMsg::LoraMsg(const uint8_t* toAddress, const uint8_t* fromAddress) {
  memset(message, 0, MAX_MSG_SIZE);
  currentIndex = 0;
  addAddress(toAddress);
  addAddress(fromAddress);
  PortValue messageID = { { 'M', 'I' }, messageCounter++ };
  addPortValue(messageID);
  memcpy(this->toAddress, toAddress, 6);
}

LoraMsg::LoraMsg(const uint8_t* encryptedMessage, byte sizeOfMsg) {
  for (int i = 0; i < sizeOfMsg; i++) {
    message[i] = encryptedMessage[i];
  }
  currentIndex = sizeOfMsg;
  memcpy(this->toAddress, encryptedMessage, 6);
}

uint8_t LoraMsg::getFromByte(const uint8_t byteNumber) {
  if (byteNumber < 6) {
    return message[6 + byteNumber];
  } else {
    return 0;
  }
}

void LoraMsg::getFromAddress(uint8_t* address) {
  for (int i = 0; i < 6; i++) {
    address[i] = message[6 + i];
  }
}

bool LoraMsg::setPortValue(const char* portType, uint16_t value) {
  for (int i = 12; i < currentIndex; i += 4) {
    if (message[i] == portType[0] && message[i + 1] == portType[1]) {
      message[i + 2] = (value >> 8) & 0xFF;
      message[i + 3] = value & 0xFF;
      return true;
    }
  }
  PortValue pv = { { portType[0], portType[1] }, value };
  return addPortValue(pv);
}

bool LoraMsg::addPortValue(const PortValue& portValue) {
  if (currentIndex + 4 > MAX_MSG_SIZE - 5) {
    return false;
  }
  message[currentIndex++] = portValue.type[0];
  message[currentIndex++] = portValue.type[1];
  message[currentIndex++] = (portValue.value >> 8) & 0xFF;
  message[currentIndex++] = portValue.value & 0xFF;
  return true;
}

uint8_t* LoraMsg::getMessage() {
  return message;
}

PortValue LoraMsg::getPortValue(int index) {
  PortValue portValue;
  int startIndex = 12 + index * 4;
  if (startIndex + 3 < currentIndex) {
    portValue.type[0] = message[startIndex];
    portValue.type[1] = message[startIndex + 1];
    portValue.value = (message[startIndex + 2] << 8) | message[startIndex + 3];
  }
  return portValue;
}

uint8_t LoraMsg::numberOfPortValues() {
  int count = 0;
  for (int i = 12; i < currentIndex; i += 4) {
    count++;
  }
  return count;
}

uint16_t LoraMsg::getMessageID() {
  PortValue messageId = getPortValue(0);
  return messageId.value;
}

void LoraMsg::addAddress(const uint8_t* address) {
  for (int i = 0; i < 6; i++) {
    message[currentIndex++] = address[i];
  }
}

void LoraMsg::encryptMessage() {
  uint16_t messageID = getMessageID();
  for (int i = 6; i < 12; i++) {
    message[i] = message[i] + toAddress[(i - 6) % 6] + (messageID & 0xFF);
  }
  for (int i = 16; i < currentIndex; i++) {
    message[i] = message[i] + toAddress[(i - 6) % 6] + (messageID & 0xFF);
  }
}

void LoraMsg::decryptMessage() {
  uint16_t messageID = getMessageID();
  for (int i = 6; i < 12; i++) {
    message[i] = message[i] - toAddress[(i - 6) % 6] - (messageID & 0xFF);
  }
  for (int i = 16; i < currentIndex; i++) {
    message[i] = message[i] - toAddress[(i - 6) % 6] - (messageID & 0xFF);
  }
}

bool LoraMsg::isForMe(const uint8_t* address) {
  for (int i = 0; i < 6; i++) {
    if (message[i] != address[i]) return false;
  }
  return true;
}

uint8_t LoraMsg::getMessageLength() {
  return currentIndex;
}

void LoraMsg::printMessage() {
  Serial.print("To: ");
  for (int i = 0; i < 6; i++) {
    if (message[i] < 0x10) Serial.print("0");
    Serial.print(message[i], HEX);
  }
  Serial.print(" ");

  Serial.print("From: ");
  for (int i = 6; i < 12; i++) {
    if (message[i] < 0x10) Serial.print("0");
    Serial.print(message[i], HEX);
  }
  Serial.print(" ");

  for (int i = 0; i < numberOfPortValues(); i++) {
    PortValue portValue = getPortValue(i);
    Serial.print(portValue.type[0]);
    Serial.print(portValue.type[1]);
    Serial.print(": ");
    Serial.print(portValue.value);
    Serial.print(" ");
  }
}
