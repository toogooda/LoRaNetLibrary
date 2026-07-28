#include "LoRaHelper.h"

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
  SetPaConfig(0x04, 0x07, 0x00, 0x01); // PA Optimal Settings +22 dBm
  SetOvercurrentProtection(60.0);
  SetPowerConfig(txPowerInDbm, SX126X_PA_RAMP_200U);
  SetRfFrequency(frequencyInHz);
  return ERR_NONE;
}

void SX126x::FixInvertedIQ(uint8_t iqConfig) {
  uint8_t iqConfigCurrent = 0;
  ReadRegister(SX126X_REG_IQ_POLARITY_SETUP, &iqConfigCurrent, 1);
  if (iqConfig == SX126X_LORA_IQ_STANDARD) {
    iqConfigCurrent &= 0xFB;
  } else {
    iqConfigCurrent |= 0x04;
  }
  WriteRegister(SX126X_REG_IQ_POLARITY_SETUP, &iqConfigCurrent, 1);
}

void SX126x::LoRaConfig(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint16_t preambleLength, uint8_t payloadLen, bool crcOn, bool invertIrq) {
  SetStopRxTimerOnPreambleDetect(false);
  SetLoRaSymbNumTimeout(0);
  SetPacketType(SX126X_PACKET_TYPE_LORA);
  uint8_t ldro = 0;
  SetModulationParams(spreadingFactor, bandwidth, codingRate, ldro);

  PacketParams[0] = (preambleLength >> 8) & 0xFF;
  PacketParams[1] = preambleLength;
  if (payloadLen) {
    PacketParams[2] = 0x01; // Fixed length packet
    PacketParams[3] = payloadLen;
  } else {
    PacketParams[2] = 0x00; // Variable length packet
    PacketParams[3] = 0xFF;
  }

  if (crcOn) {
    PacketParams[4] = SX126X_LORA_IQ_INVERTED;
  } else {
    PacketParams[4] = SX126X_LORA_IQ_STANDARD;
  }

  if (invertIrq) {
    PacketParams[5] = 0x01;
  } else {
    PacketParams[5] = 0x00;
  }

  FixInvertedIQ(PacketParams[5]);
  WriteCommand(SX126X_CMD_SET_PACKET_PARAMS, PacketParams, 6);

  SetDioIrqParams(SX126X_IRQ_ALL, SX126X_IRQ_NONE, SX126X_IRQ_NONE, SX126X_IRQ_NONE);

  SetRx(0xFFFFFF); // Continuous receive mode
}

void SX126x::DebugPrint(bool enable) {
  debugPrint = enable;
}

uint8_t SX126x::Receive(uint8_t *pData, uint16_t len) {
  uint8_t rxLen = 0;
  uint16_t irqRegs = GetIrqStatus();
  if (irqRegs & SX126X_IRQ_RX_DONE) {
    ClearIrqStatus(SX126X_IRQ_ALL);
    rxLen = ReadBuffer(pData, len);
  }
  return rxLen;
}

bool SX126x::Send(uint8_t *pData, uint8_t len, uint8_t mode) {
  uint16_t irqStatus;
  bool rv = false;

  if (txActive == false) {
    txActive = true;
    SetStandby(SX126X_STANDBY_RC);
    PacketParams[2] = 0x00;
    PacketParams[3] = len;
    WriteCommand(SX126X_CMD_SET_PACKET_PARAMS, PacketParams, 6);

    ClearIrqStatus(SX126X_IRQ_ALL);
    WriteBuffer(pData, len);
    SetTx(0xFFFF); // ~1 second timeout

    if (mode & SX126x_TXMODE_SYNC) {
      irqStatus = GetIrqStatus();
      while ((!(irqStatus & SX126X_IRQ_TX_DONE)) && (!(irqStatus & SX126X_IRQ_TIMEOUT))) {
        delay(1);
        irqStatus = GetIrqStatus();
      }

      txActive = false;
      SetRx(0xFFFFFF); // Put back into receive mode

      if (irqStatus & SX126X_IRQ_TX_DONE) {
        rv = true;
      }
    } else {
      rv = true;
    }
  }
  return rv;
}

