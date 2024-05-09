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
	bool pressed;
  bool isBeingHeld;
  unsigned long startat;
  unsigned long endat;
};

/* button */
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
    vTaskDelayMilliseconds(250);
  }
  
  /* get innitial properties before polling starts y! */
  initJuraEntityStatesFromNonVolatileStorage();

  int loopIterator = 1; 
  for(;;){ 
    if (!customMenu.active){
      machine.handlePoll(loopIterator);
      loopIterator = (loopIterator % 100) + 1; 
      vTaskDelayMilliseconds(250);
    }else{
      vTaskDelayMilliseconds(100);
    }
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
            ESP_LOGI(TAG,"Custom limits set: Water: %i ml Milk: %i ml Brew: %i ml Add: %i shots", waterLimit, milkLimit, brewLimit, addShot );
            continue;
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

          /* add shot ? */
          if (addShot > 0) {
            
            /* debug */
            ESP_LOGI(TAG,"--> Machine Add Shot Function: %s", JuraMachineFunctionEntityConfigurations[i].name);

            /* precharge the wait display for between elements */
            bridge.instructServicePortToDisplayString("   WAIT   ");

            /* iterating over two shots */
            for( int shot = 0; shot < addShot; shot ++){

              /* define a non-ready state that is not processed in the machine poller; protect  */
              xSemaphoreTake( xMachineReadyStateVariableSemaphore, portMAX_DELAY );
              machine.startAddShotPreparation();

              /* 
                NOTE TO READER: intentionally leave the semaphore taken here to prevent a ready state from happen
                until we get into the while loop below 
              */

              /* entire the loop before returning the semaphore */
              bool canGiveSemaphore = true;
              while (machine.states[(int) JuraMachineStateIdentifier::SystemIsReady] == false){
                vTaskDelayMilliseconds(500);
                if (canGiveSemaphore){
                  xSemaphoreGive( xMachineReadyStateVariableSemaphore);
                  canGiveSemaphore = false;
                }
              }

              /* between different shots - resting period */              
              vTaskDelayMilliseconds(750);

              /* set brew limit here */
              machine.setDispenseLimit(brewLimit, JuraMachineDispenseLimitType::Brew) ;

              /* send the command for this repetition */
              bridge.instructServicePortWithJuraFunctionIdentifier(JuraFunctionIdentifier::MakeEspresso);

              /* resting period */              
              vTaskDelayMilliseconds(750);
            }
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
  if (digitalRead(DEV_BOARD_BUTTON_PIN) == HIGH && menuButton.pressed == false){
    buttonChangeTime = millis();
    if (buttonChangeTime - menuButton.endat > DEBOUNCE){
      menuButton.pressed = true;
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
  for(;;){
    /* menu active or not? */
    bool selectionMade = false;
    if (menuButton.pressed) {
      /* reset button immediately */
      menuButton.pressed = false;
      menuButton.isBeingHeld = false;

      /* how long was it pressed ?*/
      int duration = menuButton.endat - menuButton.startat;
      menuButton.startat = 0;

      /* button held */
      if (duration > BUTTON_HOLD_DURATION_MS){selectionMade = true;}

      /* menu is active, lets change the menu item!*/
      if (!selectionMade){
        /* display the menu item */
        if (customMenu.active){
          /* iterate the selected menu */
          customMenu.item = (customMenu.item % menuItems) + 1; 
          customMenu.time = millis();

          /* set menu name; zero indexed but iterator is 1 indexed */
          bridge.instructServicePortToDisplayString(JuraCustomMenuItemConfigurations[customMenu.item - 1].name);

        }else{

          bridge.instructServicePortToDisplayString(JuraCustomMenuItemConfigurations[0].name);
          customMenu.item = 1;
          customMenu.active = true;
          customMenu.time = millis();

          /* cancel button press right here to show the first menu item */
          menuButton.endat = 0;
          menuButton.startat = 0;
          menuButton.pressed = false;
          menuButton.isBeingHeld = false;
          continue; 
        }

      }else{
        if (customMenu.active){
          if (customMenu.item < menuItems){
              bridge.instructServicePortToDisplayString("   WAIT");

              ESP_LOGI(TAG, "%s, ", JuraCustomMenuItemConfigurations[customMenu.item - 1].topic);
              /* send mqtt message corresponding to selected emnu item */
              xSemaphoreTake( xMQTTSemaphore, portMAX_DELAY );
              mqttClient.publish(
                JuraCustomMenuItemConfigurations[customMenu.item - 1].topic,
                JuraCustomMenuItemConfigurations[customMenu.item - 1].payload 
              );  
              xSemaphoreGive(xMQTTSemaphore);
            }else{
              /* exit item!*/
              bridge.instructServicePortToSetReady();
            }
          }
        }

       
    }else if (customMenu.active) {
      /* if startat is nonzero, then the button is currently being pressed*/
      if (menuButton.startat != 0){
        if (millis() - menuButton.startat > 500 && !menuButton.isBeingHeld){
          menuButton.isBeingHeld = true; 
          bridge.instructServicePortToDisplayString("    OK");
          vTaskDelay(750 / portTICK_PERIOD_MS);
        }
      }
    }

    /* is the menu timed out ? */
    if ((customMenu.active && (millis() - customMenu.time > 15000)) || (selectionMade && customMenu.active)){
      if (!selectionMade){bridge.instructServicePortToSetReady();}
      customMenu.active = false; 
      customMenu.item = 0;

      /* restart */
      menuButton.endat = 0;
      menuButton.startat = 0;
      menuButton.pressed = false;
      menuButton.isBeingHeld = false;
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
  //ESP_LOGI(TAG,"jurabridge v" VERSION_STR);
  ESP_LOGI(TAG,"--------------------------------------------------");
  ESP_LOGI(TAG,"  Internal Total heap %d, internal Free Heap %d", ESP.getHeapSize(), ESP.getFreeHeap());
  ESP_LOGI(TAG,"  SPIRam Total heap %d, SPIRam Free Heap %d", ESP.getPsramSize(), ESP.getFreePsram());
  ESP_LOGI(TAG,"  ChipRevision %d, Cpu Freq %d, SDK Version %s", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  ESP_LOGI(TAG,"  Flash Size %d, Flash Speed %d", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  ESP_LOGI(TAG,"--------------------------------------------------");

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