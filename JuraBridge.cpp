#include "JuraBridge.h"
#include "Preferences.h"
#include "PubSubClient.h"
#include "Version.h"

#define DEFAULT_NUMERIC_STATE 999

/* class declaration */
JuraBridge::JuraBridge(
  Preferences &prefsRef, 
  PubSubClient &mqttClientRef,  
  SemaphoreHandle_t &xMQTTSemaphoreRef, 
  SemaphoreHandle_t &xUARTSemaphoreRef) :  
  preferences(prefsRef), 
  mqttClient(mqttClientRef), 
  xMQTTSemaphore(xMQTTSemaphoreRef), 
  xUARTSemaphore(xUARTSemaphoreRef), 
  servicePort(xUARTSemaphoreRef) {
  
  /* init reported states to high array */
  for (int i = 0; i < 100; i++) { _reportableNumericStates[i] = DEFAULT_NUMERIC_STATE;}

  /* set index association table */
  int stateAttributeArraySize = sizeof(JuraEntityConfigurations) / sizeof(JuraEntityConfigurations[0]) ; 
  for (int i = 0; i < stateAttributeArraySize ; i++){
    _stateIndexToConfigurationIndex[(int)JuraEntityConfigurations[i].state] = i; 
  }
}

/***************************************************************************//**
 * Publish to mqtt broker all the machine entity configurations 
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
  void JuraBridge::publishMachineEntityConfigurations() {

  /* iterate through state attributes */
  int stateAttributeArraySize = sizeof(JuraEntityConfigurations) / sizeof(JuraEntityConfigurations[0]) ; 

  /* operaitonal state topic */
  char operational_state_topic[56]; 
  sprintf(
    operational_state_topic, 
    MQTT_ROOT "/%i/%i/%i", 
    JuraEntityConfigurations[0].machineSubsystemType, 
    JuraEntityConfigurations[0].subsystemAttributeType,
    0
  );

  /* iterate through all of the machine state attributes */
  for (int i = 0; i < stateAttributeArraySize; i++){

    /* buffer for configuration topic */
    char config_topic[255];   
    
    /* mqtt configuration */
    char buffer[2048];  DynamicJsonDocument mqttJsonConfigurationBody(2048);
 
    /* different payload types */
    if (JuraEntityConfigurations[i].dataType == JuraMachineStateDataType::Boolean){ 
      sprintf(config_topic, "homeassistant/binary_sensor/%s/config", JuraEntityConfigurations[i].entity_id);
    
    } else{
      sprintf(config_topic, "homeassistant/sensor/%s/config", JuraEntityConfigurations[i].entity_id);
    }

    /* ------- clear if disabled ------- */
    if (JuraEntityConfigurations[i].enabled == JuraEntityEnabled::No) {
      ESP_LOGI(TAG,"[disabled]: %s", config_topic);
      xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );
      mqttClient.publish(config_topic, "");  
      xSemaphoreGive(xMQTTSemaphore);
      continue;
    }

    /* unqiue id calculated*/
    char unique_id[255]; 
    sprintf(unique_id, ENTITY_PREFIX "uid_%s", JuraEntityConfigurations[i].entity_id);

    /* create body for this entity */
    mqttJsonConfigurationBody["name"] = JuraEntityConfigurations[i].name;
    mqttJsonConfigurationBody["unique_id"] = unique_id;

    /* construct state topic by index */
    char message_topic[56]; 
    sprintf(
      message_topic, 
      MQTT_ROOT "/%i/%i/%i", 
      JuraEntityConfigurations[i].machineSubsystemType, 
      JuraEntityConfigurations[i].subsystemAttributeType,
      i
    );

    /* define state topic */
    mqttJsonConfigurationBody["state_topic"] = message_topic;

    /* availability topics */
    if ( i > 0 && JuraEntityConfigurations[i].availabilityFollowsMachine == JuraEntityAvailabilityFollowsReadyState::Yes){
        mqttJsonConfigurationBody["availability"][0]["payload_available"] = "READY";
        mqttJsonConfigurationBody["availability"][0]["payload_not_available"] = "OFF";
        mqttJsonConfigurationBody["availability"][0]["topic"] = operational_state_topic;
    } 

    /* translate unit enum */
    switch (JuraEntityConfigurations[i].unitOfMeasure){
      case JuraMachineStateUnit::Preparation:
        mqttJsonConfigurationBody["unit_of_measurement"] = "preparations";
        break;
      case JuraMachineStateUnit::Operation:
        mqttJsonConfigurationBody["unit_of_measurement"] = "operations";
        break;
      case JuraMachineStateUnit::Celcius: 
        mqttJsonConfigurationBody["unit_of_measurement"] = "°C";
        break;
      case JuraMachineStateUnit::MicrolitersPerSecond: 
        mqttJsonConfigurationBody["unit_of_measurement"] = "uL/s";
        break;
      case JuraMachineStateUnit::Milliliters: 
        mqttJsonConfigurationBody["unit_of_measurement"] = "ml";
        break;
      case JuraMachineStateUnit::Grams: 
        mqttJsonConfigurationBody["unit_of_measurement"] = "g";
        break;
      case JuraMachineStateUnit::Percent:
        mqttJsonConfigurationBody["unit_of_measurement"] = "%";
        break;
      case JuraMachineStateUnit::Dose:
        mqttJsonConfigurationBody["unit_of_measurement"] = "doses";
        break;
      case JuraMachineStateUnit::Cycle:
        mqttJsonConfigurationBody["unit_of_measurement"] = "cycles";
        break;
      case JuraMachineStateUnit::Seconds:
        mqttJsonConfigurationBody["unit_of_measurement"] = "s";
        break;
      case JuraMachineStateUnit::None:
        mqttJsonConfigurationBody["unit_of_measurement"] = "";
        break;
    }

    /* translate device class enum into a string for json */  
    if (JuraEntityConfigurations[i].dataType == JuraMachineStateDataType::Boolean){ 
      switch(JuraEntityConfigurations[i].deviceClass){
        case JuraMachineDeviceClass::Opening:
          mqttJsonConfigurationBody["device_class"] = "opening";
          break;
        case JuraMachineDeviceClass::Problem:
          mqttJsonConfigurationBody["device_class"] = "problem";
          break;
        case JuraMachineDeviceClass::Door:
          mqttJsonConfigurationBody["device_class"] = "door";
          break;
        case JuraMachineDeviceClass::Moving:
          mqttJsonConfigurationBody["device_class"] = "moving";
          break;
        case JuraMachineDeviceClass::Power:
          mqttJsonConfigurationBody["device_class"] = "power";
          break;
        case JuraMachineDeviceClass::Running:
          mqttJsonConfigurationBody["device_class"] = "running";
          break;
        case JuraMachineDeviceClass::None: 
          break; 
      }

      /* special handling for binary sensors */
      mqttJsonConfigurationBody["payload_on"] = "1";
      mqttJsonConfigurationBody["payload_off"] = "0";
    
    }else if (JuraEntityConfigurations[i].dataType == JuraMachineStateDataType::String){ 
      /* necessary to show the sensor as a text value */
      mqttJsonConfigurationBody["device_class"] = "enum";
    }

    /* translate icon enum into an mdi: string for json */  
    switch(JuraEntityConfigurations[i].icon){
      case JuraMachineStateIcon::Coffee:
        mqttJsonConfigurationBody["icon"] = "mdi:coffee";
        break;
      case JuraMachineStateIcon::Info:
        mqttJsonConfigurationBody["icon"] = "mdi:information";
        break;
      case JuraMachineStateIcon::Counter:
        mqttJsonConfigurationBody["icon"] = "mdi:counter";
        break; 
      case JuraMachineStateIcon::Alert:
        mqttJsonConfigurationBody["icon"] = "mdi:alert";
        break; 
      case JuraMachineStateIcon::Check:
        mqttJsonConfigurationBody["icon"] = "mdi:check";
        break; 
      case JuraMachineStateIcon::Thermometer:
        mqttJsonConfigurationBody["icon"] = "mdi:thermometer";
        break; 
      case JuraMachineStateIcon::Valve:
        mqttJsonConfigurationBody["icon"] = "mdi:valve";
        break; 
      case JuraMachineStateIcon::Speedometer:
        mqttJsonConfigurationBody["icon"] = "mdi:speedometer";
        break;
      case JuraMachineStateIcon::Function:
        mqttJsonConfigurationBody["icon"] = "mdi:function";
        break;
    }

    /* varying device by element type for easier viewing and handling  */
    switch(JuraEntityConfigurations[i].machineSubsystemType){
      case JuraMachineSubsystem::Thermoblock:
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   THERMOBLOCK_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             THERMOBLOCK_NAME;
        break;
      case JuraMachineSubsystem::Brewgroup:
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   BREW_GROUP_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             BREW_GROUP_NAME;
        break;
      case JuraMachineSubsystem::Water:
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   WATER_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             WATER_NAME;
        break;
      case JuraMachineSubsystem::Dosing:
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   DOSING_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             DOSING_NAME;
        break;
      case JuraMachineSubsystem::Milksystem: 
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   MILKSYSTEM_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             MILKSYSTEM_NAME;
        break;
      case JuraMachineSubsystem::Controller: 
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   CONTROLLER_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             CONTROLLER_NAME;
        break;
      case JuraMachineSubsystem::Bridge: 
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   BRIDGE_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             BRIDGE_NAME;
        break;
    }

    /* static device - constant throughout each  */
    mqttJsonConfigurationBody["device"]["model"] =            MODEL_NUM;
    mqttJsonConfigurationBody["device"]["manufacturer"] =     MANUFACTURER;
    mqttJsonConfigurationBody["device"]["sw_version"] =       VERSION_STR;

    /* publish this device */
    size_t n = serializeJson(mqttJsonConfigurationBody, buffer);
    ESP_LOGI(TAG,"[enabled]: %s",config_topic);
    xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );
    mqttClient.publish(config_topic, buffer, n);
    xSemaphoreGive(xMQTTSemaphore);

    /* meter the mqtt sending */
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