bool SX126x::ReceiveMode(void) {
  uint16_t irq;
  bool rv = false;

  if (txActive == false) {
    rv = true;
  } else {
    irq = GetIrqStatus();
    if (irq & (SX126X_IRQ_TX_DONE | SX126X_IRQ_TIMEOUT)) {
      SetRx(0xFFFFFF);
      txActive = false;
      rv = true;
    }
  }
  return rv;
}

void SX126x::GetPacketStatus(int8_t *rssiPacket, int8_t *snrPacket) {
  uint8_t buf[4];
  ReadCommand(SX126X_CMD_GET_PACKET_STATUS, buf, 4);
  *rssiPacket = (buf[3] >> 1) * -1;
  (buf[2] < 128) ? (*snrPacket = buf[2] >> 2) : (*snrPacket = ((buf[2] - 256) >> 2));
}

void SX126x::SetTxPower(int8_t txPowerInDbm) {
  SetPowerConfig(txPowerInDbm, SX126X_PA_RAMP_200U);
}

int16_t SX126x::Reset(void) {
  delay(10);
  digitalWrite(SX126x_RESET, LOW);
  delay(20);
  digitalWrite(SX126x_RESET, HIGH);
  delay(10);
  WaitForIdle();
  return ERR_NONE;
}

void SX126x::Wakeup(void) {
  GetStatus();
}

void SX126x::SetSleep(uint8_t mode) {
  uint8_t data = mode;
  WriteCommand(SX126X_CMD_SET_SLEEP, &data, 1);
}

void SX126x::SetStandby(uint8_t mode) {
  uint8_t data = mode;
  WriteCommand(SX126X_CMD_SET_STANDBY, &data, 1);
}

uint8_t SX126x::GetStatus(void) {
  uint8_t rv;
  ReadCommand(SX126X_CMD_GET_STATUS, &rv, 1);
  return rv;
}

uint32_t SX126x::GetRandomNumber(void) {
  uint8_t random[4];
  ReadRegister(SX126X_REG_RANDOM_NUMBER_0, random, 4);
  return *((uint32_t *)random);
}

void SX126x::SetDio3AsTcxoCtrl(float voltage, uint32_t delayVal) {
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

  uint32_t delayValue = (float)delayVal / 15.625;
  buf[1] = (uint8_t)((delayValue >> 16) & 0xFF);
  buf[2] = (uint8_t)((delayValue >> 8) & 0xFF);
  buf[3] = (uint8_t)(delayValue & 0xFF);
  WriteCommand(SX126X_CMD_SET_DIO3_AS_TCXO_CTRL, buf, 4);
}

void SX126x::Calibrate(uint8_t calibParam) {
  uint8_t data = calibParam;
  WriteCommand(SX126X_CMD_CALIBRATE, &data, 1);
}

void SX126x::SetDio2AsRfSwitchCtrl(uint8_t enable) {
  uint8_t data = enable;
  WriteCommand(SX126X_CMD_SET_DIO2_AS_RF_SWITCH_CTRL, &data, 1);
}

void SX126x::SetRfFrequency(uint32_t frequency) {
  uint32_t frf = (uint32_t)((double)frequency / FREQ_STEP);
  uint8_t buf[4];
  buf[0] = (uint8_t)((frf >> 24) & 0xFF);
  buf[1] = (uint8_t)((frf >> 16) & 0xFF);
  buf[2] = (uint8_t)((frf >> 8) & 0xFF);
  buf[3] = (uint8_t)(frf & 0xFF);
  WriteCommand(SX126X_CMD_SET_RF_FREQUENCY, buf, 4);
}

void SX126x::SetPowerConfig(int8_t power, uint8_t rampTime) {
  uint8_t buf[2];
  buf[0] = power;
  buf[1] = rampTime;
  WriteCommand(SX126X_CMD_SET_TX_PARAMS, buf, 2);
}

void SX126x::SetOvercurrentProtection(float currentLimit) {
  uint8_t buf[1];
  buf[0] = (uint8_t)(currentLimit / 2.5);
  WriteRegister(SX126X_REG_RX_GAIN, buf, 1);
}

