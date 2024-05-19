/* external library requirements */
#include "Preferences.h"
#include "PubSubClient.h"
#include "ArduinoJson.h"
#include <WiFi.h>
#include <string>

/* project requirements */
#include "Secrets.h"
#include "Version.h"
#include "JuraMachine.h"
#include "JuraBridge.h"
#include "JuraCustomMenu.h"

/* macros */
#define xSemaphoreWrappedSetBoolean(x,y,z)  xSemaphoreTake(x, portMAX_DELAY ); y = z; xSemaphoreGive(x);
#define vTaskDelayMilliseconds(x)  vTaskDelay(pdMS_TO_TICKS(x));

/* nvs preferences handler */
Preferences prefs;

/* need to stop long-run tasks when MQTT signaling is required */
SemaphoreHandle_t xUARTSemaphore; 

/* need to ensure that different tasks do not call the mqtt client at the same time; */
SemaphoreHandle_t xMQTTSemaphore; 
SemaphoreHandle_t xMQTTStatusSemaphore; 
SemaphoreHandle_t xWIFIStatusSemaphore; 
SemaphoreHandle_t xMachineReadyStateVariableSemaphore; 

/* define comms */
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

/* define machine instance */
JuraBridge bridge(prefs, mqttClient, xMQTTSemaphore, xUARTSemaphore);
JuraMachine machine(bridge, xMachineReadyStateVariableSemaphore);

/* task handles */
TaskHandle_t xUART, xLED;

/* queue */
QueueHandle_t xQ_SubscriptionMessage; // payload and topic queue of MQTT payload and topic
const int payloadSize = 512;
struct mqtt_message {
  char payload [payloadSize] = {'\0'};
  String topic ;
} rx_message;

/* protected state variables */
volatile bool connectedToBroker;
volatile bool connectedToNetwork;

/* button */
struct Button {
	bool released;
  bool isBeingHeld;
  unsigned long startat;
  unsigned long endat;
};

/* custom menu object */
struct JuraCustomMenu {
	unsigned int item;
	bool active;
  unsigned long time;
};

volatile Button menuButton = {false, false, 0, 0};
volatile JuraCustomMenu customMenu = {0, false, 0};

//variables to keep track of the timing of recent interrupts

/***************************************************************************//**
 * Loop task to iteratively poll machine via UART
 *
 * @param[out] null 
 *     
 * @param[in] pvParameters required for callback
 ******************************************************************************/
void machineStatePollingHandler (void * pvParameters){

  /* await broker connection */
  while (!connectedToBroker){
    vTaskDelayMilliseconds(500);
  }
  
  /* get innitial properties before polling starts y! */
  initJuraEntityStatesFromNonVolatileStorage();


  int loopIterator = 1; 
  for(;;){ 
    /*

      00-0F :   8400 0023 D4D4 487C 0103 0000 0011 0000 0A3A 259B 064F D09D 719F 2DCF 01FF 3BB6
      10-1F :   E2E2 E2E2 E2E2 D5D5 D5D5 D5D5 D5D5 D5D5 D5D5 D5D5 D5D5 D5D5 D5D5 D5D5 D5D5 D5D5 <- last D5D5s all appear to change for button presses on the rotary
        Center Button: N - 4
        Right: N - 1?
        Left:  N + 1?

      20-2F :   0000 0000 0000 0000 40EB 7224 000A 0C72 7218 C2F5 74F3 FFFF 0000 0000 0000 0000
      30-3F :   0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 
      40-4F :   0000 0707 0200 0023 9201 9C00 07D9 00CA 0408 0001 0001 1600 EB7C 0450 C272 2C6A
      50-5F :   B70E 0752 C1C2 FD00 FB0F AB72 046E 0AFE F908 730D CF75 012C 0080 0404 F2FE D108
      60-6F :   126E 821A 030C 0906 04F2 F500 3558 00AD 73FA 245D FFC0 1FC9 6A10 0072 2308 0FB0
      70-7F :   FD07 4A02 0642 FF05 4042 5A15 00AF 0008 2CB5 D800 6E29 FD00 4764 00EB D800 5BBE
      80-8F :   6E48 7505 03FF E314 7504 D923 45A1 0762 0020 F5FE 01F9 47B5 FEFD 0BFF FFFF 6A02
      90-9F :   FD00 7EE4 045A AEFE 0173 9F7E CF75 FF0A C2E2 6A1A CB75 F7E2 7306 FDDD 0020 7303
      A0-AF :   B23C FEC1 73F3 400B 7309 046E 046A 6E73 00E2 066E 0AE7 B708 F272 03D8 507D FFBE
      B0-BF :   D84E 7C00 4B75 C175 2746 086C 0BBF 6E04 F500 3273 75E5 C900 BF7E 7D00 FE8B FFFF
      C0-CF :   40EB 0053 3C50 2CF2 5D00 F502 4CEA FDB1 FD0B 76F2 FD00 0BF3 0173 F507 F500 F36E
      D0-DF :   4003 03F5 ACCE 4CF8 5909 07C8 7300 B0A1 00AA 00DD 0564 7407 0B1D 7E24 3A5C C1F2
      E0-EF :   68FF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF
      F0-FF :   FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF
    */

    if (!customMenu.active){
      machine.handlePoll(loopIterator);
      loopIterator = (loopIterator % 100) + 1; 
      vTaskDelayMilliseconds(10);
    }else{
      vTaskDelayMilliseconds(500);
    }
  }
}


