#ifndef JURAINPUTCONTROLBOARD_H
#define JURAINPUTCONTROLBOARD_H
#include "JuraConfiguration.h"
#include "JuraServicePort.h"
#include <cstdlib>
#include <string>

/* valid response from jura service port */
#define INPUT_CONTROLLER_UART_RESPONSE_LEN 4
#define IC_BIN_SIZE 16
#define IC_DEC_SIZE 4
#define CHANGE_TIMEOUT_VALUE 3500 //TODO: change detection needs improvement

/* how should we interpret the input board data type */
enum class JuraInputBoardBinaryResponseInterpretation   {Inverted, AsReported};
enum class JuraInputBoardResponseDataType               {Binary, Decimal};
enum class JuraInputBoardReturnType                     {Value, IsChanging};

class JuraInputControlBoard {
public:
  JuraInputControlBoard();

  /* caller to service port with instance-configured command; if no change from previous result, return false */
  bool  didUpdate             (int, JuraServicePort &);

  /* sets the poll rate; didUpdate will return 0 if poll iterator is false */
  void  setPollRate           (int);
  void  setBinIndexIgnore     (int, int);
  void  setCommand            (JuraServicePortCommand);
  bool  validIndexForType     (int);

  /*  boolean or integer interpretation of service port responses  */
  bool  hasUpdateForValueOfServicePortResponseSubstringIndex               (int, int);
  int   returnValueOfServicePortResponseSubstringIndex                     (int, int);

  /* investigations into unhandled updates */
  void  printUnhandledUpdate  ();
  bool  hasUnhandledUpdate    ();
  

private:
  JuraServicePortCommand _default_command;
  int _poll_rate;

  /* structured data representations */
  /* 
    0000 0000 0000 0000
    ^||| |||| |||| ||||  Drainage Tray 0
     ^|| |||| |||| ||||  Bypass Doser 1
      ^| |||| |||| ||||  Water Tank 2
       ^ |||| |||| ||||  Bean Hopper 3
         ^^|| |||| ||||  Brewgroup Revolution Counter 4 - 5 
           ^| |||| ||||  Flow Meter Revolution Counter 6
                     ^^  Output Valve Servo Position 14 - 15 
            ? ???? ??     Unknown       
  */

  /* ==== BINARY INTERPRETATION OF INPUT BOARD RESPONSE VALUES == BOOLEAN DATA TYPES ==== */
  /* value store as boolean */
  bool val_bin_ignore [IC_BIN_SIZE] =                           {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  bool val_bin [IC_BIN_SIZE] =                                  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  bool val_bin_prev [IC_BIN_SIZE] =                             {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  /* change detection */
  bool val_bin_new_available [IC_BIN_SIZE] =                    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  /* timestamps */
  long val_bin_last_changed_ms [IC_BIN_SIZE] =                  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  long val_bin_prev_last_changed_ms [IC_BIN_SIZE] =             {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  
  /* ==== DECIMAL interpretation of input board response values == INTEGER data types ==== */
  /* value store as integer */
  int val_dec [IC_DEC_SIZE] =                                   {0,0,0,0};
  int val_dec_prev [IC_DEC_SIZE] =                              {0,0,0,0};

  /* change detection */
  bool val_dec_new_available [IC_DEC_SIZE] =                    {0,0,0,0};

  /* timestamps */
  long val_dec_last_changed_ms [IC_DEC_SIZE] =                  {0,0,0,0};
  long val_dec_prev_last_changed_ms [IC_DEC_SIZE] =             {0,0,0,0};
};

#endif