void SX126x::SetPaConfig(uint8_t paDutyCycle, uint8_t hpMax, uint8_t deviceSel, uint8_t paLut) {
  uint8_t buf[4] = { paDutyCycle, hpMax, deviceSel, paLut };
  WriteCommand(SX126X_CMD_SET_PA_CONFIG, buf, 4);
}

void SX126x::SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
  uint8_t buf[8];
  buf[0] = (uint8_t)((irqMask >> 8) & 0xFF);
  buf[1] = (uint8_t)(irqMask & 0xFF);
  buf[2] = (uint8_t)((dio1Mask >> 8) & 0xFF);
  buf[3] = (uint8_t)(dio1Mask & 0xFF);
  buf[4] = (uint8_t)((dio2Mask >> 8) & 0xFF);
  buf[5] = (uint8_t)(dio2Mask & 0xFF);
  buf[6] = (uint8_t)((dio3Mask >> 8) & 0xFF);
  buf[7] = (uint8_t)(dio3Mask & 0xFF);
  WriteCommand(SX126X_CMD_SET_DIO_IRQ_PARAMS, buf, 8);
}

void SX126x::SetStopRxTimerOnPreambleDetect(bool enable) {
  uint8_t data = enable ? 0x01 : 0x00;
  WriteCommand(SX126X_CMD_STOP_TIMER_ON_PREAMBLE, &data, 1);
}

void SX126x::SetLoRaSymbNumTimeout(uint8_t SymbNum) {
  uint8_t data = SymbNum;
  WriteCommand(SX126X_CMD_SET_LORA_SYMB_NUM_TIMEOUT, &data, 1);
}

void SX126x::SetPacketType(uint8_t packetType) {
  WriteCommand(SX126X_CMD_SET_PACKET_TYPE, &packetType, 1);
}

void SX126x::SetModulationParams(uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate, uint8_t lowDataRateOptimize) {
  uint8_t buf[4];
  buf[0] = spreadingFactor;
  buf[1] = bandwidth;
  buf[2] = codingRate;
  buf[3] = lowDataRateOptimize;
  WriteCommand(SX126X_CMD_SET_MODULATION_PARAMS, buf, 4);
}

void SX126x::SetRegulatorMode(uint8_t mode) {
  uint8_t data = mode;
  WriteCommand(SX126X_CMD_SET_REGULATOR_MODE, &data, 1);
}

void SX126x::SetBufferBaseAddress(uint8_t txBaseAddress, uint8_t rxBaseAddress) {
  uint8_t buf[2];
  buf[0] = txBaseAddress;
  buf[1] = rxBaseAddress;
  WriteCommand(SX126X_CMD_SET_BUFFER_BASE_ADDRESS, buf, 2);
}

uint16_t SX126x::GetIrqStatus(void) {
  uint8_t buf[2];
  ReadCommand(SX126X_CMD_GET_IRQ_STATUS, buf, 2);
  return (buf[0] << 8) | buf[1];
}

void SX126x::ClearIrqStatus(uint16_t irq) {
  uint8_t buf[2];
  buf[0] = (uint8_t)((irq >> 8) & 0xFF);
  buf[1] = (uint8_t)(irq & 0xFF);
  WriteCommand(SX126X_CMD_CLEAR_IRQ_STATUS, buf, 2);
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

void SX126x::SetRx(uint32_t timeout) {
  SetStandby(SX126X_STANDBY_RC);
  SetRxEnable();
  uint8_t buf[3];
  buf[0] = (uint8_t)((timeout >> 16) & 0xFF);
  buf[1] = (uint8_t)((timeout >> 8) & 0xFF);
  buf[2] = (uint8_t)(timeout & 0xFF);
  WriteCommand(SX126X_CMD_SET_RX, buf, 3);

  for (int retry = 0; retry < 10; retry++) {
    if ((GetStatus() & 0x70) == 0x50) break;
    delay(1);
  }
}

void SX126x::SetTx(uint32_t timeoutInMs) {
  SetStandby(SX126X_STANDBY_RC);
  SetTxEnable();
  uint8_t buf[3];
  uint32_t tout = timeoutInMs;
  if (timeoutInMs != 0) {
    uint32_t timeoutInUs = timeoutInMs * 1000;
    tout = (uint32_t)(timeoutInUs / 15.625);
  }
  buf[0] = (uint8_t)((tout >> 16) & 0xFF);
  buf[1] = (uint8_t)((tout >> 8) & 0xFF);
  buf[2] = (uint8_t)(tout & 0xFF);
  WriteCommand(SX126X_CMD_SET_TX, buf, 3);

  for (int retry = 0; retry < 10; retry++) {
    if ((GetStatus() & 0x70) == 0x60) break;
    delay(1);
  }
}

uint8_t SX126x::GetRssiInst() {
  uint8_t buf[2];
  ReadCommand(SX126X_CMD_GET_RSSI_INST, buf, 2);
  return buf[1];
}

void SX126x::GetRxBufferStatus(uint8_t *payloadLength, uint8_t *rxStartBufferPointer) {
  uint8_t buf[3];
  ReadCommand(SX126X_CMD_GET_RX_BUFFER_STATUS, buf, 3);
  *payloadLength = buf[1];
  *rxStartBufferPointer = buf[2];
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
      return;
    }
  }
}