/***************************************************************************//**
 * Await the machine ready state then add a shot 
 *
 * @param[out] null 
 *     
 * @param[in] addShot int number of shots
 * @param[in] brewLimt int limit for brew out
 ******************************************************************************/
void awaitDispenseCompletionToAddShots( int addShot = 1, int brewLimit = 17 ){

  /* precharge the wait display for between elements */
  bridge.instructServicePortToDisplayString("   WAIT   ");

  /* iterating over two shots */
  for( int shot = 0; shot < addShot; shot ++){

    /* define a non-ready state that is not processed in the machine poller; protect  */
    machine.startAddShotPreparation();
    /* 
      NOTE TO READER: intentionally leave the semaphore taken here to prevent a ready state from happen
      until we get into the while loop below; 
    */

    /* entire the loop before returning the semaphore */
    xSemaphoreTake( xMachineReadyStateVariableSemaphore, portMAX_DELAY );
    
    bool canGiveSemaphore = true;
    while (machine.states[(int) JuraMachineStateIdentifier::SystemIsReady] == false){
      vTaskDelayMilliseconds(500);
      if (canGiveSemaphore){
        xSemaphoreGive( xMachineReadyStateVariableSemaphore);
        canGiveSemaphore = false;
      }
    }

    /* between different shots - resting period */              
    vTaskDelayMilliseconds(2500);

    /* set brew limit here */
    machine.setDispenseLimit(brewLimit, JuraMachineDispenseLimitType::Brew) ;

    /* send the command for this repetition */
    bridge.instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier::MakeEspresso);

    /* resting period */              
    vTaskDelayMilliseconds(2500);
  }
}

/***************************************************************************//**
 * Process singe-item queue here so as to offload processing of mqtt
 * message content from the mqtt callback itself.
 * 
 * notes: design pattern based on @idahowalker on arduino.cc forums
 *
 * @param[out] null 
 *     
 * @param[in] pvParameters required for callback
 ******************************************************************************/
