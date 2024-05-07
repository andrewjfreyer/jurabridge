#include "JuraBridge.h"
#include "JuraMachine.h"

/* set poll rates */
#define POLL_DUTY_FULL  1
#define POLL_DUTY_50    2
#define POLL_DUTY_33    3
#define POLL_DUTY_20    5
#define POLL_DUTY_15    7
#define POLL_DUTY_10    11
#define POLL_DUTY_5     23
#define POLL_DISABLED   101

JuraMachine::JuraMachine(JuraBridge& bridge, SemaphoreHandle_t &xMachineReadyStateVariableSemaphoreRef) :  _bridge(&bridge), xMachineReadyStateVariableSemaphore(xMachineReadyStateVariableSemaphoreRef) {

  /* semaphore for dispense limit setting */
  xDispenseLimitSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive( xDispenseLimitSemaphore);

  /*eeprom*/
  _rt0.setCommand(JuraServicePortCommand::RT0);
  _rt1.setCommand(JuraServicePortCommand::RT1);
  _rt2.setCommand(JuraServicePortCommand::RT2);

  /* input board and controller */
  _ic.setCommand(JuraServicePortCommand::IC);

  /* heated beverage */
  _hz.setCommand(JuraServicePortCommand::HZ);
  
  /* system circuitry  */
  _cs.setCommand(JuraServicePortCommand::CS);
  
  /* known ignores */
  _cs.setBinIndexIgnore(SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_1, 10);
  _cs.setBinIndexIgnore(SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_2, 10);
  _cs.setBinIndexIgnore(SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_3, 10);
  _cs.setBinIndexIgnore(SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_4, 10);
  _cs.setBinIndexIgnore(SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_5, 10);
    
  /*
    NOTES:
      unknown_machine_configuration_group_1 = 0; // brew group moving??
      unknown_machine_configuration_group_2 = 0; // beginning of water 1 -> 0 (1.5) seconds
      unknown_machine_configuration_group_3 = 0; // output valve encoder state 
      unknown_machine_configuration_group_4 = 0; 
      unknown_machine_configuration_group_5 = 0; // brew group ??? 
  */

  /* meta state */
  states[(int) JuraMachineStateIdentifier::OperationalState] = (int) JuraMachineOperationalState::Starting;
  states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::ExecutingOperation;

  /* fill operational state history */
  for (int i = 0; i < MAX_STATE_EVENT_HISTORY ; i++) {
    operationalStateHistory[i] = (int) JuraMachineOperationalState::Starting;
    dispenseHistory[i] = 0;
    flowRateHistory[i] = 0;

    temperatureAverageHistory[i] = 0;
    temperatureMaximumHistory[i] = 0;
    temperatureMinimumHistory[i] = 0;
  }

  /* set to zero  */
  for (int i = 0; i < MAX_DISPENSE_SAMPLES ; i++) {
    flowRateSamples[i] = 0;
    temperatureSamples[i] = 0;
  }

  /* init to starup */
  operationalStateHistory[0] = (int) JuraMachineOperationalState::Starting;
}

/***************************************************************************//**
 * Leverage an array as a linked list or queue
 *
 * @param[out] null 
 *     
 * @param[in] int arr[]
 * @param[in] int size
 * @param[in] int newValue
 ******************************************************************************/
 void JuraMachine::pushElement(int arr[], int size, int newValue) {
    if (size <= 0)
        return; // Nothing to shift

    // Shift each element one position to the right
    for (int i = size - 1; i > 0; --i) {
        arr[i] = arr[i - 1];
    }

    // Set the first element with the new value
    arr[0] = newValue;
}

/***************************************************************************//**
 * Force an input int to fall within a particular range
 *
 * @param[out] int 
 *     
 * @param[in] int inlogg
 * @param[in] int min
 * @param[in] int max
 ******************************************************************************/
 int JuraMachine::filteredLong(int inlong, int min, int max){
  if (inlong < min){return min;} if (inlong > max){return max;}
  return inlong;
}

/***************************************************************************//**
 * Receives input of a new sample of a memory line from the Jura and outputs a bool
 * indicating whether a portion of that line has changed. 
 *
 * @param[out] bool 
 *     
 * @param[in] JuraMemoryLine *memoryLine,
 * @param[in] int *value, 
 * @param[in] int value_index, 
 * @param[in] int min_val
 * @param[in] int max_val
 ******************************************************************************/
 bool JuraMachine::didUpdateJuraMemoryLineValue(
  JuraMemoryLine *memoryLine,
  int *value, 
  int value_index, 
  int min_val, 
  int max_val){

  if (memoryLine->hasUpdateForIntegerValueOfServicePortResponseSubstringIndex(value_index)){
    *value = filteredLong(
      (int) memoryLine->checkIntegerValueOfServicePortResponseSubstringIndex(value_index), 
      min_val, 
      max_val);

    return true;
  }
  return false;
}

/***************************************************************************//**
 * Receives input of a new sample of a control board service port call and outputs
 * a bool indicating whether a portion of that output has changed. 
 *
 * @param[out] bool 
 *     
 * @param[in]   int *value, 
 * @param[in]   int value_index, 
 * @param[in]   int bits, 
 * @param[in]   JuraInputBoardBinaryResponseInterpretation _invert
 ******************************************************************************/
 bool JuraMachine::didUpdateJuraInputControlBoardValue(
  int *value, 
  int value_index, 
  int bits, 
  JuraInputBoardBinaryResponseInterpretation _invert) {
  /* check for update, if update set value at passed pointer */
  if (_ic.hasUpdateForValueOfServicePortResponseSubstringIndex(value_index, bits)){
    int raw = (int) _ic.returnValueOfServicePortResponseSubstringIndex(value_index, bits);
    *value = (_invert == JuraInputBoardBinaryResponseInterpretation::Inverted) ? (!(bool) raw) : (int) raw;
    return true;
  }
  return false;
}

/***************************************************************************//**
 * Receives input of a new sample of a system control service port call and outputs
 * a bool indicating whether a portion of that output has changed. 
 *
 * @param[out] bool 
 *     
 * @param[in] int *value, 
 * @param[in] int value_index, 
 * @param[in] int bits, 
 * @param[in] JuraSystemCircuitryBinaryResponseInterpretation _invert,
 * @param[in] JuraSystemCircuitryResponseDataType _valueTyp
 ******************************************************************************/
 bool JuraMachine::didUpdateJuraSystemCircuitValue(
  int *value, 
  int value_index, 
  int bits, 
  JuraSystemCircuitryBinaryResponseInterpretation _invert,
  JuraSystemCircuitryResponseDataType _valueType) {

  /* which value are we checking? direct value or metadata (derivative?) of value?? */
  if (_valueType == JuraSystemCircuitryResponseDataType::Decimal ){

    /* check for update in change of value */
    if (_cs.hasUpdateForDecimalValueOfServicePortResponseSubstringIndex(value_index)){
      int raw = (int) _cs.returnDecimalValueOfServicePortResponseSubstringIndex(value_index);
      *value = raw;
      return true;
    } 

  }else if (_valueType == JuraSystemCircuitryResponseDataType::Binary){
    /* check for update, if update set value at passed pointer */
    if (_cs.hasUpdateForValueOfServicePortResponseSubstringIndex(value_index, bits)){
      int raw = (int) _cs.returnValueOfServicePortResponseSubstringIndex(value_index, bits);
      *value = (_invert == JuraSystemCircuitryBinaryResponseInterpretation::Inverted) ? (!(bool) raw) : (int) raw;
      return true;
    }

  }else if (_valueType == JuraSystemCircuitryResponseDataType::Hamming){
    /* check for update, if update set value at passed pointer */
    if (_cs.hasUpdateForValueOfServicePortResponseSubstringIndex(value_index, bits)){
      int raw = (int) _cs.returnHammingValueOfServicePortResponseSubstringIndex(value_index, bits);
      *value = raw;
      return true;
    }
  }
  return false;
}

/***************************************************************************//**
 * Receives input of a new sample of a heating control service port call and outputs
 * a bool indicating whether a portion of that output has changed. 
 *
 * @param[out] bool 
 *     
 * @param[in] int *value, 
 * @param[in] int value_index, 
 * @param[in] int min_val
 * @param[in] int max_val
 ******************************************************************************/
 bool JuraMachine::didUpdateJuraHeatedBeverageValue(
  int *value, 
  int value_index, 
  int min_val, 
  int max_val){
  if (_hz.hasUpdateForValueOfServicePortResponseSubstringIndex(value_index)){
    *value = filteredLong(
      (int) _hz.returnValueOfServicePortResponseSubstringIndex(value_index), 
      min_val, 
      max_val);

    return true;
  }
  return false;
}

/***************************************************************************//**
 * Sampler from generator-esque input to determine whether aggregate
 * reocmmendation state has changed
 *
 * @param[out] bool 
 *     
 * @param[in] int iterator 
 * @param[in] int duty_cycle
 ******************************************************************************/
bool JuraMachine::recommendationStateHasChanged(int iterator, int duty_cycle){
  bool priorState =  states[(int) JuraMachineStateIdentifier::HasMaintenanceRecommendation];
  bool newState = (
    states[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended] ||
    states[(int) JuraMachineStateIdentifier::CleanMilkSystemRecommended] ||
    states[(int) JuraMachineStateIdentifier::RinseBrewGroupRecommended] ||
    states[(int) JuraMachineStateIdentifier::CleanBrewGroupRecommended]);
  
  if (priorState != newState){
    /* update the maintenence recommendation */
    states[(int) JuraMachineStateIdentifier::HasMaintenanceRecommendation] = newState;
    last_changed[(int) JuraMachineStateIdentifier::HasMaintenanceRecommendation] = millis();
    return true; 
  }
  return false; 
}

/***************************************************************************//**
 * Sampler from generator-esque input to determine whether aggregate
 * error state has changed
 *
 * @param[out] bool 
 *     
 * @param[in] int iterator 
 * @param[in] int duty_cycle
 ******************************************************************************/
 bool JuraMachine::errorStateHasChanged (int iterator, int duty_cycle){

  /* return on iterator */
  if (iterator % duty_cycle != 0 ){return false;}

  bool _has_error = (
    states[(int) JuraMachineStateIdentifier::DrainageTrayFull] ||     
    states[(int) JuraMachineStateIdentifier::BeanHopperEmpty] || 
    states[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen] || 
    states[(int) JuraMachineStateIdentifier::WaterReservoirNeedsFill] || 
    states[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen] || 
    states[(int) JuraMachineStateIdentifier::DrainageTrayRemoved]);

  if (_has_error != states[(int) JuraMachineStateIdentifier::HasError]){
    states[(int) JuraMachineStateIdentifier::HasError] = _has_error;
    return true;
  }
  return false;
}