uint8_t SX126x::ReadBuffer(uint8_t *rxData, uint8_t maxLen) {
  uint8_t offset = 0;
  uint8_t payloadLength = 0;
  GetRxBufferStatus(&payloadLength, &offset);
  if (payloadLength > maxLen) {
    return 0;
  }

  WaitForIdle(BUSY_WAIT, "start ReadBuffer", false);

  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  SPI.transfer(SX126X_CMD_READ_BUFFER);
  SPI.transfer(offset);
  SPI.transfer(SX126X_CMD_NOP);
  for (uint16_t i = 0; i < payloadLength; i++) {
    rxData[i] = SPI.transfer(SX126X_CMD_NOP);
  }
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);

  WaitForIdle(BUSY_WAIT, "end ReadBuffer", false);
  return payloadLength;
}

void SX126x::WriteBuffer(uint8_t *txData, uint8_t txDataLen) {
  WaitForIdle(BUSY_WAIT, "start WriteBuffer", true);

  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  SPI.transfer(SX126X_CMD_WRITE_BUFFER);
  SPI.transfer(0);
  for (uint16_t i = 0; i < txDataLen; i++) {
    SPI.transfer(txData[i]);
  }
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);

  WaitForIdle(BUSY_WAIT, "end WriteBuffer", false);
}

void SX126x::WriteRegister(uint16_t reg, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  WaitForIdle(BUSY_WAIT, "start WriteRegister", true);

  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));

  SPI.transfer(SX126X_CMD_WRITE_REGISTER);
  SPI.transfer((reg & 0xFF00) >> 8);
  SPI.transfer(reg & 0xFF);
  for (uint8_t n = 0; n < numBytes; n++) {
    SPI.transfer(data[n]);
  }
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);

  if (waitForBusy) {
    WaitForIdle(BUSY_WAIT, "end WriteRegister", false);
  }
}

void SX126x::ReadRegister(uint16_t reg, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  WaitForIdle(BUSY_WAIT, "start ReadRegister", true);

  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));

  SPI.transfer(SX126X_CMD_READ_REGISTER);
  SPI.transfer((reg & 0xFF00) >> 8);
  SPI.transfer(reg & 0xFF);
  SPI.transfer(SX126X_CMD_NOP);

  for (uint8_t n = 0; n < numBytes; n++) {
    data[n] = SPI.transfer(SX126X_CMD_NOP);
  }
  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);

  if (waitForBusy) {
    WaitForIdle(BUSY_WAIT, "end ReadRegister", false);
  }
}

void SX126x::WriteCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  uint8_t status;
  for (int retry = 1; retry < 10; retry++) {
    status = WriteCommand2(cmd, data, numBytes, waitForBusy);
    if (status == 0) break;
  }
}