void receivedMQTTMessageQueueWorker( void *pvParameters ){
  /* processing message px_message differs from received message rx_message */
  struct mqtt_message px_message;
  for (;;)
  {
    /* wait until the message queue includes something; */
    if ( xQueueReceive(xQ_SubscriptionMessage, &px_message, portMAX_DELAY) == pdTRUE ){
      String mqttMessageString = String(px_message.payload);
      String topic = String(px_message.topic);
      int brewLimit = 0;
      int addShot = 0;
      int milkLimit = 0; 
      int waterLimit = 0;

      /* was this a json string?  */
      if (mqttMessageString.length() > 0){

        /* define static; will this leak memory? */
        StaticJsonDocument<1024> receivedJson ;
        DeserializationError json_deserialization_error = deserializeJson(receivedJson, (const char*) mqttMessageString.c_str());
        
        /* reset all */
        machine.resetDispenseLimits();
        
        /* if the message aws JSOn and included properly formatted dictionary, then set the limits; reset on failure?*/
        if (! json_deserialization_error) {
          JsonObject dictObjct = receivedJson.as<JsonObject>();
          if (dictObjct.containsKey("milk")){
            machine.setDispenseLimit( (int) dictObjct["milk"],JuraMachineDispenseLimitType::Milk) ;
            milkLimit =  (int) dictObjct["milk"];
          }
          if (dictObjct.containsKey("brew")){
            machine.setDispenseLimit( (int) dictObjct["brew"],JuraMachineDispenseLimitType::Brew) ;
            brewLimit = (int) dictObjct["brew"];
          }
          if (dictObjct.containsKey("water")){
            machine.setDispenseLimit( (int) dictObjct["water"],JuraMachineDispenseLimitType::Water) ;
            waterLimit =  (int) dictObjct["water"];
          }
          if (dictObjct.containsKey("add")){
            addShot = (int) dictObjct["add"];
            addShot = addShot > 2 ? 2 : addShot < 0 ? 0 : addShot;
          } 
          /* is this a straigh up dispense config? */
          if (topic == MQTT_ROOT MQTT_DISPENSE_CONFIG) {
            bridge.instructServicePortToDisplayString(" PRODUCT?");
            
            /* if addshot is set, we need to wait for non-*/
            if (addShot > 0){

              /* entire the loop before returning the semaphore */
              bool canGiveSemaphore = true;
              while (machine.states[(int) JuraMachineStateIdentifier::SystemIsReady] == true){
                vTaskDelayMilliseconds(500);
                if (canGiveSemaphore){
                  canGiveSemaphore = false;
                }
              }

              /* define a non-ready state that is not processed in the machine poller; protect  */
              machine.startAddShotPreparation();

              /* instruct add shots */
              awaitDispenseCompletionToAddShots(addShot, brewLimit);

              /* return to ready*/
              bridge.instructServicePortToSetReady();

            }else{
              ESP_LOGI(TAG,"Custom limits set: Water: %i ml Milk: %i ml Brew: %i ml", waterLimit, milkLimit, brewLimit);
              continue;
            }
          }
        }
      }

      /* --- First, process through known mqtt subscriptions  ---  */
      int functionEntityArraySize = sizeof(JuraMachineFunctionEntityConfigurations) / sizeof(JuraMachineFunctionEntityConfigurations[0]) ; 
      for (int i = 0; i < functionEntityArraySize; i++){

        /* find the topic among prefab functions?  */
        if (topic == JuraMachineFunctionEntityConfigurations[i].command_topic && 
            JuraMachineFunctionEntityConfigurations[i].command != JuraServicePortCommand::None){

          /* instruct the custom command */          
          bridge.instructServicePortWithCommand(JuraMachineFunctionEntityConfigurations[i].command);

          /* dealy */
          vTaskDelay(500 / portTICK_PERIOD_MS);

          /* add shot ? */
          if (addShot > 0) {
            
            /* debug */
            ESP_LOGI(TAG,"--> Machine Add Shot Function: %s", JuraMachineFunctionEntityConfigurations[i].name);
            awaitDispenseCompletionToAddShots(addShot, brewLimit);
          }

          /* set ready */
          bridge.instructServicePortToSetReady();
          break;
        }
      }

      /* --- machine topics and bridge topics  ---  */
      if (topic ==  MQTT_ROOT MQTT_SUBTOPIC_MENU) {

        /* enter settings menu */
        bridge.instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier::SettingsMenu);
        vTaskDelayMilliseconds(300);

        /* select first menu item */
        bridge.instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier::SelectMenuItem);
        vTaskDelayMilliseconds(300);

        //how far into the menu do we go?
        int menu_item = 99;

        if      (mqttMessageString ==  "mclean")  { menu_item = 0;}
        else if (mqttMessageString ==  "rinse")   { menu_item = 1;}
        else if (mqttMessageString ==  "mrinse")  { menu_item = 2;}
        else if (mqttMessageString ==  "clean")   { menu_item = 3;}
        else if (mqttMessageString ==  "filter")  { menu_item = 4;}

        /* correct menu! */
        if (menu_item < 5){
          //navigate to message 
          for (int i = 0; i < menu_item; i++){
            bridge.instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier::RotaryRight);
            vTaskDelayMilliseconds(300);
          }

          //press the selected button and be done with it
          bridge.instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier::SelectMenuItem);
          vTaskDelayMilliseconds(300);
        }

      } else if (topic == MQTT_ROOT MQTT_BRIDGE_RESTART) {
        ESP.restart();

      } else if (topic == MQTT_ROOT MQTT_CONFIG_SEND) {
        bridge.instructServicePortToDisplayString("   WAIT");
        bridge.publishMachineEntityConfigurations();
        bridge.publishMachineFunctionConfiguration(); 

        /* set ready */
        bridge.instructServicePortToSetReady();

      } else if (topic == HA_STATUS_MQTT) {
        if (mqttMessageString == "online"){
          ESP.restart();
        }
      }
    }
  }
}