/***************************************************************************//**
 * Sampler from generator-esque input to determine whether aggregate
 * machine state has changed
 *
 * @param[out] bool 
 *     
 * @param[in] int iterator 
 * @param[in] int duty_cycle
 ******************************************************************************/
 bool JuraMachine::handleMachineState (int iterator, int duty_cycle){

  /* return on iterator */
  if (iterator % duty_cycle != 0 ){return false;}

  /* set as unknown state */
  JuraMachineOperationalState new_state = JuraMachineOperationalState::Unknown; 
  int timenow = millis();

  if ( states[(int) JuraMachineStateIdentifier::GrinderActive] == true ){
    //ESP_LOGI(TAG,"---> Machine State: Grinding ");
    states[(int) JuraMachineStateIdentifier::HasDoes] = true;
    new_state = JuraMachineOperationalState::GrindOperation; 

  } else if ( states[(int) JuraMachineStateIdentifier::BrewProgramIsCleaning] == true ){
    //ESP_LOGI(TAG,"---> Machine State: Tablet Cleaning");
    new_state = JuraMachineOperationalState::Cleaning;  
  
  } else if ( states[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen] == true ){
   //ESP_LOGI(TAG,"---> Machine State: Awaiting Powder %i ", (int) JuraMachineStateIdentifier::BypassDoserCoverOpen);
    new_state = JuraMachineOperationalState::BrewOperation; 

  } else if ( states[(int) JuraMachineStateIdentifier::VenturiPumping] == true ){

    /* machine doses before each milk preparation */
    if ( states[(int) JuraMachineStateIdentifier::HasDoes] == true ){
     //ESP_LOGI(TAG,"---> Machine State: Pumping Milk for Coffee Product");
      new_state = JuraMachineOperationalState::MilkOperation; 

    }else{
     //ESP_LOGI(TAG,"---> Machine State: Milk System Dispensing");
      new_state = JuraMachineOperationalState::MilkOperation; 
    }
  
  } else if ( states[(int) JuraMachineStateIdentifier::BrewGroupIsRinsing] == true ){
    
   //ESP_LOGI(TAG,"---> Machine State: Rinsing Brew Group");
    new_state = JuraMachineOperationalState::RinseOperation; 

  } else if ( states[(int) JuraMachineStateIdentifier::PumpActive] == true ){

    /* ceramic + output valve use together only when brewing */
    if /* PUMP ACTIVE AND */ ( states[(int) JuraMachineStateIdentifier::CeramicValveBrewingPosition] == true ){

      /* where from the output valve? */
      if ( states[(int) JuraMachineStateIdentifier::OutputValveIsBrewing] == true ){

        /* are we brewing or rinsing?? */
        if ( states[(int) JuraMachineStateIdentifier::HasDoes] == true ){
         //ESP_LOGI(TAG,"---> Machine State: Dispensing Coffee Product");
          new_state = JuraMachineOperationalState::BrewOperation; 
        
        }else{
         //ESP_LOGI(TAG,"---> Machine State: Rinsing Brew Group");
          new_state = JuraMachineOperationalState::RinseOperation; 
        }

      }else if ( states[(int) JuraMachineStateIdentifier::OutputValveIsDraining] == true ){
       //ESP_LOGI(TAG,"---> Machine State: Pumping to Drip Tray");
        new_state = JuraMachineOperationalState::RinseOperation; 
      
      }else if ( states[(int) JuraMachineStateIdentifier::OutputValveIsFlushing] == true ){
       //ESP_LOGI(TAG,"---> Machine State: Flushing System");
        new_state = JuraMachineOperationalState::RinseOperation; 
      }else{
        //ESP_LOGI(TAG,"---> Machine State: Unknown Output Valve Setting when Ceramic is Brewing");
      }

    /* ----- ceramic valve is bypassing the output valve of the brew_group ----- */

    } /* PUMP ACTIVE AND */ else if (states[(int) JuraMachineStateIdentifier::CeramicValveHotWaterPosition] == true ){
     //ESP_LOGI(TAG,"---> Machine State: Water Dispensing");
      new_state = JuraMachineOperationalState::WaterOperation; 

    } /* PUMP ACTIVE AND */ else if (states[(int) JuraMachineStateIdentifier::CeramicValveVenturiPosition] == true ){

        /* machine doses before each milk preparation */
      if ( states[(int) JuraMachineStateIdentifier::HasDoes] == true ){
       //ESP_LOGI(TAG,"---> Machine State: Milk Preparation Program Executing");
        new_state = JuraMachineOperationalState::MilkOperation; 

      }else{
       //ESP_LOGI(TAG,"---> Machine State: Milk System Dispensing");
        new_state = JuraMachineOperationalState::MilkOperation; 
      }
    
    } /* PUMP ACTIVE AND */ else if (states[(int) JuraMachineStateIdentifier::CeramicValvePressurizingPosition] == true ){
     //ESP_LOGI(TAG,"---> Machine State: System Fill");
      new_state = JuraMachineOperationalState::WaterOperation; 

    } /* PUMP ACTIVE AND */ else if (states[(int) JuraMachineStateIdentifier::CeramicValvePressureReliefPosition] == true ){
     //ESP_LOGI(TAG,"---> Machine State: Pressure Venting");
      new_state = JuraMachineOperationalState::WaterOperation; 

    } /* PUMP ACTIVE AND */ else if (states[(int) JuraMachineStateIdentifier::CeramicValveChangingPosition] == true ){
     //ESP_LOGI(TAG,"---> Machine State: Ceramic Valve Repositioning");
      new_state = JuraMachineOperationalState::WaterOperation; 

    } /* PUMP ACTIVE AND */ else if (states[(int) JuraMachineStateIdentifier::CeramicValveBrewingPosition] == true ){
      new_state = JuraMachineOperationalState::BrewOperation; 

    } /* PUMP ACTIVE AND */ else if (states[(int) JuraMachineStateIdentifier::CeramicValveSteamPosition] == true ){
      new_state = JuraMachineOperationalState::MilkOperation; 
    
    } /* PUMP ACTIVE AND */ else if (states[(int) JuraMachineStateIdentifier::CeramicValveCondenserPosition] == true ){
      new_state = JuraMachineOperationalState::ProgramPause; 
      //ESP_LOGI(TAG,"---> Machine State: Condenser");

    } else {
     //ESP_LOGI(TAG,"---> Machine State: Pumping, but unknown valve configuration");
    } 

  } else if ( ! (states[(int) JuraMachineStateIdentifier::PumpActive] == true) ){

    /* if steam mode, we're preparing for milk foam */
    if  /* PUMP STOPPED AND */ (states[(int) JuraMachineStateIdentifier::SystemSteamMode] == true  ){      

      /* thremoblock?? */
      if (states[(int) JuraMachineStateIdentifier::ThermoblockActive] == true  ){      
       //ESP_LOGI(TAG,"---> Machine State: Heating for Milk System Operation");
        new_state = JuraMachineOperationalState::HeatingOperation; 

      }else{
       //ESP_LOGI(TAG,"---> Machine State: Milk System Operating");
        new_state = JuraMachineOperationalState::MilkOperation; 

        /* awaiting user input?? */
        if (states[(int) JuraMachineStateIdentifier::ThermoblockSanitationTemperature] == true  ){    
          new_state = JuraMachineOperationalState::AwaitRotaryInput; 
        }
      }

    } /* PUMP STOPPED AND */ else {

      /* no pumping, in water mode, so we check for motion of brew_group */
      if ((states[(int) JuraMachineStateIdentifier::BrewGroupActive] == true) || (states[(int) JuraMachineStateIdentifier::BrewGroupIsReady] == false)){
          
          new_state = JuraMachineOperationalState::BrewOperation; 

      }else{
        
        /* check output valve for state of brew program */
        if ( (states[(int) JuraMachineStateIdentifier::BrewGroupIsReady] == true) && (states[(int) JuraMachineStateIdentifier::BrewProgramNumericState] == 260) ){

            /* we're in the standby state; check for errors */
            if ( states[(int) JuraMachineStateIdentifier::HasError]  == true ){
              
              if ( states[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen]  == true ){
               //ESP_LOGI(TAG,"---> Machine State: Bean Cover Open ");
                new_state = JuraMachineOperationalState::BlockingError; 
              
              }else if ( states[(int) JuraMachineStateIdentifier::WaterReservoirNeedsFill] == true ){
               //ESP_LOGI(TAG,"---> Machine State: Water Fill ");
                new_state = JuraMachineOperationalState::BlockingError; 
              
              }else if ( states[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen] == true ){
               //ESP_LOGI(TAG,"---> Machine State: Bypass Doser Open ");
                new_state = JuraMachineOperationalState::BlockingError; 
              
              }else if ( states[(int) JuraMachineStateIdentifier::DrainageTrayRemoved] == true ){
               //ESP_LOGI(TAG,"---> Machine State: Drainage Tray Removed ");
                new_state = JuraMachineOperationalState::BlockingError; 

              }else if ( states[(int) JuraMachineStateIdentifier::BeanHopperEmpty] == true ){
               //ESP_LOGI(TAG,"---> Machine State: No Beans! ");
                new_state = JuraMachineOperationalState::BlockingError; 
              }

            }else{

              /* brew group state reports ready; all other systems ready? */
              if ( states[(int) JuraMachineStateIdentifier::BrewProgramNumericState] == 260 && states[(int) JuraMachineStateIdentifier::BrewGroupIsReady] == true){

                /* thermoblock */
                if ( states[(int) JuraMachineStateIdentifier::ThermoblockActive] == true ){
                 //ESP_LOGI(TAG,"---> Machine State: Heating");
                  new_state = JuraMachineOperationalState::HeatingOperation; 
                
                }else if ( states[(int) JuraMachineStateIdentifier::ThermoblockLowMode] == true ){
                 //ESP_LOGI(TAG,"---> Machine State: Low Standby");
                  new_state = JuraMachineOperationalState::Finishing;
                
                }else if ( states[(int) JuraMachineStateIdentifier::ThermoblockColdMode] == true ){
                 //ESP_LOGI(TAG,"---> Machine State: Cold Standby");
                  new_state = JuraMachineOperationalState::Finishing;
                
                }else if ( states[(int) JuraMachineStateIdentifier::ThermoblockHighMode] == true){
                 //ESP_LOGI(TAG,"---> Machine State: High Standby");
                  new_state = JuraMachineOperationalState::Finishing;
                
                }else{
                  //ESP_LOGI(TAG,"---> Machine State: Standby");

                  new_state = JuraMachineOperationalState::Finishing;
                }

              } else if (states[(int) JuraMachineStateIdentifier::BrewProgramNumericState] == 128 ){
               //ESP_LOGI(TAG,"---> Machine State: Brew Program Finishing");
                new_state = JuraMachineOperationalState::BrewOperation;
              
              } else if (states[(int) JuraMachineStateIdentifier::BrewProgramNumericState] == 208 ){
               //ESP_LOGI(TAG,"---> Machine State: Coffee Brew Program Interrupted by User ?? ");
                new_state = JuraMachineOperationalState::BrewOperation;
              
              } else if (states[(int) JuraMachineStateIdentifier::BrewProgramNumericState] == 149 ){
               //ESP_LOGI(TAG,"---> Machine State: Espresso Brew Program Interrupted by User ?? ");
                new_state = JuraMachineOperationalState::BrewOperation;
              
              } else if (states[(int) JuraMachineStateIdentifier::BrewGroupIsReady] == false ){

                new_state = JuraMachineOperationalState::BrewOperation;

              }else{
                ESP_LOGI(TAG,"Brew Group State [UNKNOWN]: %i",  states[(int) JuraMachineStateIdentifier::BrewProgramNumericState]);
              }
            }

        } else if ( states[(int) JuraMachineStateIdentifier::HasDoes]  == true ){
         //ESP_LOGI(TAG,"---> Machine State: Brew Program Wait");
          new_state = JuraMachineOperationalState::BrewOperation;
        
        } else{
          //ESP_LOGI(TAG,"---> Machine State: Fall Back");
          new_state = JuraMachineOperationalState::BrewOperation;
        }
      }
    } 
  }

  /* ignore if unknown; probably an uncaptured intermediate state */
  if (new_state != JuraMachineOperationalState::Unknown ){
    if ((int) new_state != ((int) states[(int) JuraMachineStateIdentifier::OperationalState])){
      
      /* verify that state can change to ready? */
      if (new_state == JuraMachineOperationalState::Ready ){
        if ((int) operationalStateHistory[0] == (int) JuraMachineOperationalState::GrindOperation ||
            (int) operationalStateHistory[1] == (int) JuraMachineOperationalState::GrindOperation ||
            (int) operationalStateHistory[2] == (int) JuraMachineOperationalState::GrindOperation){
          new_state = JuraMachineOperationalState::BrewOperation;
        }
      }

      /* don't want to loop, yo */
      if (new_state == JuraMachineOperationalState::Finishing && 
          (int) states[(int) JuraMachineStateIdentifier::OperationalState] == (int) JuraMachineOperationalState::AddShotCommand ){
          /* need to ignore this -- */
          return false; 
      }

      /* don't want to loop, yo */
      if (new_state == JuraMachineOperationalState::Finishing && 
          (int) states[(int) JuraMachineStateIdentifier::OperationalState] == (int) JuraMachineOperationalState::Ready ){

          /* reset after at leaset 45 seconds  */
          if (
            (timenow - last_changed[(int) JuraMachineStateIdentifier::OperationalState] > (1000 * JURA_MACHINE_AWAIT_NEXT_INSTRUCTION_TIMEOUT_S)) && 
            (states[(int) JuraMachineStateIdentifier::ReadyStateDetail] != (int) JuraMachineReadyState::ReadyForNext)
            ){
            //ESP_LOGI(TAG,"----> Machine Operation:AWAITING NEXT INSTRUCTION");      
            states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::ReadyForNext;
            _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "AWAITING NEXT INSTRUCTION", (int) JuraMachineReadyState::ReadyForNext);
          
            /* reset machine dispense limits */
            resetDispenseLimits();
            return true; 
          }

          /* ------------------- below here we perform tasks automatically on idle ------------------- */
          
          if (/* if ready for at least x minutes */
            (states[(int) JuraMachineStateIdentifier::OperationalState] == (int) JuraMachineOperationalState::Ready) && 
            (timenow - last_changed[(int) JuraMachineStateIdentifier::OperationalState] > JURA_MACHINE_AUTOMATIC_MAINTENANCE_TIMEOUT_S * 1000) ){

            /* ready state has been milk rinse required for N minutes? */
            if (states[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended] == true){

              /* wait for five minutes, thereafter rinse milk */
              if (timenow - last_changed[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended] > JURA_MACHINE_AUTOMATIC_MILK_RINSE_TIMEOUT_S * 1000){
                _bridge->instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier::ConfirmDisplayPrompt);
                last_changed[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended] = timenow;
              }
            }

            /* ready state has been milk rinse required for N minutes? */
            if (states[(int) JuraMachineStateIdentifier::RinseBrewGroupRecommended] == true){

              /* wait for five minutes, thereafter rinse milk */
              if (timenow - last_changed[(int) JuraMachineStateIdentifier::RinseBrewGroupRecommended] > JURA_MACHINE_AUTOMATIC_BREW_GROUP_RINSE_TIMEOUT_S * 1000){
               _bridge->publishRinseRequest();
                last_changed[(int) JuraMachineStateIdentifier::RinseBrewGroupRecommended] = timenow;
              }
            }

            /* limits resets! */
            if (states[(int) JuraMachineStateIdentifier::WaterLimit] > 0 || states[(int) JuraMachineStateIdentifier::BrewLimit] > 0  || states[(int) JuraMachineStateIdentifier::MilkLimit] > 0 ){
              if (timenow - last_changed[(int) JuraMachineStateIdentifier::WaterLimit] > JURA_MACHINE_AUTOMATIC_DISPENSE_LIMIT_TIMEOUT_S * 1000){
                resetDispenseLimits();
              }else if (timenow - last_changed[(int) JuraMachineStateIdentifier::BrewLimit] > JURA_MACHINE_AUTOMATIC_DISPENSE_LIMIT_TIMEOUT_S * 1000){
                resetDispenseLimits();
              }else if (timenow - last_changed[(int) JuraMachineStateIdentifier::MilkLimit] > JURA_MACHINE_AUTOMATIC_DISPENSE_LIMIT_TIMEOUT_S * 1000){
                resetDispenseLimits();
              }
            }
          }
          return false;
      }

      //set last chagned
      last_changed[(int) JuraMachineStateIdentifier::OperationalState] = timenow;

      /* shift history array */
      pushElement(dispenseHistory, MAX_STATE_EVENT_HISTORY, states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume]);
      pushElement(flowRateHistory, MAX_STATE_EVENT_HISTORY, states[(int) JuraMachineStateIdentifier::WaterPumpFlowRate]);
      pushElement(durationHistory, MAX_STATE_EVENT_HISTORY, states[(int) JuraMachineStateIdentifier::LastDispenseDuration]);

      /* temperature histories */
      pushElement(temperatureAverageHistory, MAX_STATE_EVENT_HISTORY, states[(int) JuraMachineStateIdentifier::LastDispenseAvgTemperature]);
      pushElement(temperatureMinimumHistory, MAX_STATE_EVENT_HISTORY, states[(int) JuraMachineStateIdentifier::LastDispenseMinTemperature]);
      pushElement(temperatureMaximumHistory, MAX_STATE_EVENT_HISTORY, states[(int) JuraMachineStateIdentifier::LastDispenseMaxTemperature]);

      /* set new operational state; semaphore protected */
      xSemaphoreTake( xMachineReadyStateVariableSemaphore, portMAX_DELAY );
      states[(int) JuraMachineStateIdentifier::OperationalState] = (int) new_state;
      pushElement(operationalStateHistory, MAX_STATE_EVENT_HISTORY, states[(int) JuraMachineStateIdentifier::OperationalState]);
      xSemaphoreGive( xMachineReadyStateVariableSemaphore);
      return true; 
    }
  }
  
  /* */
  return false;
}

/***************************************************************************//**
 * Sampler from generator-esque input to determine whether flow state has changed
 *
 * @param[out] bool 
 *     
 * @param[in] int iterator 
 * @param[in] int duty_cycle
 ******************************************************************************/
 bool JuraMachine::flowStateHasChanged (int iterator, int duty_cycle){

  /* return on iterator */
  if (iterator % duty_cycle != 0 ){return false;}

  int _flow_meter_state = (_ic.returnValueOfServicePortResponseSubstringIndex(SUBSTR_INDEX_FLOW_METER_STATE, 1));

  /* create a merged state */
  int _flow_meter_rolling_state = (flow_meter_state * 2) + _flow_meter_state; /* 0, 1, 2, 3 */
  
  /* determine how many times */
  if (_flow_meter_rolling_state != flow_meter_rolling_state){
    flow_meter_stationary_counter = 0;
  }else{
    flow_meter_stationary_counter ++;
  }

  /* flow meter flowing */
  int _not_flowing = (flow_meter_stationary_counter > 6); 

  /* set previous states */
  flow_meter_state = _flow_meter_state; flow_meter_rolling_state = _flow_meter_rolling_state;

  /* full calcualted value of flow only outputs if true */
  if ( (!_not_flowing) != states[(int) JuraMachineStateIdentifier::FlowState]){
    states[(int) JuraMachineStateIdentifier::FlowState] = (! _not_flowing);
    return true; 
  }
  return false; 
}

/***************************************************************************//**
 * Convert a flow rate into an enum characterizion
 *
 * @param[out] JuraMachineOperationalStateFlowRateType 
 *     
 * @param[in] int inputFlowRate 
 ******************************************************************************/
 JuraMachineOperationalStateFlowRateType JuraMachine::characterizeFlowRate(int inputFlowRate){
  /* reminder: raw values are multiplied by 1000 to preserve precision */

  /* 4500 = espresso ..., 4900 = coffee */

  if ( inputFlowRate > 4900){         return JuraMachineOperationalStateFlowRateType::Unrestricted;
  } else if ( inputFlowRate > 3900){  return JuraMachineOperationalStateFlowRateType::Venturi;
  } else if ( inputFlowRate > 2900){  return JuraMachineOperationalStateFlowRateType::PressureBrew;
  } else {   return JuraMachineOperationalStateFlowRateType::Undeterminable;}
}

/***************************************************************************//**
 * Convert a temperature into an enum characterizion
 *
 * @param[out] JuraMachineOperationalStateTemperatureType 
 *     
 * @param[in] int inputTemperature 
 ******************************************************************************/ 
JuraMachineOperationalStateTemperatureType JuraMachine::characterizeTemperature(int inputTemperature){
  /* reminder: raw values are multiplied by 10x to preserve precision */
  inputTemperature /= 10;
  if ( inputTemperature > 140){         return JuraMachineOperationalStateTemperatureType::Steam;
  } else if ( inputTemperature > 120){  return JuraMachineOperationalStateTemperatureType::High;
  } else if ( inputTemperature > 100){  return JuraMachineOperationalStateTemperatureType::Normal;
  } else if ( inputTemperature > 50){  return JuraMachineOperationalStateTemperatureType::Low;
  } else {   return JuraMachineOperationalStateTemperatureType::Undeterminable;}
}

/***************************************************************************//**
 * Convert a dispense volume into an enum characterizion
 *
 * @param[out] JuraMachineOperationalStateDispenseQuantityType 
 *     
 * @param[in] int inputQuantity 
 ******************************************************************************/ 
