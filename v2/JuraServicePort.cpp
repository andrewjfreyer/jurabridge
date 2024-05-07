#include "JuraServicePort.h"

/* init from JuraConfiguration.h variables */
JuraServicePort::JuraServicePort(SemaphoreHandle_t &xUARTSemaphoreRef) :  _xUARTSemaphore(xUARTSemaphoreRef), juraSerial(DEV_BOARD_UART_ID) {
  juraSerial.begin(9600, SERIAL_8N1, DEV_BOARD_UART_RX_PIN, DEV_BOARD_UART_TX_PIN);
  juraSerial.setRxTimeout(DEV_BOARD_UART_TIMEOUT);
}

/* known commands generated from static strings for memory management */
String JuraServicePort::transferEncodeCommand(JuraServicePortCommand command) {
  switch (command){
    /* historical data */
    case JuraServicePortCommand::RT0:
      return this->transferEncode(STATIC_CMD_RT0);
      break;
    case JuraServicePortCommand::RT1:
      return this->transferEncode(STATIC_CMD_RT1);
      break;
    case JuraServicePortCommand::RT2:
      return this->transferEncode(STATIC_CMD_RT2);
      break;
    case JuraServicePortCommand::RT3:
      return this->transferEncode(STATIC_CMD_RT3);
      break;
    
    /* real-time data */
    case JuraServicePortCommand::IC:
      return this->transferEncode(STATIC_CMD_IC);
      break;
    case JuraServicePortCommand::HZ:
      return this->transferEncode(STATIC_CMD_HZ);
      break;
    case JuraServicePortCommand::CS:
      return this->transferEncode(STATIC_CMD_CS);
      break;

    /* machine commands */
    case JuraServicePortCommand::FA_01:
      return this->transferEncode(STATIC_CMD_FA_01);
      break;
    case JuraServicePortCommand::FA_02:
      return this->transferEncode(STATIC_CMD_FA_02);
      break;
    case JuraServicePortCommand::FA_03:
      return this->transferEncode(STATIC_CMD_FA_03);
      break;
    case JuraServicePortCommand::FA_04:
      return this->transferEncode(STATIC_CMD_FA_04);
      break;
    case JuraServicePortCommand::FA_05:
      return this->transferEncode(STATIC_CMD_FA_05);
      break;
    case JuraServicePortCommand::FA_06:
      return this->transferEncode(STATIC_CMD_FA_06);
      break;
    case JuraServicePortCommand::FA_07:
      return this->transferEncode(STATIC_CMD_FA_07);
      break;
    case JuraServicePortCommand::FA_08:
      return this->transferEncode(STATIC_CMD_FA_08);
      break;
    case JuraServicePortCommand::FA_09:
      return this->transferEncode(STATIC_CMD_FA_09);
      break;
    case JuraServicePortCommand::FA_0A:
      return this->transferEncode(STATIC_CMD_FA_0A);
      break;
    case JuraServicePortCommand::FA_0B:
      return this->transferEncode(STATIC_CMD_FA_0B);
      break;
    case JuraServicePortCommand::FA_0C:
      return this->transferEncode(STATIC_CMD_FA_0C);
      break;

    default: 
      break;
  }
  return "";
}

String JuraServicePort::transferEncode(String outbytes) {

  /* take the shared semaphore for UART; if sampling should be paused then block this request  */
  xSemaphoreTake( _xUARTSemaphore, portMAX_DELAY );

  String inbytes;
  inbytes.reserve(100);
  int w = 0;

  // Timeout for available read
  while (juraSerial.available()) {
    juraSerial.read();
  }
  
  outbytes += "\r\n";
  for (int i = 0; i < outbytes.length(); i++) {
    for (int s = 0; s < 8; s += 2) {
      char rawbyte = 255;
      bitWrite(rawbyte, 2, bitRead(outbytes.charAt(i), s + 0));
      bitWrite(rawbyte, 5, bitRead(outbytes.charAt(i), s + 1));
      juraSerial.write(rawbyte);
    }
     vTaskDelay(8 / portTICK_PERIOD_MS);
  }

  int s = 0;
  char inbyte;
  while (!inbytes.endsWith("\r\n")) {
    if (juraSerial.available()) {
      byte rawbyte = juraSerial.read();
      bitWrite(inbyte, s + 0, bitRead(rawbyte, 2));
      bitWrite(inbyte, s + 1, bitRead(rawbyte, 5));
      if ((s += 2) >= 8) {
        s = 0;
        inbytes += inbyte;
      }
    } else {
       vTaskDelay( 10 / portTICK_PERIOD_MS);
    }
    if (w++ > 500) {
      isConnected = false;
      xSemaphoreGive( _xUARTSemaphore);
      return "";
    }
  }

  /* give back the semaphore here */
  xSemaphoreGive( _xUARTSemaphore);

  /* Return full rx response without status prefix (e.g., "IC:...") */
  if (inbytes.length() > 3) {
    isConnected = true;
    return inbytes.substring(3, inbytes.length() - 2);
  } else if (inbytes.length() > 0 ) {
    isConnected = true;
    return inbytes.substring(0, inbytes.length() - 2);
  }else {
    isConnected = false;
    return "";
  }
}