/***************************************************************************//**
 * RAM held dumping and debug spot for Wifi events. Insert switch 
 * for debugging.
 * 
 * notes: design pattern based on @idahowalker on arduino.cc forums
 *
 * @param[out] null 
 *     
 * @param[in] WiFiEvent_t wifi event for preocessing
 ******************************************************************************/
void IRAM_ATTR WiFiEvent(WiFiEvent_t event){}

/***************************************************************************//**
 * RAM held mqtt rx event processor; dump received messages into 
 * single-item queue for processing at a later time. Structure message
 * into a single object that can be added into the xQueue
 * 
 * notes: design pattern based on @idahowalker on arduino.cc forums
 *
 * @param[out] null 
 *     
 * @param[in] topic char array pointer to topic of the rx message
 * @param[in] payload byte array pointer to payload of the rx message 
 * @param[in] length unsigned int length of the payload
 ******************************************************************************/
void IRAM_ATTR mqttCallback(char* topic, byte * payload, unsigned int length){
  memset( rx_message.payload, '\0', payloadSize ); // clear payload char buffer
  rx_message.topic = ""; //clear topic string buffer
  rx_message.topic = topic; //store new topic
  int i = 0; // extract payload
  for ( i; i < length; i++)
  {
    rx_message.payload[i] = (char)payload[i];
  }
  rx_message.payload[i] = '\0';
  xQueueOverwrite( xQ_SubscriptionMessage, (void *) &rx_message );// send data to queue
} 

/***************************************************************************//**
 * Second function for processing menu items with MQTT handler, but without 
 * requireing a bounce-0ff of the broker ( wifi disconnect is a problem... )
 *
 * @param[out] null 
 *     
 * @param[in] topic const char array pointer to topic of the rx message
 * @param[in] payload const char array pointer to payload of the rx message 
 * @param[in] length unsigned int length of the payload
 ******************************************************************************/
void IRAM_ATTR mqttCallbackBypassBroker(const char* topic, const char* payload, unsigned int length){
  memset( rx_message.payload, '\0', payloadSize ); 
  rx_message.topic = ""; 
  rx_message.topic = topic; 
  int i = 0; 
  for ( i; i < length; i++)
  {
    rx_message.payload[i] = (char)payload[i];
  }
  rx_message.payload[i] = '\0';
  xQueueOverwrite( xQ_SubscriptionMessage, (void *) &rx_message );// send data to queue
} 

/***************************************************************************//**
 * Publish device configuration to Home Assistant via configured broker. 
 *
 * @param[out] none 
 *     
 * @param[in] none 
 ******************************************************************************/
void refreshConfigurationWithBroker(){
  /* if the version number has changed, update the MQTT reports */
  if (prefs.getInt("version", 0) != VERSION_INT){
    bridge.instructServicePortToDisplayString(" UPDATING");
    bridge.publishMachineEntityConfigurations();
    bridge.publishMachineFunctionConfiguration(); 

    /* version stored*/
    prefs.putInt("version", VERSION_INT);
  }
}