JuraMachineOperationalStateDispenseQuantityType JuraMachine::characterizeDispenseQuantity(int inputQuantity){
  
  /* reminder: raw values are multiplied by coefficient to preserve precision */
  inputQuantity *= DISPENSED_ML_CALIBRATION_COEFFICIENT_DEFAULT; 
  if ( inputQuantity > 500){         return JuraMachineOperationalStateDispenseQuantityType::FilterFlush;
  } else if ( inputQuantity > 60){  return JuraMachineOperationalStateDispenseQuantityType::Beverage;
  } else if ( inputQuantity > 49){  return JuraMachineOperationalStateDispenseQuantityType::MilkRinse;
  } else if ( inputQuantity > 39){  return JuraMachineOperationalStateDispenseQuantityType::BrewGroupRinse;
  } else if ( inputQuantity == 0 ){  return JuraMachineOperationalStateDispenseQuantityType::NoDispense;
  } else {   return JuraMachineOperationalStateDispenseQuantityType::Undeterminable;}
}

/***************************************************************************//**
 * Set private vars 
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/ 
 void JuraMachine::startAddShotPreparation(){
    pushElement(operationalStateHistory, MAX_STATE_EVENT_HISTORY, (int) JuraMachineOperationalState::AddShotCommand);
    states[(int) JuraMachineStateIdentifier::OperationalState] = (int) JuraMachineOperationalState::AddShotCommand;
    states[(int) JuraMachineStateIdentifier::SystemIsReady] = false;
 }

/***************************************************************************//**
 * Parse through recent history to determine what preceded current ready state
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/ 
 void JuraMachine::determineReadyStateType(){

   /* operation mode occurrence */
  bool milkOperationOccurred = false;
  bool waitOperationOccurred = false;
  bool userWaitOperationOccurred = false;
  bool waterOperationOccurred = false;
  bool brewGroupOperationOccurred = false;
  bool grindOperationOccurred = false; 
  bool rinseOperationOccurred = false; 
  bool cleanOperationOccurred = false; 
  bool blockingErrorOccurred = false; 
  bool startupOccurred = false; 
  bool addShotCommand = false; 

  /* dispensers */
  int brewGroupDispense = 0;
  int milkDispense = 0;
  int waterDispense = 0;
  int rinseDispense = 0;

  int brewGroupDispenseIndex = 0;
  int milkDispenseIndex = 0;
  int waterDispenseIndex = 0;
  int rinseDispenseIndex = 0;

  /* updates of interest*/
  bool didUpdateNumEspresso = false;
  bool didUpdateNumCoffee = false;
  bool didUpdateNumCappuccino = false;
  bool didUpdateNumMacchiato = false;
  bool didUpdateNumPreground = false;
  bool didUpdateNumBrewGroupCleanSystemOperations = false;
  bool didUpdateNumSpentGrounds = false;
  bool didUpdateNumMilkFoamPreparations = false;
  bool didUpdateNumWaterPreparations = false;
  bool didUpdateNumGrinderOperations = false;
  bool didUpdateNumCleanMilkSystemOperations = false;

  /* correctable errors? */
  bool drainageTrayFullErrorFixed = false; 
  bool beanHopperEmptyErrorFixed = false; 
  bool beanHopperCoverOpenErrorFixed = false; 
  bool waterReservoirNeedsFillErrorFixed = false; 
  bool bypassDoserDoorOpenErrorFixed = false; 
  bool drainageTrayRemovedErrorFixed = false; 

   /* backtrack through history to set flags, stop at first "unkonwn" or "startup" */
  for (int op_i = 0 ; op_i < MAX_STATE_EVENT_HISTORY; op_i++){

    /* break as soon as we reach the end of our history */
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::Unknown )        {break;}
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::Starting)        {startupOccurred = true;}

    /* state occurrence booleans*/
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::AddShotCommand)             {addShotCommand = true;}
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::ProgramPause)               {waitOperationOccurred = true;}
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::AwaitRotaryInput)           {userWaitOperationOccurred = true;}
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::GrindOperation)             {grindOperationOccurred = true;}
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::Cleaning)                   {cleanOperationOccurred = true;}
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::BlockingError)              {blockingErrorOccurred = true;}

    /* state occurrence booleans & index finding for history analysis (read: find largest dispense event )*/
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::MilkOperation)   {
      milkOperationOccurred = true; 
      if (dispenseHistory[op_i] > milkDispense){
        milkDispense = dispenseHistory[op_i] * DISPENSED_ML_CALIBRATION_COEFFICIENT_MILK; 
        milkDispenseIndex = op_i;
      }
    }
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::WaterOperation)  {
      waterOperationOccurred = true; 
      if (dispenseHistory[op_i] > waterDispense){
        waterDispense = dispenseHistory[op_i] * DISPENSED_ML_CALIBRATION_COEFFICIENT_DEFAULT; 
        waterDispenseIndex = op_i;
      }
    }
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::BrewOperation)   {
      brewGroupOperationOccurred = true; 
      if (dispenseHistory[op_i] > brewGroupDispense){
        brewGroupDispense = dispenseHistory[op_i] * DISPENSED_ML_CALIBRATION_COEFFICIENT_ESPRESSO; 
        brewGroupDispenseIndex = op_i;
      }
    }
    if (operationalStateHistory[op_i] == (int) JuraMachineOperationalState::RinseOperation)  {
      rinseOperationOccurred = true; 
      if (dispenseHistory[op_i] > rinseDispense){
        rinseDispense = dispenseHistory[op_i] * DISPENSED_ML_CALIBRATION_COEFFICIENT_DEFAULT; 
        rinseDispenseIndex = op_i;
      }
    }
  }

  /* reprobe memory if we did one of the important things */
  if ( milkOperationOccurred ||
      waitOperationOccurred ||
      userWaitOperationOccurred ||
      waterOperationOccurred ||
      brewGroupOperationOccurred){

    /* refresh memory */
    handlePoll(-1);
  }

  /* determine what states have recently changed among important states to monitor */
  int stateAttributeArraySize = sizeof(JuraEntityConfigurations) / sizeof(JuraEntityConfigurations[0]) ; 

  /* iterate through all of the machine state attributes */
  int nowTime = millis();
  for (int i = 0; i < stateAttributeArraySize ; i++){
    int index = (int) JuraEntityConfigurations[i].state;
    if (last_changed[index] != 0 ){
      if(nowTime - last_changed[index] < 15000){ /* has this value changed in the last 15 seconds? */
        didUpdateNumEspresso = (index == (int) JuraMachineStateIdentifier::NumEspresso)? true : didUpdateNumEspresso;
        didUpdateNumCoffee = ( index == (int) JuraMachineStateIdentifier::NumCoffee)? true : didUpdateNumCoffee;
        didUpdateNumCappuccino = ( index == (int) JuraMachineStateIdentifier::NumCappuccino)? true : didUpdateNumCappuccino;
        didUpdateNumMacchiato = ( index == (int) JuraMachineStateIdentifier::NumMacchiato)? true : didUpdateNumMacchiato;
        didUpdateNumPreground = ( index == (int) JuraMachineStateIdentifier::NumPreground)? true : didUpdateNumPreground;
        didUpdateNumBrewGroupCleanSystemOperations = ( index == (int) JuraMachineStateIdentifier::NumBrewGroupCleanOperations)? true : didUpdateNumBrewGroupCleanSystemOperations;
        didUpdateNumSpentGrounds = ( index == (int) JuraMachineStateIdentifier::NumSpentGrounds)? true : didUpdateNumSpentGrounds;
        didUpdateNumMilkFoamPreparations = ( index == (int) JuraMachineStateIdentifier::NumMilkFoamPreparations)? true : didUpdateNumMilkFoamPreparations;
        didUpdateNumWaterPreparations = ( index == (int) JuraMachineStateIdentifier::NumWaterPreparations)? true : didUpdateNumWaterPreparations;
        didUpdateNumGrinderOperations = ( index == (int) JuraMachineStateIdentifier::NumGrinderOperations)? true : didUpdateNumGrinderOperations;
        didUpdateNumCleanMilkSystemOperations = ( index == (int) JuraMachineStateIdentifier::NumCleanMilkSystemOperations)? true : didUpdateNumCleanMilkSystemOperations;

        /* error fixes */
        drainageTrayFullErrorFixed = (index == (int) JuraMachineStateIdentifier::DrainageTrayFull) ? true : drainageTrayFullErrorFixed;
        beanHopperEmptyErrorFixed = (index == (int) JuraMachineStateIdentifier::BeanHopperEmpty) ? true  : beanHopperEmptyErrorFixed;
        beanHopperCoverOpenErrorFixed = (index == (int) JuraMachineStateIdentifier::BeanHopperCoverOpen) ? true  : beanHopperCoverOpenErrorFixed;
        waterReservoirNeedsFillErrorFixed = (index == (int) JuraMachineStateIdentifier::WaterReservoirNeedsFill) ? true  : waterReservoirNeedsFillErrorFixed;
        bypassDoserDoorOpenErrorFixed = (index == (int) JuraMachineStateIdentifier::BypassDoserCoverOpen) ? true  : bypassDoserDoorOpenErrorFixed;
        drainageTrayRemovedErrorFixed = (index == (int) JuraMachineStateIdentifier::DrainageTrayRemoved) ? true : drainageTrayRemovedErrorFixed;
      }
    }
  }

  /* flow characterizations: flow rate */
  JuraMachineOperationalStateFlowRateType brewFlowType = brewGroupOperationOccurred ? characterizeFlowRate(flowRateHistory[brewGroupDispenseIndex]) : JuraMachineOperationalStateFlowRateType::Undeterminable;
  JuraMachineOperationalStateFlowRateType milkFlowType = milkOperationOccurred ? characterizeFlowRate(flowRateHistory[milkDispenseIndex]) : JuraMachineOperationalStateFlowRateType::Undeterminable;
  JuraMachineOperationalStateFlowRateType rinseFlowType = rinseOperationOccurred ? characterizeFlowRate(flowRateHistory[rinseDispenseIndex]) : JuraMachineOperationalStateFlowRateType::Undeterminable;
  JuraMachineOperationalStateFlowRateType waterFlowType = waterOperationOccurred ? characterizeFlowRate(flowRateHistory[waterDispenseIndex]) : JuraMachineOperationalStateFlowRateType::Undeterminable;

  /* flow characterizations: temperature */
  JuraMachineOperationalStateTemperatureType brewTemperatureType = brewGroupOperationOccurred ? characterizeTemperature(temperatureAverageHistory[brewGroupDispenseIndex]) : JuraMachineOperationalStateTemperatureType::Undeterminable;
  JuraMachineOperationalStateTemperatureType milkTemperatureType = milkOperationOccurred ? characterizeTemperature(temperatureAverageHistory[milkDispenseIndex]) : JuraMachineOperationalStateTemperatureType::Undeterminable;
  JuraMachineOperationalStateTemperatureType rinseTemperatureType = rinseOperationOccurred ? characterizeTemperature(temperatureAverageHistory[rinseDispenseIndex]) : JuraMachineOperationalStateTemperatureType::Undeterminable;
  JuraMachineOperationalStateTemperatureType waterTemperatureType = waterOperationOccurred ? characterizeTemperature(temperatureAverageHistory[waterDispenseIndex]) : JuraMachineOperationalStateTemperatureType::Undeterminable;

  /* flow characterizations: dispense quantity */
  JuraMachineOperationalStateDispenseQuantityType brewDispenseQuantityType = brewGroupOperationOccurred ? characterizeDispenseQuantity(dispenseHistory[brewGroupDispenseIndex]) : JuraMachineOperationalStateDispenseQuantityType::Undeterminable;
  JuraMachineOperationalStateDispenseQuantityType milkDispenseQuantityType = milkOperationOccurred ? characterizeDispenseQuantity(dispenseHistory[milkDispenseIndex]) : JuraMachineOperationalStateDispenseQuantityType::Undeterminable;
  JuraMachineOperationalStateDispenseQuantityType rinseDispenseQuantityType = rinseOperationOccurred ? characterizeDispenseQuantity(dispenseHistory[rinseDispenseIndex]) : JuraMachineOperationalStateDispenseQuantityType::Undeterminable;
  JuraMachineOperationalStateDispenseQuantityType waterDispenseQuantityType = waterOperationOccurred ? characterizeDispenseQuantity(dispenseHistory[waterDispenseIndex]) : JuraMachineOperationalStateDispenseQuantityType::Undeterminable;

  /* determine event summary  */
  bool didSetDetail = false;

  /* completion state flags*/
  bool triggersMilkRinse = false; 
  bool doserIsEmpty = false;
  bool clearMilkRinse = false;
  bool clearMilkClean = false; 
  int drainage = 0;

  /* determine what happened the last dispense */
  if (blockingErrorOccurred){

    /* specific corrected error */
    if (drainageTrayFullErrorFixed || drainageTrayRemovedErrorFixed){
      //ESP_LOGI(TAG,"----> Machine Operation: REPLACED EMPTY TRAY AND KNOCKBOX");
      states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::ReplacedDrainageTrayAndEmptyKnockbox;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "REPLACED DRAINAGE TRAY AND EMPTIED KNOCKBOX", (int) JuraMachineReadyState::ReplacedDrainageTrayAndEmptyKnockbox); didSetDetail = true;
      
      /* reset the drainage */
      states[(int) JuraMachineStateIdentifier::DrainageSinceLastTrayEmpty] = 0;
      if (_bridge->machineStateChanged(JuraMachineStateIdentifier::DrainageSinceLastTrayEmpty ,states[(int) JuraMachineStateIdentifier::DrainageSinceLastTrayEmpty])){
        /* update tray percentage too! */
        int capacity = 0;

        states[(int) JuraMachineStateIdentifier::DrainageTrayLevel] = capacity;
        _bridge->machineStateChanged(JuraMachineStateIdentifier::DrainageTrayLevel, states[(int) JuraMachineStateIdentifier::DrainageTrayLevel]);

        /* give a small buffer for filling here */
        states[(int) JuraMachineStateIdentifier::DrainageTrayFull] = false;
        _bridge->machineStateChanged(JuraMachineStateIdentifier::DrainageTrayFull, states[(int) JuraMachineStateIdentifier::DrainageTrayFull]);
      }
    }
    if (beanHopperEmptyErrorFixed || beanHopperCoverOpenErrorFixed){
      //ESP_LOGI(TAG,"----> Machine Operation: FILLED BEANS");
      states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::FilledBeans;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "FILLED BEANS", (int) JuraMachineReadyState::FilledBeans); didSetDetail = true;
    }
    if (waterReservoirNeedsFillErrorFixed){
      //ESP_LOGI(TAG,"----> Machine Operation: FILLED WATER");
      states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::FilledWater ;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "FILLED WATER", (int) JuraMachineReadyState::FilledWater); didSetDetail = true;
    }
    if (bypassDoserDoorOpenErrorFixed){
      //ESP_LOGI(TAG,"----> Machine Operation: CLOSED BYPASS DOSER DOOR");
      states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::ClosedBypassDoserDoor ;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "CLOSED BYPASS DOSER DOOR", (int) JuraMachineReadyState::ClosedBypassDoserDoor); didSetDetail = true;
    }

  } else if ( /* bean hopper empty error */
    grindOperationOccurred && 
    (brewDispenseQuantityType == JuraMachineOperationalStateDispenseQuantityType::NoDispense) && 
    brewGroupOperationOccurred){

    /* bean hopper empty */
    doserIsEmpty = true; 
    states[(int) JuraMachineStateIdentifier::BeanHopperEmpty] = true; 
    _bridge->machineStateChanged(JuraMachineStateIdentifier::BeanHopperEmpty, states[(int) JuraMachineStateIdentifier::BeanHopperEmpty]);
    return;

  } else if (didUpdateNumEspresso){ 
    /* note: add temperature-based overextraction flag */
    //ESP_LOGI(TAG,"----> Machine Operation: ESPRESSO READY");  
    doserIsEmpty = true; 

    /* quantity */
    states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::EspressoReady;
    if (addShotCommand){
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "PREPARING NEXT SHOT", (int) JuraMachineReadyState::EspressoReady); didSetDetail = true;
    }else if (brewGroupDispense < 30){
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "SHORT ESPRESSO READY", (int) JuraMachineReadyState::EspressoReady); didSetDetail = true;
    } else if (brewGroupDispense < 40){
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "ESPRESSO READY", (int) JuraMachineReadyState::EspressoReady); didSetDetail = true;
    } else {
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "LONG ESPRESSO READY", (int) JuraMachineReadyState::EspressoReady); didSetDetail = true;
    }
    

    /* default condensation drainage*/
    drainage += DEFAULT_DRAINAGE_ML;

  } else if (didUpdateNumCoffee){ 
    /* note: add temperature-based overextraction flag */
    //ESP_LOGI(TAG,"----> Machine Operation: COFFEE READY"); 
    doserIsEmpty = true; 
    states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::CoffeeReady ;

    if (addShotCommand){
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "PREPARING NEXT SHOT", (int) JuraMachineReadyState::EspressoReady); didSetDetail = true;
    }else if (brewGroupDispense < 200 ){
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "SMALL COFFEE READY", (int) JuraMachineReadyState::CoffeeReady); didSetDetail = true;
    } else if (brewGroupDispense < 275){
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "COFFEE READY", (int) JuraMachineReadyState::CoffeeReady); didSetDetail = true;
    } else {
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "LARGE COFFEE READY", (int) JuraMachineReadyState::CoffeeReady); didSetDetail = true;
    }

    /* default */
    drainage += DEFAULT_DRAINAGE_ML;

  } else if (didUpdateNumCappuccino){ 
    /* note: add temperature-based overextraction flag */
    //ESP_LOGI(TAG,"----> Machine Operation: CAPPUCCINO READY"); 
    doserIsEmpty = true; 
    triggersMilkRinse = true;
    states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::CappuccinoReady ;

    if (addShotCommand){
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "PREPARING NEXT SHOT", (int) JuraMachineReadyState::CappuccinoReady); didSetDetail = true;
    }else{
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "CAPPUCCINO READY", (int) JuraMachineReadyState::CappuccinoReady); didSetDetail = true;
    }
    
    drainage += DEFAULT_DRAINAGE_ML;

  } else if (didUpdateNumMacchiato){ 
    /* note: add temperature-based overextraction flag */
    //ESP_LOGI(TAG,"----> Machine Operation: MACCIATTO READY"); 
    doserIsEmpty = true; 
    triggersMilkRinse = true;
    states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::MacciattoReady ;
    if (addShotCommand){
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "PREPARING NEXT SHOT", (int) JuraMachineReadyState::MacciattoReady); didSetDetail = true;
    }else{
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "MACCIATTO READY", (int) JuraMachineReadyState::MacciattoReady); didSetDetail = true;
    }
    drainage += DEFAULT_DRAINAGE_ML;

  } else if (didUpdateNumMilkFoamPreparations){
    //ESP_LOGI(TAG,"----> Machine Operation: MILK FOAM READY");  
    doserIsEmpty = true; 
    triggersMilkRinse = true;
    states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::MilkFoamReady ;
    if (addShotCommand){
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "PREPARING NEXT SHOT", (int) JuraMachineReadyState::MilkFoamReady); didSetDetail = true;
    }else if (brewGroupDispense < 60){
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "SHORT MILK FOAM READY", (int) JuraMachineReadyState::MilkFoamReady); didSetDetail = true;
    } else if (brewGroupDispense < 250){
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "MILK FOAM READY", (int) JuraMachineReadyState::MilkFoamReady); didSetDetail = true;
    } else {
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "MILK FOAM READY", (int) JuraMachineReadyState::MilkFoamReady); didSetDetail = true;
    }

    drainage += DEFAULT_DRAINAGE_ML;

  } else if (didUpdateNumWaterPreparations){
    //ESP_LOGI(TAG,"----> Machine Operation: HOT WATER READY"); 
    states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::HotWaterReady ;
    if (addShotCommand){
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "PREPARING NEXT SHOT", (int) JuraMachineReadyState::HotWaterReady); didSetDetail = true;
    }else{
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "HOT WATER READY", (int) JuraMachineReadyState::HotWaterReady); didSetDetail = true;
    }
    drainage += DEFAULT_DRAINAGE_ML;

  } else if (didUpdateNumBrewGroupCleanSystemOperations){
    /* brew group clean operation performed */
    //ESP_LOGI(TAG,"----> Machine Operation: CLEAN BREW GROUP COMPLETE"); 
    doserIsEmpty = true; 
    states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::CleanBrewGroupComplete ;
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "CLEAN BREW GROUP COMPLETE", (int) JuraMachineReadyState::CleanBrewGroupComplete); didSetDetail = true;
    drainage += JURA_MACHINE_WATER_RESERVOIR_CAPACITY_ML; /* max it out */

  } else if (rinseOperationOccurred && brewGroupOperationOccurred && !grindOperationOccurred) { 
    /* rinse operation  */
    //ESP_LOGI(TAG,"----> Machine Operation: RINSE BREW GROUP COMPLETE");  
    doserIsEmpty = true; 
    states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::RinseBrewGroupComplete ;
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "RINSE BREW GROUP COMPLETE", (int) JuraMachineReadyState::RinseBrewGroupComplete); didSetDetail = true;
    drainage += RINSE_BREW_GROUP_DRAINAGE;

  } else if ( /* nothing related to coffee preparation, only relating to milk preparation */
    !brewGroupOperationOccurred && 
    !grindOperationOccurred && 
    milkOperationOccurred ){
      
    /* milk clean operation here! */
    if (userWaitOperationOccurred || didUpdateNumCleanMilkSystemOperations){
      //ESP_LOGI(TAG,"----> Machine Operation: CLEAN MILK SYSTEM COMPLETE");
      states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::CleanMilkSystemComplete ;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "CLEAN MILK SYSTEM COMPLETE", (int) JuraMachineReadyState::CleanMilkSystemComplete); didSetDetail = true;
      clearMilkClean = true; 
      clearMilkRinse = true; 

    }else{
      //ESP_LOGI(TAG,"----> Machine Operation: RINSE MILK SYSTEM COMPLETE");
      states[(int) JuraMachineStateIdentifier::ReadyStateDetail] = (int) JuraMachineReadyState::RinseMilkSystemComplete ;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "RINSE MILK SYSTEM COMPLETE", (int) JuraMachineReadyState::RinseMilkSystemComplete); didSetDetail = true;
      clearMilkRinse = true;
      drainage += RINSE_MILK_SYSTEM_DRAINAGE;
    }
  } 

  /* ------------------------------------------------------------------------------------------ */
  //
  //          SPECIAL HANDLING AND FLAG HANDLING 
  //
  /* ------------------------------------------------------------------------------------------ */

  if (drainage > 0){
    states[(int) JuraMachineStateIdentifier::DrainageSinceLastTrayEmpty] += drainage;
    if (_bridge->machineStateChanged(JuraMachineStateIdentifier::DrainageSinceLastTrayEmpty ,states[(int) JuraMachineStateIdentifier::DrainageSinceLastTrayEmpty])){
      /* update tray percentage too! */
      int capacity = 100 * states[(int) JuraMachineStateIdentifier::DrainageSinceLastTrayEmpty] / JURA_MACHINE_DRIP_TRAY_CAPACITY_ML;
      capacity = capacity > 100 ? 100 : capacity < 0 ? 0 : capacity; 

      states[(int) JuraMachineStateIdentifier::DrainageTrayLevel] = capacity;
      _bridge->machineStateChanged(JuraMachineStateIdentifier::DrainageTrayLevel, states[(int) JuraMachineStateIdentifier::DrainageTrayLevel]);

      /* give a small buffer for filling here */
      if (capacity >= JURA_MACHINE_DRIP_TRAY_CAPACITY_ML * 0.5 ){
         states[(int) JuraMachineStateIdentifier::DrainageTrayFull] = true;
        _bridge->machineStateChanged(JuraMachineStateIdentifier::DrainageTrayFull, states[(int) JuraMachineStateIdentifier::DrainageTrayFull]);
      }
    }
  }

  /* reset dosing, if a dose has been dosed */
  if (doserIsEmpty){
    states[(int) JuraMachineStateIdentifier::HasDoes] = false;
    _bridge->machineStateChanged(JuraMachineStateIdentifier::HasDoes ,false);
  }

  /* if we need to rinse the milk system, raise the flags! */
  if (triggersMilkRinse){
    /* milk clean & rinse flags */
    states[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended] = true;  
    states[(int) JuraMachineStateIdentifier::CleanMilkSystemRecommended] = true;
    
    last_changed[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended] = millis();
    last_changed[(int) JuraMachineStateIdentifier::CleanMilkSystemRecommended] = millis();

    _bridge->machineStateChanged(JuraMachineStateIdentifier::RinseMilkSystemRecommended, states[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended]);
    _bridge->machineStateChanged(JuraMachineStateIdentifier::CleanMilkSystemRecommended, states[(int) JuraMachineStateIdentifier::CleanMilkSystemRecommended]);
    
  }else{

    /* clear these flags when completed */
    if (clearMilkRinse){
      states[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended] = false;  
      last_changed[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended] = millis();
      _bridge->machineStateChanged(JuraMachineStateIdentifier::RinseMilkSystemRecommended, states[(int) JuraMachineStateIdentifier::RinseMilkSystemRecommended]);
    }
    if (clearMilkClean){
      states[(int) JuraMachineStateIdentifier::CleanMilkSystemRecommended] = false;
      last_changed[(int) JuraMachineStateIdentifier::CleanMilkSystemRecommended] = millis();
      _bridge->machineStateChanged(JuraMachineStateIdentifier::CleanMilkSystemRecommended, states[(int) JuraMachineStateIdentifier::CleanMilkSystemRecommended]);
    }      
  }

  /* reset history array */
  for (int op_i = 0 ; op_i < MAX_STATE_EVENT_HISTORY; op_i++){
      operationalStateHistory[op_i] = (int) JuraMachineOperationalState::Unknown;
      dispenseHistory[op_i] = 0;
      flowRateHistory[op_i] = 0;
      durationHistory[op_i] = 0;

      /* seprate temperature data items */
      temperatureAverageHistory[op_i] = 0;
      temperatureMinimumHistory[op_i] = 0;
      temperatureMaximumHistory[op_i] = 0;
  }

  if (states[(int) JuraMachineStateIdentifier::ReadyStateDetail] != (int) JuraMachineReadyState::ExecutingOperation || startupOccurred){
    /* this is the only place we set ready! */
    states[(int) JuraMachineStateIdentifier::OperationalState] = (int) JuraMachineOperationalState::Ready;
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "READY", states[(int) JuraMachineStateIdentifier::OperationalState]);
  }else{
    return;
  }
}

