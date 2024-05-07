#ifndef JURASYSTEMCIRCUITRY_H
#define JURASYSTEMCIRCUITRY_H

#include "JuraConfiguration.h"
#include "JuraServicePort.h"
#include <cstdlib>
#include <string>

/* valid response from jura service port */
#define SYSTEM_CIRCUITRY_UART_RESPONSE_LEN 44 // sometimes goes to 48???
#define CS_BIN_SIZE 192
#define CS_DEC_SIZE 12

/* how should we interpret the input board data type */
enum class JuraSystemCircuitryBinaryResponseInterpretation {Inverted, AsReported};
enum class JuraSystemCircuitryResponseDataType {Binary, Decimal, Hamming};

class JuraSystemCircuitry {
public:
  JuraSystemCircuitry();

  /* caller to service port with instance-configured command; if no change from previous result, return false */
  bool  didUpdate             (int, JuraServicePort &);

  /* sets the poll rate; didUpdate will return 0 if poll iterator is false */
  void  setPollRate           (int);
  void  setDecIndexIgnore     (int);
  void  setBinIndexIgnore     (int, int);
  void  setCommand            (JuraServicePortCommand);
  bool  validIndexForType     (int);

  /*  boolean or integer interpretation of service port responses  */
  bool  hasUpdateForValueOfServicePortResponseSubstringIndex               (int, int);
  int   returnValueOfServicePortResponseSubstringIndex                     (int, int);
  int   returnHammingValueOfServicePortResponseSubstringIndex              (int, int);

  bool  hasUpdateForDecimalValueOfServicePortResponseSubstringIndex        (int);
  int   returnDecimalValueOfServicePortResponseSubstringIndex              (int);
  int   lastChangeForDecimalValueOfServicePortResponseSubstringIndex       (int);

  /* investigations into unhandled updates */
  void  printUnhandledUpdate  ();
  bool  hasUnhandledUpdate    ();
  

private:
  JuraServicePortCommand _default_command;
  int _poll_rate;

  /* ==== BINARY INTERPRETATION OF INPUT BOARD RESPONSE VALUES == BOOLEAN DATA TYPES ==== */
  /* value store as boolean */
  bool val_bin_ignore [CS_BIN_SIZE] =                           { 
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                };

  bool val_bin [CS_BIN_SIZE] =                                  {
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [0] 0-15
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [1] 16-31
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [2] 32-47
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [3] 48-63
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [4] 64-79
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [5] 80-95
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [6] 96-111
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [7] 112-127
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [8] 128-143 
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [9] 144-159
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [10] 160-175
                                                                  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // [11] 176-191
                                                                };

  bool val_bin_prev [CS_BIN_SIZE] =                             { 
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                };

  /* change detection */
  bool val_bin_new_available [CS_BIN_SIZE] =                    { 
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                };

  /* timestamps */
  long val_bin_last_changed_ms [CS_BIN_SIZE] =                  { 
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                                                                  
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                };

  long val_bin_prev_last_changed_ms [CS_BIN_SIZE] =             { 
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                                                };
  
  /* ==== DECIMAL interpretation of input board response values == INTEGER data types ==== */
  /* value store as integer */
  int val_dec_ignore [CS_DEC_SIZE] =                            {0,0,0,0,0,0,0,0,0,0,0,0};
  int val_dec [CS_DEC_SIZE] =                                   {0,0,0,0,0,0,0,0,0,0,0,0};
  int val_dec_prev [CS_DEC_SIZE] =                              {0,0,0,0,0,0,0,0,0,0,0,0};

  /* change detection */
  bool val_dec_new_available [CS_DEC_SIZE] =                    {0,0,0,0,0,0,0,0,0,0,0,0};

  /* timestamps */
  long val_dec_last_changed_ms [CS_DEC_SIZE] =                  {0,0,0,0,0,0,0,0,0,0,0,0};
  long val_dec_prev_last_changed_ms [CS_DEC_SIZE] =             {0,0,0,0,0,0,0,0,0,0,0,0};
  
  /**/
  bool val_dec_is_changing [CS_DEC_SIZE] =                      {0,0,0,0,0,0,0,0,0,0,0,0};
  bool val_dec_is_changing_new_available [CS_DEC_SIZE] =        {0,0,0,0,0,0,0,0,0,0,0,0};
};

#endif