/***************************************************************************//**
 * Update subscriptions for bridge events and device specific events.
 *
 * @param[out] none 
 *     
 * @param[in] none 
 ******************************************************************************/
void refreshSubscriptionsWithBroker(){
  bridge.subscribeToMachineFunctionButtonCommandTopics();
  bridge.subscribeToBridgeSubtopics();
}

/***************************************************************************//**
 * Connect to the mqtt broker. Called from the connection manager
 * loop. Once connected to mqtt, perform initial mqtt 
 *
 * @param[out] none 
 *     
 * @param[in] none 
 ******************************************************************************/
void connectToBroker(){

  /* semaphore-protected */
  bridge.instructServicePortToDisplayString("   MQTT   ");

  /* operaitonal state topic */
  char operational_state_topic[56]; 
  sprintf(
    operational_state_topic, 
    MQTT_ROOT "/%i/%i/%i", 
    JuraEntityConfigurations[0].machineSubsystemType, 
    JuraEntityConfigurations[0].subsystemAttributeType,
    0
  );

  /* set keepalive before connection attempt; design pattern from idahowalker on arduino forums */
  mqttClient.setKeepAlive( 90 );
  mqttClient.setServer(MQTTBROKER, 1883);
  mqttClient.setBufferSize(2048);       /* for receiving large recipes; discontinyed; not necessary? */

  while (! mqttClient.connected()){
    mqttClient.connect(TAG, MQTT_USER, MQTT_PASS, operational_state_topic, 0, false, "OFF" , true);
    vTaskDelayMilliseconds(2500);
  }

  mqttClient.setCallback( mqttCallback );

  /* subscriptions and mqtt first stuffs refreshing goes here */
  xSemaphoreWrappedSetBoolean(xMQTTStatusSemaphore, connectedToBroker, true);   
} 

/***************************************************************************//**
 * Connect to configured Wi-Fi network.
 *
 * @param[out] none 
 *     
 * @param[in] none 
 ******************************************************************************/
void connectToWiFiNetwork(){

  /* booting message on display */
  bridge.instructServicePortToDisplayString("   WIFI   ");

  /* configuration values*/
  WiFi.mode(WIFI_STA); 
  WiFi.persistent(true);

  int wifiConnectionAttempt = 0;
  while (WiFi.status() != WL_CONNECTED){
    wifiConnectionAttempt++;
    WiFi.disconnect();
    WiFi.begin( WIFINAME, WIFIPASS );
    vTaskDelayMilliseconds(5000);
    if ( wifiConnectionAttempt == 10 )
    {
      ESP.restart();
    }
  }
  WiFi.onEvent( WiFiEvent );
  
  /* print IP address for debugging - send to frontend?*/
  IPAddress localIP = WiFi.localIP();
  ESP_LOGI(TAG,"Wi-Fi: %i.%i.%i.%i",localIP[0], localIP[1],localIP[2],localIP[3]);
  xSemaphoreWrappedSetBoolean(xWIFIStatusSemaphore, connectedToNetwork, true);   
} 

/***************************************************************************//**
 * Task to maintain MQTT and Wi-Fi communicationns.
 *
 * @param[out] none 
 *     
 * @param[in] pvParameters required for callback
 ******************************************************************************/
void communicationsKeepAliveTask( void *pvParameters ){
  mqttClient.setKeepAlive(90);
  for (;;){
    if ( (wifiClient.connected()) && (WiFi.status() == WL_CONNECTED) && (mqttClient.connected()) ){
      xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );
      mqttClient.loop();
      xSemaphoreGive( xMQTTSemaphore );
    
    }else {
      /* mqtt cannot be available if wifi is not available */
      xSemaphoreWrappedSetBoolean(xMQTTStatusSemaphore, connectedToBroker, false);   
      xSemaphoreWrappedSetBoolean(xWIFIStatusSemaphore, connectedToNetwork, false);   

      if ( !(wifiClient.connected()) || !(WiFi.status() == WL_CONNECTED)){
        connectToWiFiNetwork();
      }

      connectToBroker();
      refreshSubscriptionsWithBroker();
      refreshConfigurationWithBroker();

      /* set ready */
      bridge.instructServicePortToSetReady();
    }
    vTaskDelayMilliseconds( 250);
  }

  /* null = calling task is deleted; should never exit this loop anyway...*/
  vTaskDelete ( NULL );
}

