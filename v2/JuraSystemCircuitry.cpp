#include "JuraSystemCircuitry.h"

JuraSystemCircuitry::JuraSystemCircuitry() {
  _poll_rate = 0;
}

/* simple doublecheck for proper sizing */
bool JuraSystemCircuitry::validIndexForType (int index){
  if (index >= CS_BIN_SIZE) {return false;}
  return true;
}

/* ignore these */
void JuraSystemCircuitry::setDecIndexIgnore(int index){
  if (!validIndexForType(index)){return;}
  val_dec_ignore[index] = true; 
}

void JuraSystemCircuitry::setBinIndexIgnore(int index, int bits = 1){
  if (!validIndexForType(index + (bits - 1))){return;}
  for (int i = index; i < index + bits; i ++){
    val_bin_ignore[i] = true; 
  }
}

/* reduce race conditions; only check for value when it has changed properly */
bool JuraSystemCircuitry::hasUpdateForValueOfServicePortResponseSubstringIndex (int index, int bits = 1){
  if (!validIndexForType(index + (bits - 1))){return false;}
  bool _isChanging = false;
  for (int i = index; i < index + bits; i ++){
    if (val_bin_ignore[i]){return false;}
    if (val_bin_new_available[i]) {
      _isChanging = true;
    }
  }
  return _isChanging; 
}

/* getter for values at a specific substring index */
int JuraSystemCircuitry::returnValueOfServicePortResponseSubstringIndex (int index, int bits = 1){
  if (!validIndexForType(index + (bits - 1))){return false;}
  int exponent = bits - 1;  int decimal = 0;
  for (int i = index; i < index + bits; i ++){
    val_bin_new_available[i] = false;
    decimal += (pow(2, exponent) * val_bin[i]);
    exponent--;
  }
  return decimal;
}

int JuraSystemCircuitry::returnHammingValueOfServicePortResponseSubstringIndex (int index, int bits = 1){
  if (!validIndexForType(index + (bits - 1))){return false;}
  int hamming = 0;
  for (int i = index; i < index + bits; i ++){
    val_bin_new_available[i] = false;
    hamming += val_bin[i];
  }
  return hamming;
}

/* reduce race conditions; only check for value when it has changed properly */
bool JuraSystemCircuitry::hasUpdateForDecimalValueOfServicePortResponseSubstringIndex (int index){
  if (val_dec_ignore[index]){return false;}
  return val_dec_new_available[index];  
}

/* last changed ?? */
int JuraSystemCircuitry::lastChangeForDecimalValueOfServicePortResponseSubstringIndex (int index){
  return val_dec_last_changed_ms[index];  
}

/* getter for values at a specific substring index */
int JuraSystemCircuitry::returnDecimalValueOfServicePortResponseSubstringIndex (int index){
  val_dec_new_available[index] = false;  

  /* flag binary too */
  for(int i = 0; i < 16; i++){
    val_bin_new_available[index * 16 + i] = false;
  }

  /* return decimal value */
  return val_dec[index];
}

/* poll the passed service port instance to determine whether the output from the machine has changed; parse as decimal and Decimal */
bool JuraSystemCircuitry::didUpdate(int iterator, JuraServicePort &servicePort) {
  if (_poll_rate == 0) {return false;}
  if (iterator % _poll_rate != 0 ){return false;}
  String comparator_string; comparator_string.reserve (100);
  comparator_string = servicePort.transferEncodeCommand(this->_default_command);

  /* invalid response */
  if (!((int) comparator_string.length() >= (int) SYSTEM_CIRCUITRY_UART_RESPONSE_LEN ) ){
    return false; 
  }

  //find the location of the difference for marking & debugging
  bool hasChanged = false;

  //for decomposition into Decimal array
  for (int i = 0; i < CS_DEC_SIZE; i++) {
    int start = i * 4; int end = start + 4;
  
    //decompose string to Decimal array
    long value = strtol(comparator_string.substring(start, end).c_str(), NULL, 16);
    
    //odlval 
    val_dec_prev[i] = val_dec[i];

    //compare val storage
    if (val_dec[i] != value ){
      hasChanged = true; 
      val_dec_new_available[i] = true;
      val_dec_prev_last_changed_ms[i] = val_dec_last_changed_ms[i];
      val_dec_last_changed_ms[i] = millis();
      val_dec[i] = value;
    }
  }

  //through each binary representation of the decimal values
  for (int i = 0; i < CS_BIN_SIZE; i++) {
    
    //find the equivalent subpart of the command response
    int substring_index = (int) (i / 16);
    int bitwise_comparator_exponent = 15 - (i % 16); // mod will go from 0,1,2,3 as iterator increases, invert by subtract from three to get 3,2,1,0, this is expoonent of 2 to compar 
    int bitwise_comparator = pow(2, bitwise_comparator_exponent); // 8,4,2,1 for each iterator
    
    //get value to compare bitwise comparator to 
    int substring_as_long = val_dec[substring_index];

    //set value, which is binary compar
    bool value = (substring_as_long & (int) bitwise_comparator) != 0;
            
    //reset change
    val_bin_prev[i] = val_bin[i];

    /* timeout */
    bool hasExpired = false; 
    if ((millis() - val_bin_last_changed_ms[i]) > JURA_MACHINE_SYSTEM_CIRCUITRY_TIMEOUT){
      hasExpired = true;
    }

    //compare val storage
    if (val_bin[i] != value || hasExpired ){
      hasChanged = true; 
      val_bin_new_available[i] = true;
      val_bin_prev_last_changed_ms[i] = val_bin_last_changed_ms[i];
      val_bin_last_changed_ms[i] = millis();
      val_bin[i] = value;
    }
  }

  return hasChanged;
}

/* command for each individual memory line handler is treated as status, but doesn't really matter */
void JuraSystemCircuitry::setCommand(JuraServicePortCommand command){
  _default_command = command;
}

/* poll rate = processor speed of executing a single instance of loop() */
void JuraSystemCircuitry::setPollRate(int modulus){
  _poll_rate = modulus;
}

/* */
bool JuraSystemCircuitry::hasUnhandledUpdate(){
  bool unhandled = false; 
  for (int i = 0; i< CS_BIN_SIZE; i++){
    if (val_bin_new_available[i]){
      unhandled = true; break;
    }
  }
  return unhandled;
}

void JuraSystemCircuitry::printUnhandledUpdate(){
  for (int i = 0; i< CS_BIN_SIZE; i++){
    if (val_bin_new_available[i]){
      int dec_index = (int)(i / 16);
      if (val_bin_prev[i] != val_bin[i]){ 
        if (!val_bin_ignore[i]){
          ESP_LOGI(TAG,"UKN: [cmd:CS] [0x i:%d] [0b i:%d] = %d -> %d (0d = %i)", dec_index, i, val_bin_prev[i], val_bin[i], val_dec[dec_index]);
          val_bin_new_available[i] = false;
        }
      }
    }
  }
}