/***************************************************************************//**
 * Reset global dispense limits to machine defaults
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
void JuraMachine::resetDispenseLimits(){
  setDispenseLimit(0, JuraMachineDispenseLimitType::Brew);
  setDispenseLimit(0, JuraMachineDispenseLimitType::Milk);
  setDispenseLimit(0, JuraMachineDispenseLimitType::Water);
}

/***************************************************************************//**
 * Set global dispense limits based on input
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
void JuraMachine::setDispenseLimit(int dispenseMax, JuraMachineDispenseLimitType limitType){
  xSemaphoreTake( xDispenseLimitSemaphore, portMAX_DELAY );
  /* must be greater than zero; must be greater than 15ml and less than 300 ml */
  switch (limitType ){
    case JuraMachineDispenseLimitType::Brew:
      states[(int)JuraMachineStateIdentifier::BrewLimit] = (dispenseMax >= 15 && dispenseMax < 65) ? dispenseMax : 0 ;
      last_changed[(int)JuraMachineStateIdentifier::BrewLimit] = millis();
      break; 
    case JuraMachineDispenseLimitType::Milk:
      states[(int)JuraMachineStateIdentifier::MilkLimit] = (dispenseMax >= 30 && dispenseMax < 300) ? dispenseMax : 0 ;
      last_changed[(int)JuraMachineStateIdentifier::MilkLimit] = millis();
      break; 
    case JuraMachineDispenseLimitType::Water:    
      states[(int)JuraMachineStateIdentifier::WaterLimit] = (dispenseMax >= 25 && dispenseMax < 300) ? dispenseMax : 0 ;
      last_changed[(int)JuraMachineStateIdentifier::WaterLimit] = millis();
      break;
  }
  xSemaphoreGive( xDispenseLimitSemaphore);
}

/***************************************************************************//**
 * Clear specific limit type
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
 void JuraMachine::clearDispenseLimit(JuraMachineDispenseLimitType limitType){
  xSemaphoreTake( xDispenseLimitSemaphore, portMAX_DELAY );
  switch (limitType) {
    case JuraMachineDispenseLimitType::Brew:
      states[(int)JuraMachineStateIdentifier::BrewLimit] = 0 ;
      last_changed[(int)JuraMachineStateIdentifier::BrewLimit] = millis();
      break; 
    case JuraMachineDispenseLimitType::Milk:
      states[(int)JuraMachineStateIdentifier::MilkLimit] = 0 ;
      last_changed[(int)JuraMachineStateIdentifier::MilkLimit] = millis();
      break; 
    case JuraMachineDispenseLimitType::Water:    
      states[(int)JuraMachineStateIdentifier::WaterLimit] = 0 ;
      last_changed[(int)JuraMachineStateIdentifier::WaterLimit] = millis();
      break;
  }
  xSemaphoreGive( xDispenseLimitSemaphore);
}


/***************************************************************************//**
 * Determine meta states related to ceramic valve position; decode ceramic valve
 * position. 
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
 void JuraMachine::handleCeramicValve(){
  /* value parsing */
  int position = states[(int) JuraMachineStateIdentifier::CeramicValvePosition];
  int comparator = position;
  bool steam_mode = (comparator & (int) 4) != 0;
  bool water_mode = !steam_mode;
  
  /* numerical comparisons made easier */      
  if (comparator > 4) {comparator = comparator - 4;}

  bool isCondenserPosition = false;
  bool isHotWaterCircuit= false;
  bool isPressurizingPosition= false;
  bool isBrewingPosition= false;
  bool isMoving= false;
  bool isSteamPosition= false;
  bool isVenturiPosition= false;
  bool isPressureReliefPosition = false;

  if (steam_mode) {
    if (comparator == 0) {
      isPressureReliefPosition = true; 
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "PRESSURE RELIEF", position);    
    }else if (comparator == 1) {
      isSteamPosition= true;  
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "STEAM", position);       

    }else if (comparator == 2) {
      isVenturiPosition= true;  
      if (_bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "VENTURI", position)){
      };

    }else if (comparator == 3) {
      isMoving= true;    
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "MOVING", position);
    }else{
      isPressurizingPosition= true;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "SYSTEM FILL", position);
    }
  } else {
    
    if (comparator == 0){
      isMoving= true;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "MOVING", position);
    }else if (comparator == 1){
      isHotWaterCircuit= true;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "HOT WATER", position);
    }else if (comparator == 2){
      isPressurizingPosition= true;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "SYSTEM FILL", position);
    }else if (comparator == 3){
      isBrewingPosition= true;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "BREWING", position);
    }else{
      isCondenserPosition = true;
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::CeramicValvePosition, "CONDENSER", position);
    }
  }

  /* water system modes*/
  _bridge->machineStateChanged(JuraMachineStateIdentifier::SystemWaterMode, water_mode);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::SystemSteamMode, steam_mode);

  /* save states */
  states[(int) JuraMachineStateIdentifier::CeramicValveCondenserPosition] = isCondenserPosition;
  states[(int) JuraMachineStateIdentifier::CeramicValveHotWaterPosition] = isHotWaterCircuit;
  states[(int) JuraMachineStateIdentifier::CeramicValvePressurizingPosition] = isPressurizingPosition;
  states[(int) JuraMachineStateIdentifier::CeramicValveBrewingPosition] =  isBrewingPosition;
  states[(int) JuraMachineStateIdentifier::CeramicValveSteamPosition] =  isSteamPosition;
  states[(int) JuraMachineStateIdentifier::CeramicValveVenturiPosition] =  isVenturiPosition;
  states[(int) JuraMachineStateIdentifier::CeramicValvePressureReliefPosition] = isPressureReliefPosition;
  states[(int) JuraMachineStateIdentifier::SystemWaterMode] = water_mode;
  states[(int) JuraMachineStateIdentifier::SystemSteamMode] = steam_mode;

  /* ceramic valve states */
  _bridge->machineStateChanged(JuraMachineStateIdentifier::CeramicValveCondenserPosition, isCondenserPosition);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::CeramicValveHotWaterPosition, isHotWaterCircuit);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::CeramicValvePressurizingPosition, isPressurizingPosition);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::CeramicValveBrewingPosition, isBrewingPosition);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::CeramicValveSteamPosition, isSteamPosition);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::CeramicValveVenturiPosition, isVenturiPosition);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::CeramicValvePressureReliefPosition, isPressureReliefPosition);
}