/***************************************************************************//**
 * Init states based on stored memory. 
 *
 * @param[out] none 
 *     
 * @param[in] none
 ******************************************************************************/
void initJuraEntityStatesFromNonVolatileStorage() {
  
  /* for debugging states */
  if (DISABLE_NONVOLATILE_LOAD){return;}

  /* iterate through state attributes */
  int stateAttributeArraySize = sizeof(JuraEntityConfigurations) / sizeof(JuraEntityConfigurations[0]) ; 

  /* iterate through all of the machine state attributes -- skipping the meta */
  for (int entityConfigurationIndex = 1; entityConfigurationIndex < stateAttributeArraySize; entityConfigurationIndex++){
    
    if (JuraEntityConfigurations[entityConfigurationIndex].nonvolatile == JuraEntityNonvolatile::Yes) {

      /* set instance vars for machine class */
      char prefKey[8]; dtostrf((int) JuraEntityConfigurations[entityConfigurationIndex].state, 4, 0, prefKey);

      /* set from preference */
      machine.states[(int) JuraEntityConfigurations[entityConfigurationIndex].state] = prefs.getInt(prefKey, 0);
      
      /* notify bridge of change */
      bridge.machineStateChanged(JuraEntityConfigurations[entityConfigurationIndex].state, machine.states[ (int) JuraEntityConfigurations[entityConfigurationIndex].state]);
      vTaskDelay(100 / portTICK_PERIOD_MS);

    } else if ( (JuraEntityConfigurations[entityConfigurationIndex].dataType == JuraMachineStateDataType::Integer) ||
                (JuraEntityConfigurations[entityConfigurationIndex].dataType == JuraMachineStateDataType::Boolean) ) {

      machine.states[(int) JuraEntityConfigurations[entityConfigurationIndex].state] = JuraEntityConfigurations[entityConfigurationIndex].defaultValue;

      /* notify bridge of change */
      bridge.machineStateChanged(JuraEntityConfigurations[entityConfigurationIndex].state, machine.states[ (int) JuraEntityConfigurations[entityConfigurationIndex].state]);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    } 
  }
}

/***************************************************************************//**
 * Flash onboard LED at different rates based on connection status. 
 *
 * @param[out] none 
 *     
 * @param[in] none
 ******************************************************************************/
void statusLEDBlinker(void * parameter){
  int blinkrate = 250; int flashnum = 3; int waitfor = 3000;
  digitalWrite(DEV_BOARD_LED_PIN, HIGH);
  vTaskDelayMilliseconds(1000);
  for(;;){ 

    /* signal status light differently based on communication variables */
    if ( connectedToBroker ){
      blinkrate = 750;
      flashnum = 4;
      waitfor = 0;
   
    }else if (connectedToNetwork){
      blinkrate = 450;
      flashnum = 2;
      waitfor = 1200;
    
    } else {
      blinkrate = 240;
      flashnum = 6;
      waitfor = 500;
    }

    /* flash sequencer */
    for (int i = 0; i < flashnum; i++){
      digitalWrite(DEV_BOARD_LED_PIN, HIGH);
      vTaskDelayMilliseconds(blinkrate);
      digitalWrite(DEV_BOARD_LED_PIN, LOW);
      vTaskDelayMilliseconds(blinkrate);
    }

    /* pause between sequemces */
    if (waitfor > 0){vTaskDelayMilliseconds(waitfor);}
  }
}

/***************************************************************************//**
 * Button ISR handler
 *
 * @param[out] none 
 *     
 * @param[in] none
 ******************************************************************************/
