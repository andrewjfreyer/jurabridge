#ifndef JURASERVICEPORT_H
#define JURASERVICEPORT_H
#include <Arduino.h>
#include "JuraConfiguration.h"

class JuraServicePort {
public:
  JuraServicePort(SemaphoreHandle_t &);
  bool isConnected;
  

  /* from cmd2jura */
  String transferEncode(String);
  String transferEncodeCommand(JuraServicePortCommand);

private:
  HardwareSerial juraSerial;
  SemaphoreHandle_t &_xUARTSemaphore;

  /* ----- constants ----- */

  inline static const String STATIC_CMD_TL = "TL:";
  inline static const String STATIC_CMD_TY = "TY:";

  /* EEPROM */
  inline static const String STATIC_CMD_RT0  =  "RT:0000";
  inline static const String STATIC_CMD_RT1  =  "RT:0010";
  inline static const String STATIC_CMD_RT2  =  "RT:0020";
  inline static const String STATIC_CMD_RT3  =  "RT:0030";

  inline static const String STATIC_CMD_IC  =  "IC:";
  inline static const String STATIC_CMD_HZ  =  "HZ:";
  inline static const String STATIC_CMD_CS  =  "CS:";

  /* prefer static commands so no unknown commands are sent to the machine */
  inline static const String STATIC_CMD_FA_01	=	"FA:01";
  inline static const String STATIC_CMD_FA_02	=	"FA:02";
  inline static const String STATIC_CMD_FA_03	=	"FA:03";
  inline static const String STATIC_CMD_FA_04	=	"FA:04";
  inline static const String STATIC_CMD_FA_05	=	"FA:05";
  inline static const String STATIC_CMD_FA_06	=	"FA:06";
  inline static const String STATIC_CMD_FA_07	=	"FA:07";
  inline static const String STATIC_CMD_FA_08	=	"FA:08";
  inline static const String STATIC_CMD_FA_09	=	"FA:09";
  inline static const String STATIC_CMD_FA_0A	=	"FA:0A";
  inline static const String STATIC_CMD_FA_0B	=	"FA:0B";
  inline static const String STATIC_CMD_FA_0C	=	"FA:0C";

};

#endif