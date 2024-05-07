#ifndef JURAMEMORYLINE_H
#define JURAMEMORYLINE_H
#include "JuraConfiguration.h"
#include "JuraServicePort.h"
#include <cstdlib>
#include <string>

/* invalid response from jura service port */
#define EEPROM_UART_RESPONSE_LEN 64
#define RT_BIN_SIZE 16

class JuraMemoryLine {
public:
  JuraMemoryLine();

  /* caller to service port with instance-configured command; if no change from previous result, return false */
  bool  didUpdate             (int, JuraServicePort &);

  /* sets the poll rate; didUpdate will return 0 if poll iterator is false */
  void  setPollRate           (int);
  void  setCommand            (JuraServicePortCommand);

  /* after parsing response string into substrings and interpreting as int/hex/bin, store in array */
  int   checkIntegerValueOfServicePortResponseSubstringIndex               (int);
  bool  hasUpdateForIntegerValueOfServicePortResponseSubstringIndex        (int);

  /* investigations into unhandled updates */
  void  printUnhandledUpdate  ();
  bool  hasUnhandledUpdate    ();

private:
  JuraServicePortCommand _default_command;
  int _poll_rate;

  // Private arrays
  long val_dec [RT_BIN_SIZE] =                   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  long val_dec_prev [RT_BIN_SIZE] =              {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  bool val_dec_new_available [RT_BIN_SIZE] =     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  long val_dec_last_changed [RT_BIN_SIZE] =      {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  long val_dec_prev_last_changed [RT_BIN_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
};

#endif