/***************************************************************************//**
 * Publish to mqtt broker all the machine buttons and functions that can be triggered
 * via mqtt
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/

void JuraBridge::publishMachineFunctionConfiguration(){

  /* iterate through state attributes */
  int functionEntityArraySize = sizeof(JuraMachineFunctionEntityConfigurations) / sizeof(JuraMachineFunctionEntityConfigurations[0]) ; 

  /* operaitonal state topic */
  char operational_state_topic[56]; 
  sprintf(
    operational_state_topic, 
    MQTT_ROOT "/%i/%i/%i", 
    JuraEntityConfigurations[0].machineSubsystemType, 
    JuraEntityConfigurations[0].subsystemAttributeType,
    0
  );

  /* iterate through all of the machine state attributes */
  for (int i = 0; i < functionEntityArraySize; i++){

    /* mqtt configuration */
    char buffer[2048];  DynamicJsonDocument mqttJsonConfigurationBody(2048);
    char config_topic[255]; sprintf(config_topic, "homeassistant/button/%s/config",  JuraMachineFunctionEntityConfigurations[i].entity_id );
    char unique_id[255]; sprintf(unique_id, ENTITY_PREFIX "uid_%s", JuraMachineFunctionEntityConfigurations[i].entity_id );

    /* ------- clear if disabled ------- */
    if (JuraMachineFunctionEntityConfigurations[i].enabled == JuraEntityEnabled::No) {
      ESP_LOGI(TAG,"[disabled]: %s", config_topic);
      xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );
      mqttClient.publish(config_topic, ""); 
      xSemaphoreGive(xMQTTSemaphore);
      continue;
    }

    /* all device specific configuration functions */     
    mqttJsonConfigurationBody["name"] =  JuraMachineFunctionEntityConfigurations[i].name;
    mqttJsonConfigurationBody["unique_id"] = unique_id;
    mqttJsonConfigurationBody["command_topic"] =  JuraMachineFunctionEntityConfigurations[i].command_topic;

    /* availability topics */
    if ( i > 0 && JuraMachineFunctionEntityConfigurations[i].availabilityFollowsMachine == JuraEntityAvailabilityFollowsReadyState::Yes){
        mqttJsonConfigurationBody["availability"][0]["payload_available"] = "READY";
        mqttJsonConfigurationBody["availability"][0]["payload_not_available"] = "OFF";
        mqttJsonConfigurationBody["availability"][0]["topic"] = operational_state_topic;
    } 

    /* menu items */
    switch(JuraMachineFunctionEntityConfigurations[i].menu_command){
      case JuraMachineMenuCommand::Rinse:
        mqttJsonConfigurationBody["payload_press"] = "rinse";
        break;
      case JuraMachineMenuCommand::MRinse:
        mqttJsonConfigurationBody["payload_press"] = "mrinse";
        break;
      case JuraMachineMenuCommand::MClean:
        mqttJsonConfigurationBody["payload_press"] = "mclean";
        break;
      case JuraMachineMenuCommand::Clean:
        mqttJsonConfigurationBody["payload_press"] = "clean";
        break;
    }   

    /* translate icon enum into an mdi: string for json */  
    switch(JuraMachineFunctionEntityConfigurations[i].icon){
      case JuraMachineStateIcon::Coffee:
        mqttJsonConfigurationBody["icon"] = "mdi:coffee";
        break;
      case JuraMachineStateIcon::Counter:
        mqttJsonConfigurationBody["icon"] = "mdi:counter";
        break; 
      case JuraMachineStateIcon::Water:
        mqttJsonConfigurationBody["icon"] = "mdi:water";
        break; 
      case JuraMachineStateIcon::Alert:
        mqttJsonConfigurationBody["icon"] = "mdi:alert";
        break; 
      case JuraMachineStateIcon::Check:
        mqttJsonConfigurationBody["icon"] = "mdi:check";
        break; 
      case JuraMachineStateIcon::Thermometer:
        mqttJsonConfigurationBody["icon"] = "mdi:thermometer";
        break; 
      case JuraMachineStateIcon::Valve:
        mqttJsonConfigurationBody["icon"] = "mdi:valve";
        break; 
      case JuraMachineStateIcon::Speedometer:
        mqttJsonConfigurationBody["icon"] = "mdi:speedometer";
        break;
      case JuraMachineStateIcon::Function:
        mqttJsonConfigurationBody["icon"] = "mdi:function";
        break;
    }

    /* varying device by element type for easier viewing and handling  */
    switch(JuraMachineFunctionEntityConfigurations[i].machineSubsystemType){
      case JuraMachineSubsystem::Thermoblock:
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   THERMOBLOCK_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             THERMOBLOCK_NAME;
        break;
      case JuraMachineSubsystem::Brewgroup:
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   BREW_GROUP_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             BREW_GROUP_NAME;
        break;
      case JuraMachineSubsystem::Water:
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   WATER_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             WATER_NAME;
        break;
      case JuraMachineSubsystem::Dosing:
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   DOSING_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             DOSING_NAME;
        break;
      case JuraMachineSubsystem::Milksystem: 
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   MILKSYSTEM_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             MILKSYSTEM_NAME;
        break;
      case JuraMachineSubsystem::Controller: 
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   CONTROLLER_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             CONTROLLER_NAME;
        break;
      case JuraMachineSubsystem::Bridge: 
        mqttJsonConfigurationBody["device"]["identifiers"][0] =   BRIDGE_NAME;
        mqttJsonConfigurationBody["device"]["name"] =             BRIDGE_NAME;
        break;
    }

    /* device setup */
    mqttJsonConfigurationBody["device"]["model"] =            MODEL_NUM;
    mqttJsonConfigurationBody["device"]["manufacturer"] =     MANUFACTURER;
    mqttJsonConfigurationBody["device"]["sw_version"] =       VERSION_STR;

    /* publish this device */
    ESP_LOGI(TAG,"[enabled]: %s %s",config_topic, buffer);
    size_t n = serializeJson(mqttJsonConfigurationBody, buffer);
    xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );
    mqttClient.publish(config_topic, buffer, n);
    xSemaphoreGive(xMQTTSemaphore);

    /* meter the mqtt sending */
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

