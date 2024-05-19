#ifndef JURAWORKINGMEMORY_H
#define JURAWORKINGMEMORY_H
#include "JuraConfiguration.h"
#include "JuraServicePort.h"
#include <cstdlib>
#include <string>

/* valid response from jura service port */
#define WORKING_MEMORY_UART_RESPONSE_LEN 4
#define RM_BIN_SIZE 16
#define RM_DEC_SIZE 4
#define CHANGE_TIMEOUT_VALUE 3500 //TODO: change detection needs improvement

/* how should we interpret the input board data type */
enum class JuraWorkingMemoryBinaryResponseInterpretation   {Inverted, AsReported};
enum class JuraWorkingMemoryResponseDataType               {Binary, Decimal};
enum class JuraWorkingMemoryReturnType                     {Value, IsChanging};

class JuraWorkingMemory {
public:
  JuraWorkingMemory();

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

  /* value store as boolean */
  bool val_bin_ignore [RM_BIN_SIZE] =                           {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  bool val_bin [RM_BIN_SIZE] =                                  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  bool val_bin_prev [RM_BIN_SIZE] =                             {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  /* change detection */
  bool val_bin_new_available [RM_BIN_SIZE] =                    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  /* timestamps */
  long val_bin_last_changed_ms [RM_BIN_SIZE] =                  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  long val_bin_prev_last_changed_ms [RM_BIN_SIZE] =             {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  
  /* ==== DECIMAL interpretation of input board response values == INTEGER data types ==== */
  /* value store as integer */
  int val_dec [RM_DEC_SIZE] =                                   {0,0,0,0};
  int val_dec_prev [RM_DEC_SIZE] =                              {0,0,0,0};

  /* change detection */
  bool val_dec_new_available [RM_DEC_SIZE] =                    {0,0,0,0};

  /* timestamps */
  long val_dec_last_changed_ms [RM_DEC_SIZE] =                  {0,0,0,0};
  long val_dec_prev_last_changed_ms [RM_DEC_SIZE] =             {0,0,0,0};
};

#endif