/***************************************************************************//**
 * Determine meta states related to brew group position; decode brew group
 * position. 
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
void JuraMachine::handleBrewGroupOutput(){
  /* mask out the output valve states; TODO: actually decode the values here - seemingly different for different brew characteristics?  */
  int brew_group_drive_shaft_position = states[(int) JuraMachineStateIdentifier::BrewGroupOutputStatus] & 63;

  /* interpret brew group drive shaft position */
  /*
      greater than 55 is puck ejection, but sampling rates are too low to catch
      lower than 10 is tamping, but again sampling rates are too low to catch; broad states only
  */

  bool isWaterRinsing = (brew_group_drive_shaft_position == 57) || (brew_group_drive_shaft_position == 58)  || (brew_group_drive_shaft_position == 50) || (brew_group_drive_shaft_position == 42);
  bool isOpen = false;
  bool isReady = false; //ESP_LOGI(TAG, "Drive Shaft Position: %i",brew_group_drive_shaft_position);
  bool isLocked = false;  

  if (brew_group_drive_shaft_position >= 31){
    isOpen = true; 
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::BrewGroupPosition, "OPEN", 31);

  }else if (brew_group_drive_shaft_position == 29 || brew_group_drive_shaft_position == 28 || brew_group_drive_shaft_position == 30){
    isReady = true; 
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::BrewGroupPosition, "READY FOR DOSE", 29);

  }else if (brew_group_drive_shaft_position <= 27){
    isLocked = true; 
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::BrewGroupPosition, "LOCKED", 10);
  }

  /* set values */
  _bridge->machineStateChanged(JuraMachineStateIdentifier::BrewGroupIsRinsing, isWaterRinsing);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::BrewGroupIsOpen, isOpen);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::BrewGroupIsReady, isReady);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::BrewGroupIsLocked, isLocked);

  /* update local states */
  states[(int) JuraMachineStateIdentifier::BrewGroupIsRinsing] = isWaterRinsing;
  states[(int) JuraMachineStateIdentifier::BrewGroupIsOpen] = isOpen;
  states[(int) JuraMachineStateIdentifier::BrewGroupIsReady] = isReady;
  states[(int) JuraMachineStateIdentifier::BrewGroupIsLocked] = isLocked;

  /* output valve */ 
  int _output_valve_postition = states[(int) JuraMachineStateIdentifier::BrewGroupOutputStatus] >> 7;

  bool isOutputValveBrewPosition = ((int)(_output_valve_postition) & (int) 4) != 0; //brew position
  bool isOutputValveFlushPosition = ((int)(_output_valve_postition) & (int) 2) != 0; //flush position? 
  bool isOutputValveDrainPosition = ((int)(_output_valve_postition) & (int) 1) != 0; //drain position? 
  bool isOutputValveBlockPosition = !isOutputValveBrewPosition && !isOutputValveDrainPosition && !isOutputValveFlushPosition;

  /* save state */
  states[(int) JuraMachineStateIdentifier::OutputValveIsBrewing] = isOutputValveBrewPosition;
  states[(int) JuraMachineStateIdentifier::OutputValveIsFlushing] = isOutputValveFlushPosition;
  states[(int) JuraMachineStateIdentifier::OutputValveIsDraining] = isOutputValveDrainPosition;
  states[(int) JuraMachineStateIdentifier::OutputValveIsBlocking] = isOutputValveBlockPosition;

  /* notify bridge of multiple values derived from the single valve position state */
  _bridge->machineStateChanged(JuraMachineStateIdentifier::OutputValveIsBrewing, isOutputValveBrewPosition);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::OutputValveIsFlushing, isOutputValveFlushPosition);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::OutputValveIsDraining, isOutputValveDrainPosition);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::OutputValveIsBlocking, isOutputValveBlockPosition);

  /* single state  */
  if (isOutputValveBrewPosition){
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OutputValvePosition, "BREWING", _output_valve_postition);
  }else if(isOutputValveFlushPosition){
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OutputValvePosition, "FLUSHING", _output_valve_postition);
  }else if(isOutputValveDrainPosition){
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OutputValvePosition, "DRAINING", _output_valve_postition);
  }else if(isOutputValveBlockPosition){
    _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OutputValvePosition, "BLOCKING", _output_valve_postition);
  }
}

/***************************************************************************//**
 * Determine meta states related to thermoblock
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
void JuraMachine::handleThermoblockTemperature(int _temp){
  /* modes */
  bool isColdMode = false;
  bool isLowMode= false;
  bool isHighMode= false;
  bool isOvertemp= false;
  bool isSanitationLevel = false;

  /* does temperature fall within flagged thresholds */
  int _thermoblock_status = 5;
  if (_temp <= COLD_MODE_THRESHOLD){ isColdMode = true; _thermoblock_status = 0;
  }else if (_temp <= LOW_MODE_THRESHOLD){ isLowMode = true;  _thermoblock_status = 1;
  }else if (_temp >= HIGH_MODE_THRESHOLD){isHighMode = true; _thermoblock_status = 2;}

  /* flag overtemp warning separately */
  if (_temp > OVERTEMP_THRESHOLD){isOvertemp = true; _thermoblock_status = 3;}
  if (_temp > SANITATION_TEMP_THRESHOLD){isSanitationLevel = true; _thermoblock_status = 4;}

  if(_thermoblock_status != thermoblock_status){
    switch(_thermoblock_status){
      case 0:
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ThermoblockStatus, "COLD", _thermoblock_status);
        break;
      case 1:
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ThermoblockStatus, "LOW", _thermoblock_status);
        break;
      case 2:
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ThermoblockStatus, "HIGH", _thermoblock_status);
        break;
      case 3:
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ThermoblockStatus, "ThermoblockOvertemperature", _thermoblock_status);
        break;
      case 4:
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ThermoblockStatus, "SANITIZE", _thermoblock_status);
        break;
      case 5:
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ThermoblockStatus, "NORMAL", _thermoblock_status);
        break;
      default: 
        break;
    }
    thermoblock_status = _thermoblock_status;
  }

  /* publish states to bridge */
  _bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockColdMode, isColdMode);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockLowMode, isLowMode);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockHighMode, isHighMode);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockOvertemperature, isOvertemp);
  _bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockSanitationTemperature, isSanitationLevel);

  states[(int) JuraMachineStateIdentifier::ThermoblockColdMode] = isColdMode;
  states[(int) JuraMachineStateIdentifier::ThermoblockLowMode] = isLowMode;
  states[(int) JuraMachineStateIdentifier::ThermoblockHighMode] = isHighMode;
  states[(int) JuraMachineStateIdentifier::ThermoblockOvertemperature] = isOvertemp;
  states[(int) JuraMachineStateIdentifier::ThermoblockSanitationTemperature] = isSanitationLevel;
}

/***************************************************************************//**
 * Determine meta states related to bean hopper cover (beans, volume, etc.)
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
void JuraMachine::handleBeanHopperCoverOpen(){
  /* recent change, test current state as open */
  int last_changed_ms = last_changed[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen ];

  /* bean hopper */
  if (( states[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen] == false) ){
    states[(int) JuraMachineStateIdentifier::BeanHopperEmpty] = false; 
    _bridge->machineStateChanged(JuraMachineStateIdentifier::BeanHopperEmpty, states[(int) JuraMachineStateIdentifier::BeanHopperEmpty]);

    /* has enough time elapsed? */
    if (millis() -  last_changed[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen] > JURA_MACHINE_BEANS_REFILLED_TIMEOUT){
      states[(int) JuraMachineStateIdentifier::SpentBeansByWeight] = 0;
      _bridge->machineStateChanged(JuraMachineStateIdentifier::SpentBeansByWeight, states[(int) JuraMachineStateIdentifier::SpentBeansByWeight]);
      _bridge->machineStateChanged(JuraMachineStateIdentifier::BeanHopperLevel, 100.0);
    }
  }
}

/***************************************************************************//**
 * Determine meta and calculated states related to last dispense volue
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
 void JuraMachine::handleLastDispenseChange(int prior_dispense){
  
  //ESP_LOGI(TAG,"Raw Uncalibrated Dispense Measurement: %i", states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume]);
  float calibrationCoefficient = 1.0;
  JuraMachineDispenseLimitType dispenseLimitType = JuraMachineDispenseLimitType::None;

  /* starting new? */
  if (states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] - prior_dispense < 0){
    /* fill operational state history */
    for (int i = 0; i < MAX_DISPENSE_SAMPLES ; i++) {
      /* reset this during the dispense checker so that we track temperature during a dispense */
      flowRateSamples[i]=0;
      temperatureSamples[i]=0;
      timestampSamples[i]=0;
    }

    /* reset with event state history size; these indexes should match with the dispense & flow max*/
    for (int i = 0; i < MAX_STATE_EVENT_HISTORY ; i++) {
      temperatureAverageHistory[i] = 0;
      temperatureMaximumHistory[i] = 0;
      temperatureMinimumHistory[i] = 0;
    }

    /* start time of the new dispense*/
    timestampSamples[0] = millis();

  } else {
    /* timestamp */
    float time_delta = (millis() - last_changed[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume]);

    /* calculate */
    float ml_per_min = ((states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] - prior_dispense) * 100000 / time_delta)  * DISPENSED_ML_CALIBRATION_COEFFICIENT_DEFAULT ;
    ml_per_min = (ml_per_min < 0) ? 0 : ml_per_min;

    /* calculate new flow rate index  */
    pushElement(flowRateSamples, MAX_DISPENSE_SAMPLES, ml_per_min);
    pushElement(temperatureSamples, MAX_DISPENSE_SAMPLES, (int) states[(int) JuraMachineStateIdentifier::ThermoblockTemperature]);
    pushElement(timestampSamples, MAX_DISPENSE_SAMPLES, millis());

    /* statistics during this dispense action */
    int maxTemp=0;
    int minTemp = INT_MAX;
    int nonzero_temperature_sum=0;
    int nonzero_dispense_sum = 0;
    int nonzero_dispense_count = 0;
    int nonzero_temp_count = 0;
    int gross_temperature_trend = 0;
    int last_temperature = 0;
    int start_time = INT_MAX; 

    /* iterat through samples captured thus far to capture metrics */
    /* REMEMBER THAT AS THIS PROFRESSES FORWARD, WE GO BACK IN TIME -- TEMPERATURE TREND IS OPPOSITE OF INTUITION*/
    for (int i = 0; i < MAX_DISPENSE_SAMPLES ; i++) {
      if (timestampSamples[i]> 0){
        start_time = timestampSamples[i] < start_time ? timestampSamples[i] : start_time; 
      }

      if (temperatureSamples[i] > 0){
        maxTemp = temperatureSamples[i] > maxTemp ? temperatureSamples[i] : maxTemp;
        minTemp = temperatureSamples[i] < minTemp ? temperatureSamples[i] : minTemp;
        nonzero_temperature_sum += temperatureSamples[i];
        nonzero_temp_count++;

        /* determine tempertaure trend */
        if (last_temperature > 0) {
          
          /* determine gross temperature trend during this dispense */
          if (temperatureSamples[i] > last_temperature){
            gross_temperature_trend--;
          
          }else if (temperatureSamples[i] < last_temperature){
            gross_temperature_trend++;
          }
        }

        /* record trailing temperature */
        last_temperature = temperatureSamples[i];
      }
      
      if (flowRateSamples[i] != 0){
        nonzero_dispense_sum += flowRateSamples[i];
        nonzero_dispense_count++;
      };
    }

    /* div by zero */
    if (nonzero_dispense_count > 0 && nonzero_temp_count > 0){

      int flow_rate_average = nonzero_dispense_sum / nonzero_dispense_count;
      int temperature_average = nonzero_temperature_sum / nonzero_temp_count;
      int current_millis = millis();

      /* set appropriate limits heree --- REMEMBER THAT EACH ARE MULTIPLIED BY 10  */
      if ((flow_rate_average >=0 && flow_rate_average <= 10000) && (temperature_average > 500 && temperature_average < 1800)){
        /* flow rate only if change */
        states[(int) JuraMachineStateIdentifier::WaterPumpFlowRate] = (int) flow_rate_average * 10;
        _bridge->machineStateChanged(
          JuraMachineStateIdentifier::WaterPumpFlowRate, 
          (int) flow_rate_average * 10 
        );

        /* update tempreature values for */
        states[(int) JuraMachineStateIdentifier::LastDispenseAvgTemperature] = (int) temperature_average;
        _bridge->machineStateChanged(
          JuraMachineStateIdentifier::LastDispenseAvgTemperature, 
          (int) temperature_average  / 10 
        );

        states[(int) JuraMachineStateIdentifier::LastDispenseMaxTemperature] = (int) maxTemp;
        _bridge->machineStateChanged(
          JuraMachineStateIdentifier::LastDispenseMaxTemperature, 
          (int) maxTemp  / 10 
        );

        states[(int) JuraMachineStateIdentifier::LastDispenseMinTemperature] = (int) minTemp;
        _bridge->machineStateChanged(
          JuraMachineStateIdentifier::LastDispenseMinTemperature, 
          (int) minTemp / 10 
        );

        /* characterize temperature trend of this dispense event */
        states[(int) JuraMachineStateIdentifier::LastDispenseGrossTemperatureTrend] = (int) gross_temperature_trend;
        if (gross_temperature_trend > 5){
          _bridge->machineStateStringChanged(
              JuraMachineStateIdentifier::LastDispenseGrossTemperatureTrend, 
              "INCREASING",
              (int) gross_temperature_trend
          );
        } else if (gross_temperature_trend < -5){
          _bridge->machineStateStringChanged(
              JuraMachineStateIdentifier::LastDispenseGrossTemperatureTrend, 
              "DECREASING",
              (int) gross_temperature_trend
          );
        }else{
           _bridge->machineStateStringChanged(
              JuraMachineStateIdentifier::LastDispenseGrossTemperatureTrend, 
              "NEUTRAL",
              (int) gross_temperature_trend
          );
        }

        /* ====================== dispense type ====================== */
        if(states[(int) JuraMachineStateIdentifier::SystemSteamMode] == true){
          calibrationCoefficient = DISPENSED_ML_CALIBRATION_COEFFICIENT_MILK;
          dispenseLimitType = JuraMachineDispenseLimitType::Milk;
          _bridge->machineStateStringChanged(
              JuraMachineStateIdentifier::LastDispenseType, 
              "MILK",
              (int) 0
          );

          /* set milk dispense */
          if (states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] * DISPENSED_ML_CALIBRATION_COEFFICIENT_MILK > 10){
            states[(int) JuraMachineStateIdentifier::LastMilkDispenseVolume] = states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume];
            _bridge->machineStateChanged(
              JuraMachineStateIdentifier::LastMilkDispenseVolume, 
              (int) states[(int) JuraMachineStateIdentifier::LastMilkDispenseVolume] * DISPENSED_ML_CALIBRATION_COEFFICIENT_MILK
            );
          }
        }else if ( states[(int) JuraMachineStateIdentifier::HasDoes] == true ) {
          calibrationCoefficient = DISPENSED_ML_CALIBRATION_COEFFICIENT_ESPRESSO;
          dispenseLimitType = JuraMachineDispenseLimitType::Brew;
          if (states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] * DISPENSED_ML_CALIBRATION_COEFFICIENT_ESPRESSO > 10){
            _bridge->machineStateStringChanged(
                JuraMachineStateIdentifier::LastDispenseType, 
                "BREW",
                (int) 1
            );

            /* set brew dispense */
            states[(int) JuraMachineStateIdentifier::LastBrewDispenseVolume] = states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume];
            _bridge->machineStateChanged(
              JuraMachineStateIdentifier::LastBrewDispenseVolume, 
              (int) states[(int) JuraMachineStateIdentifier::LastBrewDispenseVolume] * DISPENSED_ML_CALIBRATION_COEFFICIENT_ESPRESSO
            );
          }

        }else{
           calibrationCoefficient = DISPENSED_ML_CALIBRATION_COEFFICIENT_DEFAULT;
           dispenseLimitType = JuraMachineDispenseLimitType::Water;
           _bridge->machineStateStringChanged(
              JuraMachineStateIdentifier::LastDispenseType, 
              "WATER",
              (int) 2
          );

          /* set water dispense */
          states[(int) JuraMachineStateIdentifier::LastWaterDispenseVolume] = states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume];
          _bridge->machineStateChanged(
            JuraMachineStateIdentifier::LastWaterDispenseVolume, 
            (int) states[(int) JuraMachineStateIdentifier::LastWaterDispenseVolume] * DISPENSED_ML_CALIBRATION_COEFFICIENT_DEFAULT
          );
        }

        /* duration of current dispense */
        states[(int) JuraMachineStateIdentifier::LastDispenseDuration] = (int) ((int) current_millis - (int) start_time)/1000;
        _bridge->machineStateChanged(
          JuraMachineStateIdentifier::LastDispenseDuration, 
          (int) ((int) current_millis - (int) start_time)/1000
        );
      }

      /* for debugging dispense limits 
      ESP_LOGI(TAG," Limit: %2.2f of %i [0:%i, 1:%i, 2:%i]  (raw: %i)", 
        states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] * calibrationCoefficient, 
        dispenseLimitType,
        states[(int)JuraMachineStateIdentifier::WaterLimit], 
        states[(int)JuraMachineStateIdentifier::MilkLimit], 
        states[(int)JuraMachineStateIdentifier::BrewLimit],
        (int) states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] );
      */

      /* check against limit only if we have things that we're dispensing; NOTE; eventually set brew dispense vs. milk dispense ?? */
      if (nonzero_dispense_sum > 4 &&  /* greater than four samples insures that we have sampled at least a few */
          states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] > 15
          ){

          int dispenseLimit = 0;
          switch (dispenseLimitType) {
            case JuraMachineDispenseLimitType::Brew:
              dispenseLimit = states[(int)JuraMachineStateIdentifier::BrewLimit];
              break; 
            case JuraMachineDispenseLimitType::Milk:
              dispenseLimit = states[(int)JuraMachineStateIdentifier::MilkLimit];
              break; 
            case JuraMachineDispenseLimitType::Water:    
              dispenseLimit = states[(int)JuraMachineStateIdentifier::WaterLimit];
              break;
          }

        /* if we have a real dispense here, compare agains tthe limit */
        if (dispenseLimit > 0 && 
            (states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] * calibrationCoefficient) > dispenseLimit - 3){
          /* send cancel command !*/
          ESP_LOGI(TAG,"Cancel command!");
          _bridge->instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier::ConfirmDisplayPrompt);
          clearDispenseLimit(dispenseLimitType);
          vTaskDelay(pdMS_TO_TICKS(750));
        }
      } 
    }
  }

  /* if last dispense volume ~= 1000, just changed the filter */
  last_changed[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] = millis();
}