/***************************************************************************//**
 * Specialized mqtt publication for automatic rinsing 
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
 void JuraBridge::publishRinseRequest(){
    xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );
    mqttClient.publish(
    MQTT_ROOT MQTT_SUBTOPIC_MENU,
    "rinse");  
  xSemaphoreGive(xMQTTSemaphore);
 }

/***************************************************************************//**
 * Subscribe to machine function mqtt topics 
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
 void JuraBridge::subscribeToMachineFunctionButtonCommandTopics(){

 /* iterate through function entities  */
  int functionEntityArraySize = sizeof(JuraMachineFunctionEntityConfigurations) / sizeof(JuraMachineFunctionEntityConfigurations[0]) ; 

  /* iterate through all of the machine function attributes */
  for (int i = 0; i < functionEntityArraySize; i++){

    /* register subscription */
    
    mqttClient.subscribe(JuraMachineFunctionEntityConfigurations[i].command_topic);
    

    /* meter the mqtt sending */
    vTaskDelay(pdMS_TO_TICKS(150));
  }
}

/***************************************************************************//**
 * Subscribe to all topics specific to bridge 
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
void JuraBridge::subscribeToBridgeSubtopics(){
  /* manually set, but configurable in JuraConfiguration.h */
  
  mqttClient.subscribe(MQTT_ROOT MQTT_SUBTOPIC_MENU);
  mqttClient.subscribe(MQTT_ROOT MQTT_BRIDGE_RESTART);
  mqttClient.subscribe(MQTT_ROOT MQTT_CONFIG_SEND);
  mqttClient.subscribe(MQTT_ROOT MQTT_DISPENSE_CONFIG);  
}