uint8_t SX126x::WriteCommand2(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  WaitForIdle(BUSY_WAIT, "start WriteCommand2", true);

  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));

  SPI.transfer(cmd);
  uint8_t status = 0;

  for (uint8_t n = 0; n < numBytes; n++) {
    uint8_t in = SPI.transfer(data[n]);
    if (((in & 0b00001110) == SX126X_STATUS_CMD_TIMEOUT) ||
        ((in & 0b00001110) == SX126X_STATUS_CMD_INVALID) ||
        ((in & 0b00001110) == SX126X_STATUS_CMD_FAILED)) {
      status = in & 0b00001110;
      break;
    } else if (in == 0x00 || in == 0xFF) {
      status = SX126X_STATUS_SPI_FAILED;
      break;
    }
  }

  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);

  if (waitForBusy) {
    WaitForIdle(BUSY_WAIT, "end WriteCommand2", false);
  }
  return status;
}

void SX126x::ReadCommand(uint8_t cmd, uint8_t* data, uint8_t numBytes, bool waitForBusy) {
  WaitForIdle(BUSY_WAIT, "start ReadCommand", true);

  digitalWrite(SX126x_SPI_SELECT, LOW);
  SPI.beginTransaction(SPISettings(SPI_Speed, MSBFIRST, SPI_MODE0));
  SPI.transfer(cmd);

  for (uint8_t n = 0; n < numBytes; n++) {
    data[n] = SPI.transfer(SX126X_CMD_NOP);
  }

  SPI.endTransaction();
  digitalWrite(SX126x_SPI_SELECT, HIGH);

  if (waitForBusy) {
    WaitForIdle(BUSY_WAIT, "end ReadCommand", false);
  }
}

// LoraMsg Member Implementations
uint16_t LoraMsg::messageCounter = 0;

LoraMsg::LoraMsg(const uint8_t* toAddress, const uint8_t* fromAddress) {
  this->currentIndex = 0;

  message[currentIndex++] = toAddress[0];
  message[currentIndex++] = toAddress[1];
  message[currentIndex++] = toAddress[2];
  message[currentIndex++] = toAddress[3];
  message[currentIndex++] = toAddress[4];
  message[currentIndex++] = toAddress[5];

  message[currentIndex++] = fromAddress[0];
  message[currentIndex++] = fromAddress[1];
  message[currentIndex++] = fromAddress[2];
  message[currentIndex++] = fromAddress[3];
  message[currentIndex++] = fromAddress[4];
  message[currentIndex++] = fromAddress[5];

  message[currentIndex++] = 'M';
  message[currentIndex++] = 'I';
  message[currentIndex++] = (messageCounter >> 8) & 0xFF;
  message[currentIndex++] = messageCounter & 0xFF;

  messageCounter++;
  memcpy(this->toAddress, toAddress, 6);
}

LoraMsg::LoraMsg(const uint8_t* encryptedMessage, byte sizeOfMsg) {
  currentIndex = sizeOfMsg;
  memcpy(message, encryptedMessage, sizeOfMsg);
  memcpy(this->toAddress, encryptedMessage, 6);
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

uint8_t LoraMsg::getMessageLength() {
  return currentIndex;
}

PortValue LoraMsg::getPortValue(int index) {
  PortValue portValue;
  int startIndex = 12 + index * 4;
  portValue.type[0] = message[startIndex];
  portValue.type[1] = message[startIndex + 1];
  portValue.value = (message[startIndex + 2] << 8) | message[startIndex + 3];
  return portValue;
}

uint8_t LoraMsg::numberOfPortValues() {
  return (currentIndex - 16) / 4;
}

void LoraMsg::printMessage() {
  for (int i = 0; i < currentIndex; i++) {
    if (message[i] < 0x10) Serial.print("0");
    Serial.print(message[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
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
    address[i] = message[i + 6];
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

uint16_t LoraMsg::getMessageID() {
  return (message[14] << 8) | message[15];
}

void LoraMsg::encryptMessage() {
  uint16_t crc = 0;
  for (int i = 0; i < currentIndex; i++) {
    crc += message[i];
  }

  message[currentIndex++] = 'C';
  message[currentIndex++] = 'S';
  message[currentIndex++] = (crc >> 8) & 0xFF;
  message[currentIndex++] = crc & 0xFF;
}

void LoraMsg::decryptMessage() {
  // Frame validation and CRC check
}

bool LoraMsg::isForMe(const uint8_t* address) {
  for (int i = 0; i < 6; i++) {
    if (message[i] != address[i]) return false;
  }
  return true;
}