/***************************************************************************//**
 * Sampler from generator-esque input to determine whether to poll and parse UART
 *
 * @param[out] bool 
 *     
 * @param[in] int iterator 
 ******************************************************************************/
 void JuraMachine::handlePoll(int iterator){

  /* set polling rates */
  int poll_rate_ic = POLL_DISABLED;   /* flow meter, brew group encoder, output valve encoder, errors */
  int poll_rate_cs = POLL_DISABLED;   /* real-time access needed hardware functions and drive modes (grinder, pump, thermoblock) */
  int poll_rate_hz = POLL_DISABLED;   /* non real-time access needed hardware functions and states */
  int poll_rate_rt0 = POLL_DISABLED;
  int poll_rate_rt1 = POLL_DISABLED;
  int poll_rate_rt2 = POLL_DISABLED;

  /* set polling rate based on current machine state */
  switch (states[(int) JuraMachineStateIdentifier::OperationalState] ){
    case (int) JuraMachineOperationalState::Starting:
      /* need to get data from the machine first */
      poll_rate_ic = POLL_DUTY_FULL;
      poll_rate_cs = POLL_DUTY_FULL;
      poll_rate_hz = POLL_DUTY_FULL;

      /* memory */
      poll_rate_rt0 = POLL_DUTY_FULL;
      poll_rate_rt1 = POLL_DUTY_FULL;
      poll_rate_rt2 = POLL_DUTY_FULL;

      break;

    case (int) JuraMachineOperationalState::Finishing:
      /* clear everytihng up; continue the polling  */
      poll_rate_ic = POLL_DUTY_33;
      poll_rate_cs = POLL_DUTY_FULL; /* capture grinder quickly */
      poll_rate_hz = POLL_DUTY_20;

      /* memory */
      poll_rate_rt0 = POLL_DUTY_15;
      poll_rate_rt1 = POLL_DUTY_10;
      poll_rate_rt2 = POLL_DUTY_5;
      break;
    
    case (int) JuraMachineOperationalState::Ready:
      /* awaiting a grind or awaiting pump & thermoblock */
      poll_rate_ic = POLL_DUTY_33;
      poll_rate_cs = POLL_DUTY_FULL; /* capture grinder quickly */
      poll_rate_hz = POLL_DUTY_20;

      /* memory */
      poll_rate_rt0 = POLL_DUTY_15;
      poll_rate_rt1 = POLL_DUTY_10;
      poll_rate_rt2 = POLL_DUTY_5;
      break;
    
    case (int) JuraMachineOperationalState::GrindOperation:
      /* awaiting a grind or awaiting pump & thermoblock */
      poll_rate_cs = POLL_DUTY_FULL; /* refresh grinder asap */
      break;
    
    case (int) JuraMachineOperationalState::BrewOperation:
      /* awaiting a grind or awaiting pump & thermoblock */
      poll_rate_ic = POLL_DUTY_33;
      poll_rate_cs = POLL_DUTY_FULL; /* capture grinder quickly */
      poll_rate_hz = POLL_DUTY_20;

      poll_rate_rt0 = POLL_DUTY_15; /* needed to catch cleaning state based on grounds */
      break;

    case (int) JuraMachineOperationalState::WaterOperation:
      /* awaiting a grind or awaiting pump & thermoblock */
      poll_rate_ic = POLL_DUTY_33;
      poll_rate_cs = POLL_DUTY_FULL; /* capture grinder quickly */
      poll_rate_hz = POLL_DUTY_20;

      poll_rate_rt0 = POLL_DUTY_15;
      break;
    
    case (int) JuraMachineOperationalState::MilkOperation:
      /* awaiting a grind or awaiting pump & thermoblock */
      poll_rate_ic = POLL_DUTY_33;
      poll_rate_cs = POLL_DUTY_FULL; /* capture grinder quickly */
      poll_rate_hz = POLL_DUTY_20;
      break;
    default:
      /*  */
      poll_rate_ic = POLL_DUTY_50;   /* flow meter & errors */
      poll_rate_cs = POLL_DUTY_FULL; 
      poll_rate_hz = POLL_DUTY_33;

      /* memory */
      poll_rate_rt0 = POLL_DUTY_15;
      poll_rate_rt1 = POLL_DUTY_10;
      break;
  }
  
  /* set iterator modulo to throttle machine polling; can change  */
  _ic.setPollRate   (poll_rate_ic);
  _cs.setPollRate   (poll_rate_cs);
  _hz.setPollRate   (poll_rate_hz);
  _rt0.setPollRate  (poll_rate_rt0);
  _rt1.setPollRate  (poll_rate_rt1);
  _rt2.setPollRate  (poll_rate_rt2);

  /* special memory refresh? */
  if (iterator == -1){
    /* reset iterator to 1, disable poll reates for non-memory elements*/
    iterator = 1;
    _ic.setPollRate   (POLL_DISABLED);
    _cs.setPollRate   (POLL_DISABLED);
    _hz.setPollRate   (POLL_DISABLED);
    _rt0.setPollRate  (POLL_DUTY_FULL);
    _rt1.setPollRate  (POLL_DUTY_FULL);
    _rt2.setPollRate  (POLL_DUTY_FULL);
  }

  /* update dump of eeprom_word word 0, advance if a change is registered && if iterator matches instantiation */
  if (_rt0.didUpdate(iterator, _bridge->servicePort)){    
    /* -------------- ESPRESSO -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumEspresso], SUBSTR_INDEX_NUM_ESPRESSO_PREPARATIONS, 0, 50000)){ 
      if (_bridge->machineStateChanged(JuraMachineStateIdentifier::NumEspresso, states[(int) JuraMachineStateIdentifier::NumEspresso])){
        last_changed[(int) JuraMachineStateIdentifier::NumEspresso] = millis();
      }  
    }

    /* -------------- COFFEE -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumCoffee], SUBSTR_INDEX_NUM_COFFEE_PREPARATIONS, 0, 50000)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumCoffee, states[(int) JuraMachineStateIdentifier::NumCoffee])){
        last_changed[(int) JuraMachineStateIdentifier::NumCoffee] = millis();
      }
    }

    /* -------------- CAPPUCCINO -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumCappuccino], SUBSTR_INDEX_NUM_CAPPUCCINO_PREPARATIONS, 0, 50000)){
      if (_bridge->machineStateChanged(JuraMachineStateIdentifier::NumCappuccino, states[(int) JuraMachineStateIdentifier::NumCappuccino])){
        last_changed[(int) JuraMachineStateIdentifier::NumCappuccino] = millis(); 
      }
    }

    /* -------------- MACCHIATO -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumMacchiato], SUBSTR_INDEX_NUM_MACCHIATO_PREPARATIONS, 0, 50000)){
      if (_bridge->machineStateChanged(JuraMachineStateIdentifier::NumMacchiato, states[(int) JuraMachineStateIdentifier::NumMacchiato])){
        last_changed[(int) JuraMachineStateIdentifier::NumMacchiato] = millis(); 
      }     
    }

    /* -------------- PREGROUND -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumPreground], SUBSTR_INDEX_NUM_PREGROUND_PREPARATIONS, 0, 50000)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumPreground, states[(int) JuraMachineStateIdentifier::NumPreground])){
        last_changed[(int) JuraMachineStateIdentifier::NumPreground] = millis(); 
      }
    }

    /* -------------- LOW PRESSURE PUMP -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumLowPressurePumpOperations], SUBSTR_INDEX_NUM_LOW_PRESSURE_PUMP_OPERATIONS, 0, 50000)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumLowPressurePumpOperations, states[(int) JuraMachineStateIdentifier::NumLowPressurePumpOperations])){
        last_changed[(int) JuraMachineStateIdentifier::NumLowPressurePumpOperations] = millis(); 
      }
    }

     /* -------------- MOTOR CYCLES -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumDriveMotorOperations], SUBSTR_INDEX_NUM_DRIVE_MOTOR_OPERATIONS, 0, 50000)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumDriveMotorOperations, states[(int) JuraMachineStateIdentifier::NumDriveMotorOperations])){
        last_changed[(int) JuraMachineStateIdentifier::NumDriveMotorOperations] = millis(); 
      }
    }

    /* -------------- SYSTEM CLEAN -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumBrewGroupCleanOperations], SUBSTR_INDEX_NUM_CLEAN_SYSTEM_OPERATIONS, 0, 50000)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumBrewGroupCleanOperations, states[(int) JuraMachineStateIdentifier::NumBrewGroupCleanOperations])){
        last_changed[(int) JuraMachineStateIdentifier::NumBrewGroupCleanOperations] = millis(); 
      }
    }

    /* -------------- SPENT GROUNDS -------------- */
    int previous_spent_grounds = states[(int) JuraMachineStateIdentifier::NumSpentGrounds];
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumSpentGrounds], SUBSTR_INDEX_NUM_SPENT_GROUNDS, 0, 255)){

      /* append weight if new spent pucks are in knockbox/hopper */
      if (previous_spent_grounds >= 0 && states[(int) JuraMachineStateIdentifier::NumSpentGrounds] < 10){

        /* has there been a change? */
        if (states[(int) JuraMachineStateIdentifier::NumSpentGrounds] != previous_spent_grounds){

          /* did the machine reset? */
          if (states[(int) JuraMachineStateIdentifier::NumSpentGrounds] == 0){
            previous_spent_grounds = 0;
          }

          /* spent grounds has updated; how many have been added since last recording by weight? */
          int pucks_since_last_record = states[(int) JuraMachineStateIdentifier::NumSpentGrounds] - previous_spent_grounds; 
          int avg_weight_since_last_record = pucks_since_last_record * 8.5; /* 7g used for low strenght; 10g for high strength (https://www.coffeeness.de/en/jura-a1-review/) */

          /* update spent ground volume estimation */
          states[(int) JuraMachineStateIdentifier::SpentBeansByWeight] += avg_weight_since_last_record;
          if(_bridge->machineStateChanged(JuraMachineStateIdentifier::SpentBeansByWeight, states[(int) JuraMachineStateIdentifier::SpentBeansByWeight])){
            last_changed[(int) JuraMachineStateIdentifier::SpentBeansByWeight] = millis(); 
          }

          /* hopper is approximately 200g, so... */
          if(_bridge->machineStateChanged(JuraMachineStateIdentifier::BeanHopperLevel, (JURA_MACHINE_BEAN_HOPPER_CAPACITY_G - states[(int) JuraMachineStateIdentifier::SpentBeansByWeight] ) / 2.0 )){
            last_changed[(int) JuraMachineStateIdentifier::BeanHopperLevel] = millis(); 
          }
        }
      }
  
      /* grounds error? */
      bool _grounds_needs_empty = states[(int) JuraMachineStateIdentifier::NumSpentGrounds] > 7;
      if (grounds_needs_empty != _grounds_needs_empty){
        grounds_needs_empty = _grounds_needs_empty;
        if(_bridge->machineStateChanged(JuraMachineStateIdentifier::GroundsNeedsEmpty, grounds_needs_empty)){
          last_changed[(int) JuraMachineStateIdentifier::GroundsNeedsEmpty] = millis();
        }

      }else if ( ! _grounds_needs_empty) {
        if(_bridge->machineStateChanged(JuraMachineStateIdentifier::GroundsNeedsEmpty, false)){
          last_changed[(int) JuraMachineStateIdentifier::GroundsNeedsEmpty] = millis();
        }
      }

      /* cleaning? */
      ESP_LOGI(TAG,"Grounds:  %i", states[(int) JuraMachineStateIdentifier::NumSpentGrounds] );
      bool _is_cleaning = states[(int) JuraMachineStateIdentifier::NumSpentGrounds] > 10;
      if (_is_cleaning != is_cleaning_brew_group){
        is_cleaning_brew_group = _is_cleaning;
        states[(int) JuraMachineStateIdentifier::BrewProgramIsCleaning] = _is_cleaning;
        if(_bridge->machineStateChanged(JuraMachineStateIdentifier::BrewProgramIsCleaning, is_cleaning_brew_group)){
          last_changed[(int) JuraMachineStateIdentifier::BrewProgramIsCleaning] = millis();
        }
      
      }else if (!_is_cleaning){
        states[(int) JuraMachineStateIdentifier::BrewProgramIsCleaning] = _is_cleaning;
        if(_bridge->machineStateChanged(JuraMachineStateIdentifier::BrewProgramIsCleaning, false)){
          last_changed[(int) JuraMachineStateIdentifier::BrewProgramIsCleaning] = millis();
        }
      }
      
      /* update bridge with knockbox capacity values */
      int spent_grounds_level = 100 * states[(int) JuraMachineStateIdentifier::NumSpentGrounds] / 8.0;
      spent_grounds_level = spent_grounds_level> 100 ? 100 : spent_grounds_level < 0 ? 0 : spent_grounds_level;

      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::SpentGroundsLevel, spent_grounds_level)){
        last_changed[(int) JuraMachineStateIdentifier::SpentGroundsLevel] = millis();
      }

      /* num spent grounds can increase beyond 100 during a cleaning operation; but should only report the number of grounds int the knockbox in reasonable range*/
      int num_spent_grounds = states[(int) JuraMachineStateIdentifier::NumSpentGrounds];
      num_spent_grounds = num_spent_grounds> 8 ? 8 : num_spent_grounds < 0 ? 0 : num_spent_grounds;
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumSpentGrounds,num_spent_grounds )){
        last_changed[(int) JuraMachineStateIdentifier::NumSpentGrounds] = millis();
      }
    }

    /* -------------- PREPARATIONS SINCE CLEAN -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt0, &this->states[(int) JuraMachineStateIdentifier::NumPreparationsSinceLastBrewGroupClean], SUBSTR_INDEX_NUM_PREPARATIONS_SINCE_LAST_CLEAN, 0, 50000)){
      
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumPreparationsSinceLastBrewGroupClean, states[(int) JuraMachineStateIdentifier::NumPreparationsSinceLastBrewGroupClean])){
        last_changed[(int) JuraMachineStateIdentifier::NumPreparationsSinceLastBrewGroupClean] = millis();
      } 

      if (states[(int) JuraMachineStateIdentifier::NumPreparationsSinceLastBrewGroupClean] >= BREW_GROUP_CLEAN_THRESHOLD){
        /* recommended now */
        if(_bridge->machineStateChanged(JuraMachineStateIdentifier::CleanBrewGroupRecommended, true)){
          last_changed[(int) JuraMachineStateIdentifier::CleanBrewGroupRecommended] = millis();
        }
       if(_bridge->machineStateChanged(JuraMachineStateIdentifier::CleanBrewGroupRecommendedSoon, true)){
        last_changed[(int) JuraMachineStateIdentifier::CleanBrewGroupRecommendedSoon] = millis();
       }

      }else if (states[(int) JuraMachineStateIdentifier::NumPreparationsSinceLastBrewGroupClean] > BREW_GROUP_CLEAN_RECOMMEND_THRESHOLD){
        
        /* not recommended YET */
        if(_bridge->machineStateChanged(JuraMachineStateIdentifier::CleanBrewGroupRecommended, false)){
          last_changed[(int) JuraMachineStateIdentifier::CleanBrewGroupRecommended] = millis();
        }
        if(_bridge->machineStateChanged(JuraMachineStateIdentifier::CleanBrewGroupRecommendedSoon, true)){
          last_changed[(int) JuraMachineStateIdentifier::CleanBrewGroupRecommendedSoon] = millis();
        }

      } else{
       if(_bridge->machineStateChanged(JuraMachineStateIdentifier::CleanBrewGroupRecommended, false)){
         last_changed[(int) JuraMachineStateIdentifier::CleanBrewGroupRecommended] = millis();
       }
       if(_bridge->machineStateChanged(JuraMachineStateIdentifier::CleanBrewGroupRecommendedSoon, false)){
         last_changed[(int) JuraMachineStateIdentifier::CleanBrewGroupRecommendedSoon] = millis();
       }
      }
    }

    /* unhandled changes for reporting */
    if (_rt0.hasUnhandledUpdate() && PRINT_UNHANDLED){_rt0.printUnhandledUpdate();}
  }
  
  /* update dump of eeprom_word word 1, advance if a change is registered && if iterator matches instantiation */
  if (_rt1.didUpdate(iterator, _bridge->servicePort)){
    
    /* -------------- HIGH PRESSURE PUMP -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt1, &this->states[(int) JuraMachineStateIdentifier::NumHighPressurePumpOperations], SUBSTR_INDEX_NUM_HIGH_PRESSURE_PUMP_OPERATIONS, 0, 50000)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumHighPressurePumpOperations, states[(int) JuraMachineStateIdentifier::NumHighPressurePumpOperations])){
        last_changed[(int) JuraMachineStateIdentifier::NumHighPressurePumpOperations] = millis();
      }
    }

    /* -------------- MILK FOAM PREPARATIONS -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt1, &this->states[(int) JuraMachineStateIdentifier::NumMilkFoamPreparations], SUBSTR_INDEX_NUM_MILK_FOAM_PREPARATIONS, 0, 50000)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumMilkFoamPreparations, states[(int) JuraMachineStateIdentifier::NumMilkFoamPreparations])){
        last_changed[(int) JuraMachineStateIdentifier::NumMilkFoamPreparations] = millis();
      }
    }

    /* -------------- WATER PREPARATIONS -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt1, &this->states[(int) JuraMachineStateIdentifier::NumWaterPreparations], SUBSTR_INDEX_NUM_WATER_PREPARATIONS, 0, 50000)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumWaterPreparations, states[(int) JuraMachineStateIdentifier::NumWaterPreparations])){
        last_changed[(int) JuraMachineStateIdentifier::NumWaterPreparations] = millis();
      }
    }

    /* -------------- GRINDER OPERATIONS -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt1, &this->states[(int) JuraMachineStateIdentifier::NumGrinderOperations], SUBSTR_INDEX_NUM_GRINDER_OPERATIONS, 0, 50000)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::NumGrinderOperations, states[(int) JuraMachineStateIdentifier::NumGrinderOperations])){
        last_changed[(int) JuraMachineStateIdentifier::NumGrinderOperations] = millis();
      };
    }

    /* -------------- MILK CLEAN OPERATIONS -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt1, &this->states[(int) JuraMachineStateIdentifier::NumCleanMilkSystemOperations], SUBSTR_INDEX_NUM_CLEAN_MILK_SYSTEM_OPERATIONS, 0, 50000)){
      if (_bridge->machineStateChanged(JuraMachineStateIdentifier::NumCleanMilkSystemOperations, states[(int) JuraMachineStateIdentifier::NumCleanMilkSystemOperations])){
        last_changed[(int) JuraMachineStateIdentifier::NumCleanMilkSystemOperations] = millis();
        
      }
    }

    /* unhandled changes for reporting */
    if (_rt1.hasUnhandledUpdate() && PRINT_UNHANDLED){_rt1.printUnhandledUpdate();}
  }
  
  /* update dump of eeprom_word word 2, advance if a change is registered && if iterator matches instantiation */
  if (_rt2.didUpdate(iterator, _bridge->servicePort)){

    /* -------------- HAS FILTER -------------- */
    if (didUpdateJuraMemoryLineValue(&_rt2, &this->states[(int) JuraMachineStateIdentifier::HasFilter], SUBSTR_INDEX_HAS_FILTER, 0, 20)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::HasFilter, (((int) states[(int) JuraMachineStateIdentifier::HasFilter] & (int) 16) != 0))){
        last_changed[(int) JuraMachineStateIdentifier::HasFilter] = millis();
      };
    }

    /* unhandled changes for reporting */
    if (_rt2.hasUnhandledUpdate() && PRINT_UNHANDLED){
      _rt2.printUnhandledUpdate();
    }
  }
  /* update collection of input and control board values, advance if a change is registered && if iterator matches instantiation */
  if (_ic.didUpdate(iterator, _bridge->servicePort)){
    
    /* -------------- BEAN HOPPER COVER -------------- */
    if (didUpdateJuraInputControlBoardValue(&this->states[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen], SUBSTR_INDEX_BEAN_HOPPER_COVER_OPEN_IC, 1, JuraInputBoardBinaryResponseInterpretation::Inverted)){      
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::BeanHopperCoverOpen, states[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen])){
        handleBeanHopperCoverOpen();
        last_changed[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen] = millis();
      };
    }

    /* -------------- WATER RESERVOIR NEEDS FILL -------------- */
    if (didUpdateJuraInputControlBoardValue(&this->states[(int) JuraMachineStateIdentifier::WaterReservoirNeedsFill], SUBSTR_INDEX_WATER_RESERVOIR_NEEDS_FILL_IC, 1,JuraInputBoardBinaryResponseInterpretation::AsReported)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::WaterReservoirNeedsFill, states[(int) JuraMachineStateIdentifier::WaterReservoirNeedsFill])){
        last_changed[(int) JuraMachineStateIdentifier::WaterReservoirNeedsFill] = millis();
      }
    }

    /* -------------- BYPASS DOSER  -------------- */
    if (didUpdateJuraInputControlBoardValue(&this->states[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen], SUBSTR_INDEX_BYPASS_DOSER_COVER_OPEN_IC, 1, JuraInputBoardBinaryResponseInterpretation::AsReported)){
      if (_bridge->machineStateChanged(JuraMachineStateIdentifier::BypassDoserCoverOpen, states[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen])){
        last_changed[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen] = millis();

        /* dose has be inserted */
        if (states[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen] == 1){
          states[(int) JuraMachineStateIdentifier::HasDoes] = true;
          _bridge->machineStateChanged(JuraMachineStateIdentifier::HasDoes ,true);
        }
      }
    }

    /* -------------- DRIP TRAY REMOVED -------------- */
    if (didUpdateJuraInputControlBoardValue(&this->states[(int) JuraMachineStateIdentifier::DrainageTrayRemoved], SUBSTR_INDEX_DRAINAGE_TRAY_REMOVED_IC, 1, JuraInputBoardBinaryResponseInterpretation::Inverted)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::DrainageTrayRemoved, states[(int) JuraMachineStateIdentifier::DrainageTrayRemoved])){
        last_changed[(int) JuraMachineStateIdentifier::DrainageTrayRemoved] = millis();
      }
    }

    /* -------------- BREW GROUP ENCODER 4 - 5 -------------- */
    if (didUpdateJuraInputControlBoardValue(&this->states[(int) JuraMachineStateIdentifier::BrewGroupEncoderState], SUBSTR_INDEX_BREW_GROUP_ENCODER_STATE, 2, JuraInputBoardBinaryResponseInterpretation::AsReported)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::BrewGroupEncoderState, states[(int) JuraMachineStateIdentifier::BrewGroupEncoderState])){
        last_changed[(int) JuraMachineStateIdentifier::BrewGroupEncoderState] = millis();
      }
    }

    /* -------------- OUTPUT VALVE SERVO ENCODER POSITION 14 - 15 -------------- */
    if (didUpdateJuraInputControlBoardValue(&this->states[(int) JuraMachineStateIdentifier::OutputValveEncoderState], SUBSTR_INDEX_OUTPUT_VALVE_ENCODER_STATE, 2, JuraInputBoardBinaryResponseInterpretation::AsReported)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::OutputValveEncoderState, states[(int) JuraMachineStateIdentifier::OutputValveEncoderState])){
          last_changed[(int) JuraMachineStateIdentifier::OutputValveEncoderState] = millis();
          states[(int) JuraMachineStateIdentifier::OutputValveNominalPosition] = (states[(int) JuraMachineStateIdentifier::OutputValveEncoderState] == 2);
          if(_bridge->machineStateChanged(JuraMachineStateIdentifier::OutputValveNominalPosition,  states[(int) JuraMachineStateIdentifier::OutputValveNominalPosition] )){
            last_changed[(int) JuraMachineStateIdentifier::OutputValveNominalPosition] = millis();
          }
      };
    }

    /* unhandled changes for reporting */
    if (_ic.hasUnhandledUpdate() && PRINT_UNHANDLED){_ic.printUnhandledUpdate();}
  }
  
  /* update collection of heated beverage realtime output values, advance if a change is registered && if iterator matches instantiation */
  if (_hz.didUpdate(iterator, _bridge->servicePort)){
    
    /* -------------- WATER RINSE RECOMMENDED -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::RinseBrewGroupRecommended], SUBSTR_INDEX_RINSE_BREW_GROUP_RECOMMENDED, 0, 1)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::RinseBrewGroupRecommended, states[(int) JuraMachineStateIdentifier::RinseBrewGroupRecommended])){
        last_changed[(int) JuraMachineStateIdentifier::RinseBrewGroupRecommended] = millis();
      }
    }

    /* -------------- THERMOBLOCK PREHEATED -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::ThermoblockPreheated], SUBSTR_INDEX_THERMOBLOCK_PREHEATED, 0, 1)){
     if( _bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockPreheated, states[(int) JuraMachineStateIdentifier::ThermoblockPreheated])){
      last_changed[(int) JuraMachineStateIdentifier::ThermoblockPreheated] = millis();
     }
    }

    /* -------------- THERMOBLOCK NOMINAL -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::ThermoblockReady], SUBSTR_INDEX_THERMOBLOCK_READY, 0, 1)){
     if( _bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockReady, states[(int) JuraMachineStateIdentifier::ThermoblockReady])){
      last_changed[(int) JuraMachineStateIdentifier::ThermoblockReady] = millis();
     }
    }

     /* -------------- LAST BREW_GROUP OPERATION -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::BrewGroupLastOperation], SUBSTR_INDEX_LAST_BREW_OPERATION, 0, 25)){
      if (states[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] == 0){
        if(_bridge->machineStateStringChanged(JuraMachineStateIdentifier::BrewGroupLastOperation, "None", 0)){
          last_changed[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] = millis();
        }

      }else if (states[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] == 15 || states[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] == 16){
        if(_bridge->machineStateStringChanged(JuraMachineStateIdentifier::BrewGroupLastOperation, "LOW PRESSURE BREW", 1)){
          last_changed[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] = millis();
        }; /* coffee */
      
      }else if ((states[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] == 21) || (states[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] == 20)){
        if(_bridge->machineStateStringChanged(JuraMachineStateIdentifier::BrewGroupLastOperation, "HIGH PRESSURE BREW", 2)){
          last_changed[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] = millis();
        } /* espresso drink */
      
      }else if ((states[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] == 22) ){
        if(_bridge->machineStateStringChanged(JuraMachineStateIdentifier::BrewGroupLastOperation, "RINSE", 3)){
          last_changed[(int) JuraMachineStateIdentifier::BrewGroupLastOperation] = millis();
        }
      
      }else{
        ESP_LOGI(TAG,"Other Last Brew Program State: %i", states[(int) JuraMachineStateIdentifier::BrewGroupLastOperation]);
        /* 14 = canceled cappucino?? */
      }
    }

    /* -------------- OUTPUT STATUS  -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::BrewGroupOutputStatus], SUBSTR_INDEX_OUTPUT_VALVE_POSITION, 0, 50000)){
      handleBrewGroupOutput();     
    }

    /* -------------- THERMOBLOCK TEMPERATURE C -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::ThermoblockTemperature], SUBSTR_INDEX_THERMOBLOCK_TEMPERATURE, 0, 2000)){
      int _temp = states[(int) JuraMachineStateIdentifier::ThermoblockTemperature] / 10.0;
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockTemperature, _temp)){
        last_changed[(int) JuraMachineStateIdentifier::ThermoblockTemperature] = millis();
        handleThermoblockTemperature(_temp);
      };
    }

    /* -------------- LAST DISPENSE ML -------------- */
    int prior_dispense = states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume];
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume], SUBSTR_INDEX_LAST_DISPENSE_PUMPED_WATER_VOLUME_ML, 0, 50000)){
      if (_bridge->machineStateChanged(JuraMachineStateIdentifier::LastDispensePumpedWaterVolume, states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume] * DISPENSED_ML_CALIBRATION_COEFFICIENT_DEFAULT)){
        handleLastDispenseChange(prior_dispense);
      }
    }

    /* -------------- CERAMIC VALVE POSITION -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::CeramicValvePosition], SUBSTR_INDEX_CERAMIC_VALVE_POSITION, 0, 10)){
      last_changed[(int) JuraMachineStateIdentifier::CeramicValvePosition] = millis();
      handleCeramicValve();
    }

    /* -------------- BEAN HOPPER COVER OPEN -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen], SUBSTR_INDEX_BEAN_HOPPER_COVER_OPEN_HZ, 0, 1)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::BeanHopperCoverOpen, states[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen])){
        handleBeanHopperCoverOpen();
        last_changed[(int) JuraMachineStateIdentifier::BeanHopperCoverOpen] = millis();
      };
    }

    /* -------------- WATER RESERVOIR NEEDS FILL -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::WaterReservoirNeedsFill], SUBSTR_INDEX_WATER_RESERVOIR_NEEDS_FILL_HZ, 0, 1)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::WaterReservoirNeedsFill, states[(int) JuraMachineStateIdentifier::WaterReservoirNeedsFill])){
        last_changed[(int) JuraMachineStateIdentifier::WaterReservoirNeedsFill] = millis();
      }
    }

    /* -------------- BYPASS DOSER COVER OPEN -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen], SUBSTR_INDEX_BYPASS_DOSER_COVER_OPEN_HZ, 0, 1)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::BypassDoserCoverOpen, states[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen])){
        last_changed[(int) JuraMachineStateIdentifier::BypassDoserCoverOpen] = millis();
      }
    }

    /* -------------- BYPASS DOSER COVER OPEN -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::DrainageTrayRemoved], SUBSTR_INDEX_DRAINAGE_TRAY_REMOVED_HZ, 0, 1)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::DrainageTrayRemoved, states[(int) JuraMachineStateIdentifier::DrainageTrayRemoved])){
        last_changed[(int) JuraMachineStateIdentifier::DrainageTrayRemoved] = millis();
      }
    }

    /* -------------- THREMOBLOCK IN MILK DISPENSE MODE -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::ThermoblockMilkDispenseMode], SUBSTR_INDEX_THERMOBLOCK_MILK_DISPENSE_MODE, 0, 1)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockMilkDispenseMode, states[(int) JuraMachineStateIdentifier::ThermoblockMilkDispenseMode])){
        last_changed[(int) JuraMachineStateIdentifier::ThermoblockMilkDispenseMode] = millis();
      }
    }

    /* -------------- VENTURI PUMPING -------------- */
    if (didUpdateJuraHeatedBeverageValue(&this->states[(int) JuraMachineStateIdentifier::VenturiPumping], SUBSTR_INDEX_VENTURI_PUMPING, 0, 1)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::VenturiPumping, states[(int) JuraMachineStateIdentifier::VenturiPumping])){
        last_changed[(int) JuraMachineStateIdentifier::VenturiPumping] = millis();
      }
    }

    /* unhandled changes for reporting */
    if (_hz.hasUnhandledUpdate() && PRINT_UNHANDLED){_hz.printUnhandledUpdate();}
  }
  
  /* update collection of system circuitry values realtime output values, advance if a change is registered && if iterator matches instantiation */
  if (_cs.didUpdate(iterator, _bridge->servicePort)){
 
    /* -------------- THERMOBLOCK TEMPERATURE -------------- */
    if (didUpdateJuraSystemCircuitValue(&this->states[(int) JuraMachineStateIdentifier::ThermoblockTemperature], SUBSTR_DEC_INDEX_THERMOBLOCK_TEMPERATURE, 1, JuraSystemCircuitryBinaryResponseInterpretation::AsReported, JuraSystemCircuitryResponseDataType::Decimal)){
      int _temp = states[(int) JuraMachineStateIdentifier::ThermoblockTemperature] / 10.0;
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockTemperature, _temp)){
        handleThermoblockTemperature(_temp);
        last_changed[(int) JuraMachineStateIdentifier::ThermoblockTemperature] = millis();
      }
    }

    /* -------------- BREW GROUP OUTPUT POSITION -------------- */
    if (didUpdateJuraSystemCircuitValue(&this->states[(int) JuraMachineStateIdentifier::BrewGroupOutputStatus], SUBSTR_DEC_INDEX_OUTPUT_STATUS, 1, JuraSystemCircuitryBinaryResponseInterpretation::AsReported, JuraSystemCircuitryResponseDataType::Decimal)){
      handleBrewGroupOutput();  
    }

    /* -------------- LAST DISPENSE ML -------------- */
    int prior_dispense = states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume];
    if (didUpdateJuraSystemCircuitValue(&this->states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume], SUBSTR_DEC_INDEX_LAST_DISPENSE_PUMPED_WATER_VOLUME_ML, 1, JuraSystemCircuitryBinaryResponseInterpretation::AsReported, JuraSystemCircuitryResponseDataType::Decimal)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::LastDispensePumpedWaterVolume, states[(int) JuraMachineStateIdentifier::LastDispensePumpedWaterVolume]* DISPENSED_ML_CALIBRATION_COEFFICIENT_DEFAULT)){
        handleLastDispenseChange(prior_dispense);
      }  
    }

    /* -------------- PROGRAM STATE? -------------- */
    if (didUpdateJuraSystemCircuitValue(&this->states[(int) JuraMachineStateIdentifier::BrewProgramNumericState], SUBSTR_DEC_INDEX_CIRCUIT_READY_STATE, 1, JuraSystemCircuitryBinaryResponseInterpretation::AsReported, JuraSystemCircuitryResponseDataType::Decimal)){
      //_bridge->machineStateChanged(JuraMachineStateIdentifier::BrewProgramIsReady, states[(int) JuraMachineStateIdentifier::BrewProgramNumericState] == 260);
    }

    /* -------------- CERAMIC VALVE POSITION -------------- */
    if (didUpdateJuraSystemCircuitValue(&this->states[(int) JuraMachineStateIdentifier::CeramicValvePosition], SUBSTR_BIN_INDEX_CERAMIC_VALVE_POSITION, 4, JuraSystemCircuitryBinaryResponseInterpretation::AsReported, JuraSystemCircuitryResponseDataType::Binary)){
      handleCeramicValve();
    }

    /* -------------- THERMOBLOCK ACTIVE -------------- */
    if (didUpdateJuraSystemCircuitValue(&this->states[(int) JuraMachineStateIdentifier::ThermoblockDutyCycle], SUBSTR_BIN_INDEX_THERMOBLOCK_DUTY_CYCLE, 12, JuraSystemCircuitryBinaryResponseInterpretation::AsReported, JuraSystemCircuitryResponseDataType::Hamming)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockDutyCycle, states[(int) JuraMachineStateIdentifier::ThermoblockDutyCycle] * 10)){
        last_changed[(int) JuraMachineStateIdentifier::ThermoblockDutyCycle] = millis();
      }
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::ThermoblockActive, states[(int) JuraMachineStateIdentifier::ThermoblockDutyCycle] != 0)){
        last_changed[(int) JuraMachineStateIdentifier::ThermoblockActive] = millis();;
      }
    }

    /* -------------- PUMP ACTIVE -------------- */
    if (didUpdateJuraSystemCircuitValue(&this->states[(int) JuraMachineStateIdentifier::PumpDutyCycle], SUBSTR_BIN_INDEX_PUMP_DUTY_CYCLE, 12, JuraSystemCircuitryBinaryResponseInterpretation::AsReported, JuraSystemCircuitryResponseDataType::Hamming)){

      /* because our sampling rate doesn't work well for PEP extraction, we only set the pump to off when the output and ceramic valves are not in a special mode */
      int pump_duty_cycle = states[(int) JuraMachineStateIdentifier::PumpDutyCycle];

      /* pump doesn't have istent duty when sampled during venturi pumping */
      if (pump_duty_cycle == 0 && (states[(int) JuraMachineStateIdentifier::VenturiPumping] == 0) ){

        if (states[(int) JuraMachineStateIdentifier::CeramicValvePosition] == 3 && states[(int) JuraMachineStateIdentifier::BrewGroupOutputStatus] == 1 ){
          states[(int) JuraMachineStateIdentifier::PumpActive] = (pump_duty_cycle > 0);
          if(_bridge->machineStateChanged(JuraMachineStateIdentifier::PumpDutyCycle, pump_duty_cycle * 10)){
            last_changed[(int) JuraMachineStateIdentifier::PumpDutyCycle] = millis();
          }
          if(_bridge->machineStateChanged(JuraMachineStateIdentifier::PumpActive, pump_duty_cycle > 0)){
            last_changed[(int) JuraMachineStateIdentifier::PumpActive] = millis();
          }
        
        }else{
          states[(int) JuraMachineStateIdentifier::PumpActive] = (pump_duty_cycle > 0);
          if(_bridge->machineStateChanged(JuraMachineStateIdentifier::PumpActive, pump_duty_cycle > 0)){
            last_changed[(int) JuraMachineStateIdentifier::PumpActive] = millis();
          }
        }
      }else{
        states[(int) JuraMachineStateIdentifier::PumpActive] = true;
        if(_bridge->machineStateChanged(JuraMachineStateIdentifier::PumpDutyCycle, pump_duty_cycle * 10.0)){
          last_changed[(int) JuraMachineStateIdentifier::PumpDutyCycle] = millis();
        }
        if(_bridge->machineStateChanged(JuraMachineStateIdentifier::PumpActive, true)){
          last_changed[(int) JuraMachineStateIdentifier::PumpActive] = millis();
        }
      }
    }

    /* -------------- GRINDER ACTIVE -------------- */
    if (didUpdateJuraSystemCircuitValue(&this->states[(int) JuraMachineStateIdentifier::GrinderDutyCycle], SUBSTR_BIN_INDEX_GRINDER_DUTY_CYCLE, 12, JuraSystemCircuitryBinaryResponseInterpretation::AsReported, JuraSystemCircuitryResponseDataType::Hamming)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::GrinderDutyCycle, states[(int) JuraMachineStateIdentifier::GrinderDutyCycle] * 10)){
        last_changed[(int) JuraMachineStateIdentifier::GrinderDutyCycle] = millis();
      }
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::GrinderActive, states[(int) JuraMachineStateIdentifier::GrinderDutyCycle] != 0)){
        last_changed[(int) JuraMachineStateIdentifier::GrinderActive] = millis();
        /* true grinder state change; what to do? */
        if (states[(int) JuraMachineStateIdentifier::GrinderActive] == true){
          states[(int) JuraMachineStateIdentifier::HasDoes] = true;
          _bridge->machineStateChanged(JuraMachineStateIdentifier::HasDoes,true);
        }
      };
      states[(int) JuraMachineStateIdentifier::GrinderActive] = states[(int) JuraMachineStateIdentifier::GrinderDutyCycle] != 0;
    }

    /* -------------- BREW GROUP DUTY CYCLE -------------- */
    if (didUpdateJuraSystemCircuitValue(&this->states[(int) JuraMachineStateIdentifier::BrewGroupDutyCycle], SUBSTR_BIN_INDEX_BREW_GROUP_DUTY_CYCLE, 12, JuraSystemCircuitryBinaryResponseInterpretation::AsReported, JuraSystemCircuitryResponseDataType::Hamming)){
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::BrewGroupDutyCycle, states[(int) JuraMachineStateIdentifier::BrewGroupDutyCycle] * 10)){
        last_changed[(int) JuraMachineStateIdentifier::BrewGroupDutyCycle] = millis();
      }
      if(_bridge->machineStateChanged(JuraMachineStateIdentifier::BrewGroupActive, states[(int) JuraMachineStateIdentifier::BrewGroupDutyCycle] != 0)){
        last_changed[(int) JuraMachineStateIdentifier::BrewGroupActive] = millis();
      }
      states[(int) JuraMachineStateIdentifier::BrewGroupActive] = states[(int) JuraMachineStateIdentifier::BrewGroupDutyCycle] != 0;
    }

    /* unhandled changes for reporting */
    if (_cs.hasUnhandledUpdate() && PRINT_UNHANDLED){_cs.printUnhandledUpdate();}
  }
  
  /* ---------------------- CALCULATED STATES FOLLOW  ---------------------- */
  if (errorStateHasChanged(iterator, POLL_DUTY_FULL)){
    _bridge->machineStateChanged(JuraMachineStateIdentifier::HasError, states[(int) JuraMachineStateIdentifier::HasError] );
  }

  if (recommendationStateHasChanged(iterator, POLL_DUTY_FULL)){
    _bridge->machineStateChanged(JuraMachineStateIdentifier::HasMaintenanceRecommendation, states[(int) JuraMachineStateIdentifier::HasMaintenanceRecommendation]);
  }
  
  if (flowStateHasChanged(iterator, POLL_DUTY_FULL)){
    _bridge->machineStateChanged(JuraMachineStateIdentifier::FlowState,  states[(int) JuraMachineStateIdentifier::FlowState] );
  }
  
  /* calculated meta state */
  if (handleMachineState(iterator, POLL_DUTY_FULL)){

    /* iterate through each operational state */
    bool isReady = false;

    switch(states[(int) JuraMachineStateIdentifier::OperationalState]){
      case (int) JuraMachineOperationalState::Starting:
        ESP_LOGI(TAG,"--> Machine State: BOOTING"); 
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "BOOTING", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;

       case (int) JuraMachineOperationalState::Finishing:
        isReady = true;
        ESP_LOGI(TAG,"--> Machine State: FINISHING");  
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "READY", states[(int) JuraMachineStateIdentifier::OperationalState]);
        
        /* protect updating of the system is ready flag */
        xSemaphoreTake( xMachineReadyStateVariableSemaphore, portMAX_DELAY );
        states[(int) JuraMachineStateIdentifier::SystemIsReady] = true; 
        xSemaphoreGive(xMachineReadyStateVariableSemaphore);

        determineReadyStateType();
        break;
        
      case (int) JuraMachineOperationalState::Ready:
        isReady = true;
        ESP_LOGI(TAG,"--> Machine State: READY"); 

         /* protect updating of the system is ready flag */
        xSemaphoreTake( xMachineReadyStateVariableSemaphore, portMAX_DELAY );
        states[(int) JuraMachineStateIdentifier::SystemIsReady] = true; 
        xSemaphoreGive(xMachineReadyStateVariableSemaphore);

        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "READY", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::BlockingError:
        ESP_LOGI(TAG,"--> Machine State: ERROR"); 
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "ERROR", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::GrindOperation: 
        ESP_LOGI(TAG,"--> Machine State: GRIND OPERATION");
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "GRIND OPERATION", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::BrewOperation: 
        ESP_LOGI(TAG,"--> Machine State: BREW GROUP OPERATION");  
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "BREW GROUP OPERATION", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::RinseOperation: 
        ESP_LOGI(TAG,"--> Machine State: RINSE OPERATION");  
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "RINSE OPERATION", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::WaterOperation:
        ESP_LOGI(TAG,"--> Machine State: WATER OPERATION");  
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "WATER OPERATION", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::MilkOperation:  
        ESP_LOGI(TAG,"--> Machine State: MILK OPERATION");  
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "MILK OPERATION", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::Cleaning:  
        ESP_LOGI(TAG,"--> Machine State: CLEANING");       
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "CLEANING", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::HeatingOperation:
        ESP_LOGI(TAG,"--> Machine State: HEATING");       
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "HEAT OPERATION", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::AwaitRotaryInput:
        ESP_LOGI(TAG,"--> Machine State: AWAITING ROTARY");  
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "AWAITING ROTARY", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::ProgramPause:
        ESP_LOGI(TAG,"--> Machine State: PAUSE");  
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "WAITING", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      case (int) JuraMachineOperationalState::Unknown:
        ESP_LOGI(TAG,"--> Machine State: UNKNOWN");  
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "UNKNOWN", states[(int) JuraMachineStateIdentifier::OperationalState]);
        break;
      default:
        ESP_LOGI(TAG,"--> Machine State: UNDETERMINED %i", states[(int) JuraMachineStateIdentifier::OperationalState]);  
        _bridge->machineStateStringChanged(JuraMachineStateIdentifier::OperationalState, "UNKNOWN", states[(int) JuraMachineStateIdentifier::OperationalState]); 
        break;
    }

    /* reset */
    if (!isReady){
       //ESP_LOGI(TAG,"----> Machine Operation: WAITING READY");
      _bridge->machineStateStringChanged(JuraMachineStateIdentifier::ReadyStateDetail, "WAITING READY", (int) JuraMachineReadyState::ExecutingOperation);
    }

    /* protect setting of global ready state */
    xSemaphoreTake( xMachineReadyStateVariableSemaphore, portMAX_DELAY );

    /* set ready state globally */
    states[(int) JuraMachineStateIdentifier::SystemIsReady] = (states[(int) JuraMachineStateIdentifier::OperationalState] == (int) JuraMachineOperationalState::Ready);

    /* set binary system ready */
    _bridge->machineStateChanged(JuraMachineStateIdentifier::SystemIsReady, states[(int) JuraMachineStateIdentifier::SystemIsReady]) ;
    xSemaphoreGive(xMachineReadyStateVariableSemaphore);
  }
}
