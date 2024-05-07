#ifndef JURAHEATEDBEVERAGE_H
#define JURAHEATEDBEVERAGE_H
#include "JuraConfiguration.h"
#include "JuraServicePort.h"
#include <cstdlib>
#include <string>

/* valid response from jura service port */
#define HZ_UART_RESPONSE_LEN 44
#define HZ_BIN_SIZE 22

class JuraHeatedBeverage {
public:
  JuraHeatedBeverage();

  /* caller to service port with instance-configured command; if no change from previous result, return false */
  bool  didUpdate             (int, JuraServicePort &);

  /* sets the poll rate; didUpdate will return 0 if poll iterator is false */
  void  setPollRate           (int);
  void  setMixIndexIgnore     (int);
  void  setCommand            (JuraServicePortCommand);
  bool  validIndexForType     (int);

  /*  boolean or integer interpretation of service port responses  */
  bool  hasUpdateForValueOfServicePortResponseSubstringIndex               (int);
  int   returnValueOfServicePortResponseSubstringIndex                     (int);

  /* investigations into unhandled updates */
  void  printUnhandledUpdate  ();
  bool  hasUnhandledUpdate    ();
  
private:
  JuraServicePortCommand _default_command;
  int _poll_rate;

  /* ==== BINARY INTERPRETATION OF INPUT BOARD RESPONSE VALUES == BOOLEAN DATA TYPES ==== */
  /* value store as boolean */
  int val_mix_ignore [HZ_BIN_SIZE] =                           {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int val_mix [HZ_BIN_SIZE] =                                  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int val_mix_prev [HZ_BIN_SIZE] =                             {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  /* change detection */
  bool val_mix_new_available [HZ_BIN_SIZE] =                   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  /* timestamps */
  long val_mix_last_changed_ms [HZ_BIN_SIZE] =                 {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  long val_mix_prev_last_changed_ms [HZ_BIN_SIZE] =            {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    
};

#endif