volatile int buttonChangeTime;
#define DEBOUNCE 300
void IRAM_ATTR buttonInterruptSwitchPressRoutine() {
  if (digitalRead(DEV_BOARD_BUTTON_PIN) == HIGH && menuButton.released == false){
    buttonChangeTime = millis();
    if (buttonChangeTime - menuButton.endat > DEBOUNCE){
      menuButton.released = true;
      menuButton.endat = buttonChangeTime;
    }
  }else if (menuButton.startat == 0){
    menuButton.startat = millis();
    menuButton.endat = 0;
  }
 
}

/***************************************************************************//**
 * Flash onboard LED at different rates based on connection status. 
 *
 * @param[out] none 
 *     
 * @param[in] none
 ******************************************************************************/
void customMenuHandler(void * parameter){
  int menuItems = sizeof(JuraCustomMenuItemConfigurations) / sizeof(JuraCustomMenuItemConfigurations[0]) ; 
  bool canceled = false; 

  for(;;){
    /* menu active or not? */
    bool userMadeSelectionByHoldingButton = false;

    /* has the hardware button been pressed ?? */
    if (menuButton.released) {
      /* reset button immediately */
      menuButton.released = false;
      menuButton.isBeingHeld = false;

      /* how long was it pressed ?*/
      int duration = menuButton.endat - menuButton.startat;
      menuButton.startat = 0;

      /* button held */
      if (duration > BUTTON_HOLD_DURATION_MS && !canceled ){
        userMadeSelectionByHoldingButton = true;
      }
      canceled = false; 

      /* menu is active, lets change the menu item!*/
      if (!userMadeSelectionByHoldingButton && customMenu.active){
          /* iterate the selected menu */
          customMenu.item = (customMenu.item % menuItems) + 1; 
          customMenu.time = millis();

          /* set menu name; zero indexed but iterator is 1 indexed */
          bridge.instructServicePortToDisplayString(JuraCustomMenuItemConfigurations[customMenu.item - 1].name);

      /* menu active, and user held button*/
      }else if (userMadeSelectionByHoldingButton && customMenu.active){

        /* if the currently-selected item is less than the last item, trigger the appropriate menu item mqtt */
        if (customMenu.item < menuItems){
          bridge.instructServicePortToDisplayString("   WAIT");

          /* send mqtt message corresponding to selected emnu item */
          xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );

          /* trigger internal mqtt processor for consistently; do not need to bounce from broker */
          mqttCallbackBypassBroker(
            JuraCustomMenuItemConfigurations[customMenu.item - 1].topic,
            JuraCustomMenuItemConfigurations[customMenu.item - 1].payload, 
            strlen(JuraCustomMenuItemConfigurations[customMenu.item - 1].payload) + 1
          );

          /*
          mqttClient.publish(
            JuraCustomMenuItemConfigurations[customMenu.item - 1].topic,
            JuraCustomMenuItemConfigurations[customMenu.item - 1].payload 
          );*/  
          xSemaphoreGive(xMQTTSemaphore);

        }else{
          /* so the ready prompt doesn't show immediately */
          vTaskDelay(750 / portTICK_PERIOD_MS);

          /* if we are at the last item, set ready */
          bridge.instructServicePortToSetReady();
        }

      /* menu is not avtive, but user pressed the button */
      } else if (! customMenu.active ){
        /* start the menu off regardless whether it's being held or not */
        bridge.instructServicePortToDisplayString(JuraCustomMenuItemConfigurations[0].name);
        customMenu.item = 1;
        customMenu.active = true;
        customMenu.time = millis();

        /* cancel button press right here to show the first menu item */
        menuButton.endat = 0;
        menuButton.startat = 0;
        menuButton.released = false;
        menuButton.isBeingHeld = false;
        canceled = false; 

        /* prevent button from being immediately pressed again */
        vTaskDelay(500 / portTICK_PERIOD_MS);
        continue; 
      }  
    
    /* if the hardware button has not been released yet, is it currently being pressed while the menu is open? */
    }else if (customMenu.active) {
      /* if startat is nonzero, then the button is currently being pressed*/
      if (menuButton.startat != 0){
        if (millis() - menuButton.startat > 800 && !menuButton.isBeingHeld){
          menuButton.isBeingHeld = true; 
          bridge.instructServicePortToDisplayString("    OK");
          vTaskDelay(250 / portTICK_PERIOD_MS);
        
        }else if (millis() - menuButton.startat > 2500 && menuButton.isBeingHeld && !canceled){
          canceled = true;
          bridge.instructServicePortToDisplayString("  CANCEL");
          customMenu.item = (customMenu.item % menuItems) - 1; 
          vTaskDelay(500 / portTICK_PERIOD_MS);
        }
      }
    }

    /* is the menu timed out while the button isn't being engaged by the user ? */
    if (!menuButton.isBeingHeld){
      if ((customMenu.active && (millis() - customMenu.time > 15000)) || (userMadeSelectionByHoldingButton && customMenu.active)){
        if (!userMadeSelectionByHoldingButton){bridge.instructServicePortToSetReady();}
        customMenu.active = false; 
        customMenu.item = 0;

        /* restart */
        menuButton.endat = 0;
        menuButton.startat = 0;
        menuButton.released = false;
        menuButton.isBeingHeld = false;
      }
    }
  }
}

