#include "JuraMemoryLine.h"

JuraMemoryLine::JuraMemoryLine() {
  _poll_rate = 0;
}

/* command for each individual memory line handler is treated as status, but doesn't really matter */
void JuraMemoryLine::setCommand(JuraServicePortCommand command){
  _default_command = command;
}

/* 
  each JuraMemoryLine instance handles it's own refreshing (ping coffee machine, process results) so this determines whether
  a particular call to didUpdate with a particular iterator will actually execute anything or will skip by returning 
  false almost immeidately. 

  poll rate = processor speed of executing a single instance of loop()
*/
void JuraMemoryLine::setPollRate(int modulus){
  _poll_rate = modulus;
}

bool JuraMemoryLine::hasUnhandledUpdate(){
  bool unhandled = false; 
  for (int i = 0; i< RT_BIN_SIZE; i++){
    if (val_dec_new_available[i] && val_dec_prev[i] > 0){
      unhandled = true; break;
    }
  }
  return unhandled;
}

void JuraMemoryLine::printUnhandledUpdate(){
  bool unhandled = false; 
  for (int i = 0; i< RT_BIN_SIZE; i++){
    if ((val_dec_new_available[i] && val_dec_prev[i] > 0) && (val_dec_prev[i] != val_dec[i])){
      ESP_LOGI(TAG,"UKN: [cmd:RT%d] [i:%d] = %d -> %d", (int) _default_command - 1, i, val_dec_prev[i], val_dec[i]);
      val_dec_new_available[i] = false;
    }
  }
}

bool JuraMemoryLine::hasUpdateForIntegerValueOfServicePortResponseSubstringIndex(int index){
  if (index >= RT_BIN_SIZE){return false;}
  return val_dec_new_available[index];  
}

int JuraMemoryLine::checkIntegerValueOfServicePortResponseSubstringIndex(int index){
  if (index >= RT_BIN_SIZE){return 0;}
  val_dec_new_available[index] = false;  
  return val_dec[index];
}

bool JuraMemoryLine::didUpdate(int iterator, JuraServicePort &servicePort) {
  /* able to disable */
  if (_poll_rate == 0) {return false;}

  if (iterator % _poll_rate != 0){return false;}
  String comparator_string; comparator_string.reserve (100);
  comparator_string = servicePort.transferEncodeCommand(_default_command);

  /* invalid response */
  if (comparator_string.length() != EEPROM_UART_RESPONSE_LEN){
    return false; 
  }

  bool hasChanged = false;

  //populate array for comparison
  for (int i = 0; i < RT_BIN_SIZE; i++) {
    //end 
    int start = i * 4; int end = start + 4;
    
    //decompose string to integer array
    long value = strtol(comparator_string.substring(start, end).c_str(), NULL, 16);  

    //set hasChanged flag if we've timed out 
    bool hasExpired = false; 
    if ((millis() - val_dec_last_changed[i]) > JURA_MACHINE_EEPROM_TIMEOUT){
      hasExpired = true;
    }

    //compare val storage
    if (val_dec[i] != value || hasExpired){
      hasChanged = true;
      val_dec_prev_last_changed[i] = val_dec_last_changed[i];
      val_dec_last_changed[i] = millis();
      
      val_dec_prev[i] = val_dec[i];
      val_dec_new_available[i] = true;

      val_dec[i] = value;
    }
  }
  return hasChanged;
}