/***************************************************************************//**
 * Special handler to print ready state to Jura display; based on VERSION_MAJOR_STR macro
 *
 * @param[out] null 
 *     
 * @param[in] null
 ******************************************************************************/
void JuraBridge::instructServicePortToSetReady(){servicePort.transferEncode("DT: READY v" VERSION_MAJOR_STR);}

/***************************************************************************//**
 * Special handler to print custom message to display; bad characters ignored by machine; 
 * TODO: sanitize input
 *
 * @param[out] null 
 *     
 * @param[in] command String 10 characters or fewer to display 
 ******************************************************************************/
void JuraBridge::instructServicePortToDisplayString(String command) {servicePort.transferEncode("DT:" + command);}

/***************************************************************************//**
 * Special handler to perform specific known command call via UART to machine
 *
 * @param[out] null 
 *     
 * @param[in] command JuraServicePortCommand enum command
 ******************************************************************************/
void JuraBridge::instructServicePortWithCommand(JuraServicePortCommand command) {servicePort.transferEncodeCommand(command);}

/***************************************************************************//**
 * Special handler to perform specific known function via UART to machine
 *
 * @param[out] null 
 *     
 * @param[in] command JuraFunctionIdentifier
 ******************************************************************************/
void JuraBridge::instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier identifier) {
  int functionEntityArraySize = sizeof(JuraMachineFunctionEntityConfigurations) / sizeof(JuraMachineFunctionEntityConfigurations[0]) ; 
  for (int i = 0; i < functionEntityArraySize; i++){
    if (identifier == JuraMachineFunctionEntityConfigurations[i].function){
      ESP_LOGI(TAG, "-> Select Function: %s",JuraMachineFunctionEntityConfigurations[i].name );
      servicePort.transferEncodeCommand(JuraMachineFunctionEntityConfigurations[i].command);
      break;
    }
  }
}

