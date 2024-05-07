#include "JuraHeatedBeverage.h"

JuraHeatedBeverage::JuraHeatedBeverage() {
  _poll_rate = 0;
}

/* simple doublecheck for proper sizing */
bool JuraHeatedBeverage::validIndexForType (int index){
  if (index >= HZ_BIN_SIZE) {return false;}
  return true;
}

/* ignore these */
void JuraHeatedBeverage::setMixIndexIgnore(int index){
  if (!validIndexForType(index)){return;}
  val_mix_ignore[index] = true; 
}

/* reduce race conditions; only check for value when it has changed properly */
bool JuraHeatedBeverage::hasUpdateForValueOfServicePortResponseSubstringIndex (int index){
  if (val_mix_ignore[index]){return false;}
  return val_mix_new_available[index];  
}

/* getter for values at a specific substring index */
int JuraHeatedBeverage::returnValueOfServicePortResponseSubstringIndex (int index){
  val_mix_new_available[index] = false;  
  return val_mix[index];
}

/* poll the passed service port instance to determine whether the output from the machine has changed */
bool JuraHeatedBeverage::didUpdate(int iterator, JuraServicePort &servicePort) {
  if (_poll_rate == 0) {return false;}

  if (iterator % _poll_rate != 0 ){return false;}

  /*command execution; todo: one day, stage all commands in a queue, and process items from the queue in parallel? */
  String comparator_string; comparator_string.reserve (100);
  comparator_string = servicePort.transferEncodeCommand(this->_default_command);

  /* invalid response */
  if (comparator_string.length() != HZ_UART_RESPONSE_LEN){
    return false; 
  }
  
  bool hasChanged = false;

  //populate array for comparison
  for (int i = 0; i < HZ_UART_RESPONSE_LEN; i++) {
    int start; int end; int index;

    /* parse the hz output into a single array */
    if (i < 11)        {start = i; end = i + 1; index = i;} //binary values first bit;
    else if (i == 12)  {start = 12; end = 16; index = 11;} //first hex
    else if (i == 17)  {start = 17; end = 21; index = 12;} //second hex
    else if (i == 22)  {start = 22; end = 26; index = 13;} //third hex
    else if (i == 27)  {start = 27; end = 31; index = 14;} //fourth hex
    else if (i == 32)  {start = 32; end = 36; index = 15;} //fifth hex
    else if (i == 37)  {start = 37; end = 38; index = 16;} //single bit for valve
    else if (i > 38)   {start = i; end = i + 1; index = i - 22;} //binary values first bit;
    else {continue;} //if the index is not specifically defined above we skip it

    //decompose string to integer array
    long val = strtol(comparator_string.substring(start, end).c_str(), NULL, 16);

    /*force updates on timeout */
    bool hasExpired = false; 
    if ((millis() - val_mix_prev_last_changed_ms[i]) > JURA_MACHINE_HEATED_BEVERAGE_TIMEOUT){
      hasExpired = true;
    }

    //reset
    val_mix_prev[index] = val_mix[index];

    //compare val storage
    if ((val_mix[index] != val && index < 22) || hasExpired ){

      hasChanged = true; 
      val_mix_prev_last_changed_ms[index] = val_mix_last_changed_ms[index];
      val_mix_last_changed_ms[index] = millis();
      val_mix_new_available[index] = true;
      val_mix[index] = val;
    }
  }

  return hasChanged;
}

/* command for each individual memory line handler is treated as status, but doesn't really matter */
void JuraHeatedBeverage::setCommand(JuraServicePortCommand command){
  _default_command = command;
}

/* poll rate = processor speed of executing a single instance of loop() */
void JuraHeatedBeverage::setPollRate(int modulus){
  _poll_rate = modulus;
}

bool JuraHeatedBeverage::hasUnhandledUpdate(){
  bool unhandled = false; 
  for (int i = 0; i< HZ_BIN_SIZE; i++){
    if (val_mix_new_available[i]){
      unhandled = true; break;
    }
  }
  return unhandled;
}

void JuraHeatedBeverage::printUnhandledUpdate(){
  bool unhandled = false; 
  for (int i = 0; i< HZ_BIN_SIZE; i++){
    if (val_mix_new_available[i]) {
      if (val_mix_prev[i] != val_mix[i]){
        if (! val_mix_ignore[i]){
          ESP_LOGI(TAG,"UKN: [cmd:HZ] [i:%d] = %d -> %d", i, val_mix_prev[i], val_mix[i]);
          val_mix_new_available[i] = false; 
        }
      }
    }
  }
}