/***************************************************************************//**
 * Arduino-specific setup loop. 
 *
 * @param[out] none 
 *     
 * @param[in] none
 ******************************************************************************/
void setup() {

  /* led control */
  pinMode(DEV_BOARD_LED_PIN, OUTPUT);
  xTaskCreate(
    statusLEDBlinker,   
    "statusLEDBlinker",  
    2048,           
    NULL,      
    2,
    &xLED
  );

  /* UART requires to much vTaskDelayMilliseconds and blocking; semaphore to prevent doubled instructions */
  xUARTSemaphore = xSemaphoreCreateBinary(); 
  xMQTTSemaphore = xSemaphoreCreateBinary(); 
  xMQTTStatusSemaphore = xSemaphoreCreateBinary();
  xWIFIStatusSemaphore = xSemaphoreCreateBinary();
  xMachineReadyStateVariableSemaphore = xSemaphoreCreateBinary();

  /* give all semaphores */
  xSemaphoreGive( xUARTSemaphore);
  xSemaphoreGive( xMQTTSemaphore);
  xSemaphoreGive( xMQTTStatusSemaphore);
  xSemaphoreGive( xWIFIStatusSemaphore);
  xSemaphoreGive( xMachineReadyStateVariableSemaphore);

  /* lock mqtt vals*/
  xSemaphoreWrappedSetBoolean(xMQTTStatusSemaphore, connectedToBroker, false);
  xSemaphoreWrappedSetBoolean(xWIFIStatusSemaphore, connectedToNetwork, false);

  /* mqtt queue values */
  rx_message.topic.reserve(100);

  /* queue holds one message at a time */
  xQ_SubscriptionMessage = xQueueCreate( 1, sizeof(mqtt_message) );

  /* booting message on display */
  bridge.instructServicePortToDisplayString(" STARTING ");

  /* print hardware information */
  ESP_LOGI(TAG,"jurabridge v" VERSION_STR);

  //set output pin modes
  pinMode(DEV_BOARD_BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(DEV_BOARD_BUTTON_PIN, buttonInterruptSwitchPressRoutine,CHANGE);
  xTaskCreate(
    customMenuHandler,   
    "customMenuHandler",  
    10000,           
    NULL,      
    2,
    NULL
  );

  /* init preferences */
  prefs.begin(PREF_KEY, false);

  /* mqtt keepalive; design pattern inspired by @idahowalker */
  xTaskCreate( 
    communicationsKeepAliveTask, 
    "communicationsKeepAliveTask", 
    10000, 
    NULL, 
    5, 
    NULL)
  ;

  /* mqtt queue */
  xTaskCreate( 
    receivedMQTTMessageQueueWorker,
    "receivedMQTTMessageQueueWorker", 
    10000, 
    NULL, 
    5, 
    NULL
  );

  /* uart task */
  xTaskCreate(
    machineStatePollingHandler,        
    "machineStatePollingHandler",      
    10000,            
    NULL,            
    4,               
    &xUART
  );

}

/***************************************************************************//**
 * Arduino-specific main loop. [intentionally left blank]
 *
 * @param[out] none 
 *     
 * @param[in] none
 ******************************************************************************/
void loop() {}