/***************************************************************************//**
 * Special handler to determine whether a particular string state has changed. 
 * string state change is determiened via numerical comparison to the integer
 * representation of that state to save on string storage and comparison. 
 * If string state is determined to be different, report via mqtt. 
 *
 * @param[out] bool changed yes or no 
 *     
 * @param[in] JuraMachineStateIdentifier machineStateIdentifier
 * @param[in] const char * _newStringState 
 * @param[in] int _intState
 ******************************************************************************/
bool JuraBridge::machineStateStringChanged(JuraMachineStateIdentifier machineStateIdentifier, const char * _newStringState, int _intState) {

  /* find index (don't rely on pairty beteween state enum and the state array ) */
  int entityConfigurationIndex =  _stateIndexToConfigurationIndex[(int) machineStateIdentifier];
 
  /* compare int states, print and enqueue string values */
  if (JuraEntityConfigurations[entityConfigurationIndex].enabled == JuraEntityEnabled::Yes){
    
    /* todo: compare to old string state? [no - save memory] */
    int _oldstate = _reportableNumericStates[entityConfigurationIndex];

    if (_oldstate != _intState ){

      /* report integer represtntation of the staet; report the string*/
      _reportableNumericStates[entityConfigurationIndex] = _intState; 

      /* record state for bulk reporting */
      char message_topic[56]; 
      sprintf(
        message_topic, 
        MQTT_ROOT "/%i/%i/%i", 
        JuraEntityConfigurations[entityConfigurationIndex].machineSubsystemType, 
        JuraEntityConfigurations[entityConfigurationIndex].subsystemAttributeType,
        entityConfigurationIndex
      );

      /* report out */
      xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );
      mqttClient.publish(message_topic, _newStringState);
      xSemaphoreGive(xMQTTSemaphore);

      /* mark as bridge processing completed only if this is not the first default value */
      if (_intState != DEFAULT_NUMERIC_STATE){
        return true; 
      }
    }
      
    /* printing for debug*/
    if (PRINT_KNOWN_VALUES && (JuraEntityConfigurations[entityConfigurationIndex].serialPrintable == JuraEntitySerialPrintable::Yes) ) { 
      ESP_LOGI(TAG, "[%i] %s: %s", machineStateIdentifier, JuraEntityConfigurations[entityConfigurationIndex].name, _newStringState);
    }
  }
  return false;
}

/***************************************************************************//**
 * Special handler to determine whether a particular int state has changed. 
 * State change is determiened via numerical comparison to the integer
 * representation of the input state. 
 * If int state is determined to be different, report via mqtt. 
 *
 * @param[out] bool changed yes or no 
 *     
 * @param[in] JuraMachineStateIdentifier machineStateIdentifier
 * @param[in] int _newState
 ******************************************************************************/
 bool JuraBridge::machineStateChanged(JuraMachineStateIdentifier state, int _newState) {
  
  /* find index (don't rely on pairty beteween state enum and the state array ) */
  int entityConfigurationIndex = _stateIndexToConfigurationIndex[(int)state];
 
  /* report and record only if needed */
  if (JuraEntityConfigurations[entityConfigurationIndex].enabled == JuraEntityEnabled::Yes){

    /* store all states as integers */
    int _oldstate = _reportableNumericStates[entityConfigurationIndex];
    
      /* compare to old value */
    if (_oldstate != _newState){

      char mqttFloatToString[8]; 
      dtostrf(_newState, 1, 0, mqttFloatToString);

      /* record state for bulk reporting */
      char message_topic[56]; 
      sprintf(
        message_topic, 
        MQTT_ROOT "/%i/%i/%i", 
        JuraEntityConfigurations[entityConfigurationIndex].machineSubsystemType, 
        JuraEntityConfigurations[entityConfigurationIndex].subsystemAttributeType,
        entityConfigurationIndex
      );

      /* report the string version */
      xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );
      mqttClient.publish(message_topic, mqttFloatToString);
      xSemaphoreGive(xMQTTSemaphore);

      /* record the report*/
      _reportableNumericStates[entityConfigurationIndex] = _newState;

       /* save pref only if device is enabeld? */
      if (JuraEntityConfigurations[entityConfigurationIndex].nonvolatile == JuraEntityNonvolatile::Yes){
        char prefKey[8]; dtostrf((int)JuraEntityConfigurations[entityConfigurationIndex].state, 4, 0, prefKey);
        preferences.putInt(prefKey, _newState);
      }

      /* return true only if state has changed */
      return true; 
    }

    /* printing for debug*/
    if (PRINT_KNOWN_VALUES && (JuraEntityConfigurations[entityConfigurationIndex].serialPrintable == JuraEntitySerialPrintable::Yes) ) { 
      if (JuraEntityConfigurations[entityConfigurationIndex].dataType == JuraMachineStateDataType::Boolean){       
        ESP_LOGI(TAG, "[%i] %s: %s", state, JuraEntityConfigurations[entityConfigurationIndex].name, (bool) _newState ? "YES" : "NO");
      }else{
        ESP_LOGI(TAG, "[%i] %s: %i", state, JuraEntityConfigurations[entityConfigurationIndex].name, _newState);
      }
    }
  }
  return false; 
}

