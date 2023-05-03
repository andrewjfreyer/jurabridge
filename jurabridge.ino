//includes for wifi handling
#include <WiFi.h>
#include <string>
#include <mutex>
#include <ArduinoJson.h>
#include "PubSubClient.h"
#include "Preferences.h"

// bridge configuration
#include "secrets.h"
#include "configuration.h"

//define some vars
TaskHandle_t JURA_UPDATE_TASK;
Preferences preferences; // save values to flash for reboot
std::mutex mutex; // stop race conditions by locking

//mqtt and interupt stuffs
WiFiClient espClient;
PubSubClient client(espClient);

//wifi definintions
#define TAG "jurabridge"
#define VERSION "0.4.25"

//for showing on the main display 
#define CURRENT_VERSION_DISPLAY "DT: READY v4"

//mode investigation
#define MQTT_ENABLED true
#define PREFERENCES_ENABLED true
#define MODE_INVESTIGATION false
#define UNHANDLED_INVESTIGATION false 
#define UART_TIMEOUT 300

//specific investigations
#define EEPROM_INVESTIGATION false
#define CS_INVESTIGATION false
#define IC_INVESTIGATION false
#define HZ_INVESTIGATION false
#define AS_INVESTIGATION false

//-----------------------------------------------------------------
// version notes
//
// 0.4.25 - custom recipe work
// 0.4.24 - remove custom hardcoded recipes, MQTT only
// 0.4.23 - update mqtt buffer size
// 0.4.22 - recipe format update 
// 0.4.21 - bugfix re: calculating estimated hopper volume remaining
// 0.4.20 - json custom recipes!
// ---
// 0.4.15 - adjust grind/strength estimations
// 0.4.14 - restart in response to home assistant reboot (TODO: more elegant response??); additional buttons on webUI?
// 0.4.13 - fix bug with last dispense error
// 0.4.12 - fix error 
// 0.4.11 - first publication flag; maybe remove init?? no, need to know at least a single value exists 
// 0.4.10 - rename sensors to high pressure/low pressure pump operations
// 0.4.9 - status reordering; pump offtime from timout 
// 0.4.8 - remove tray error tracker; wifi reliablity work
// 0.4.7 - remove websockets stuffs; simplify
// 0.4.6 - fill beans soon warning
// 0.4.5 - reduce size of web page hosted to custom automations only; fix wait_ready to account for duration 
// 0.4.4 - link communications of MQTT and websockets
// 0.4.3 - fix flow meter MQTT reporting; fix ceramic valve error
// 0.4.2 - custom executino status, blocking further executions
// 0.4.1 - correct version name format; correct custom automations for history log
// 0.4.0 - timeouts; new version; crash recovery
// 0.3.06 - identify AS
// 0.3.05 - custom automation handling; interrupt on main core, not trapped in waitloop
// 0.3.04 - finally find a reutrnn >= 999 counter... version up entire system;
// 0.3.02 - differeing update intervals; find flow meter!
// 0.3.01 - completed arudino convrsion; add JSON output
// 0.3.00 - convert to arduino 
// 0.2.41 - try supplemental commends from other sources; non seem to work...
// 0.2.40 - convert all calculations to cpp; reduce lambdas
// 0.2.39 - delay testing for connectivity?
// 0.2.28 - support for machine version (needs bootloader version too?)
// 0.2.27 - more custom automations and experimentation with live modification of automations
// 0.2.26 - pump state tied to output valve state; should always be flush/drain if not pumping (during cleaning to?)
// 0.2.25 - remove hidden options; leave for further testing
// 0.2.24 - refactoring
// 0.2.23 - re-add buttons
// 0.2.22 - add more and more sensors
// 0.2.21 - re-add sensors defined from strings; only pull comes from strings (only blocking?)
// 0.2.20 - minimize all calculations in update loop
// 0.2.19 - ESP32 changeover
// 0.2.18 - proper extraction and naming for cs:; rename to jura bridge
// 0.2.17 - duty cycle to mode translation with longer averaging (UART sampling probably too slow)
// 0.2.16 - pump duty cycle to three modes
// 0.2.15 - add menu items to yaml
// 0.2.14 - rename sensors in yaml to be prefixed by friendly_name
// 0.2.13 - break out cs1 into binary bitfield for testing;
//          change releative temperature to 180F ??
//          annotate hz_7 bitfield based on current research
// 0.2.12 - modify mod-based custom polling intervals for different UART queries
// 0.2.11 - comment out unknown vars; testing later
// 0.2.10 - refactor and relabel all "sensors" to origin-based nomenclature (from base; no more xSensor etc)

// ===== References:
// https://www.youtube.com/watch?v=sNedGBSFs04 (disassembly)
// https://github.com/MayaPosch/BMaC/blob/master/esp8266/app/juraterm_module.cpp 
// https://forum.fhem.de/index.php?topic=45331.0
// https://us.jura.com/-/media/global/pdf/manuals-na/Home/ENA-Micro-90/download_manual_ena_micro90_ul.pdf
// https://github.com/PromyLOPh/juramote <-- HUGE HELP & INSPIRATION
// https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/
// https://blog.q42.nl/hacking-the-coffee-machine-5802172b17c1/
// https://github.com/psct/sharespresso
// https://github.com/Q42/coffeehack
// https://hackaday.com/tag/jura/
// https://community.home-assistant.io/t/control-your-jura-coffee-machine/26604/65
// https://github.com/Jutta-Proto/protocol-cpp
// https://github.com/COM8/esp32-jura
// https://github.com/hn/jura-coffee-machine
// http://protocoljura.wiki-site.com/index.php/Hauptseite
// https://elektrotanya.com/showresult?what=jura-impressa&kategoria=coffee-machine&kat2=all
// https://www.elektro-franck.de/search?sSearch=15269&p=1
// https://www.juraprofi.de/anleitungen/Jura_ENA_Micro-9-90_A-5-7-9_Wasserlaufplan.pdf
// https://www.thingiverse.com/thing:5348735
// https://github.com/sklas/CofFi/blob/master/sketch/coffi_0.3.ino
// https://github.com/Q42/coffeehack/blob/master/reverse-engineering/commands.txt
// https://tore.tuhh.de/bitstream/11420/11433/2/Antrittsvortrag.pdf
// https://protocol-jura.at.ua/index/commands_for_coffeemaker/0-5 

//basic app structure structure from: https://github.com/hn/jura-coffee-machine/blob/master/cmd2jura.ino
/*

AS:   as:0734 0BB1 0000,0734 0BFC 0001 
DI:   drainage valve init? 
GB:   machine shut down (without rinse; HV board shut down? )
MI:   drainage valve init OTHER position
MC:   drainage valve init OTHER position
MV:   drainage valve cycling
RM:   rm:8400; requires 2 byte argument; 02 changes from 257, 258, 259 
TS:   ts:
TT:   OK

Something between MV and UU clears the display to blank.
*/

//-----------------------------------------------------------------
// HZ - Heißwasser-Zubereitung ??
// CS - circuits und Systeme ??
// IC - input contorller
// FA - Funktion auswählen; select a function??
// FN - Funktion ??
//-----------------------------------------------------------------

//hardware UART is significantly faster than software serial
HardwareSerial JuraSerial ( 2 );

//for json crash detection? 
long last_update = 0;
long status_update = 0;
bool first_publication;

//val registry 
long rt1_val [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long rt2_val [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long rt3_val [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long hz_val [22]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long cs_val [16]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long ic_val [4]   = {0,0,0,0};
long as_val [6]   = {0,0,0,0,0,0};

//prev val registry
long rt1_val_prev [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long rt2_val_prev [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long rt3_val_prev [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long hz_val_prev [22]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long cs_val_prev [16]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long ic_val_prev [4]   = {0,0,0,0};
long as_val_prev [6]   = {0,0,0,0,0,0};

//timestamps!
long rt1_val_last_changed [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long rt2_val_last_changed [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long rt3_val_last_changed [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long hz_val_last_changed [22]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long cs_val_last_changed [16]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long ic_val_last_changed [4]   = {0,0,0,0};
long as_val_last_changed [6]   = {0,0,0,0,0,0};

long rt1_val_prev_last_changed [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long rt2_val_prev_last_changed [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long rt3_val_prev_last_changed [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long hz_val_prev_last_changed [22]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long cs_val_prev_last_changed [16]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
long ic_val_prev_last_changed [4]   = {0,0,0,0};
long as_val_prev_last_changed [6]   = {0,0,0,0,0,0};

//whether a chnage has been handled (allows for no debug printing of known values)
bool rt1_handled_change [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
bool rt2_handled_change [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
bool rt3_handled_change [16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
bool hz_handled_change  [22]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
bool cs_handled_change [16]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
bool ic_handled_change [4]   = {0,0,0,0};
bool as_handled_change [6]   = {0,0,0,0,0,0};

//inits values from defaults 
bool rt1_initialized;
bool rt2_initialized;
bool rt3_initialized;
bool hz_initialized;
bool cs_initialized;
bool ic_initialized;
bool as_initialized;

// update intervals (approximate every x seconds)
int loop_iterator;

//different values holders
int status_update_interval;

//update intervals
int rt1_update_interval;
int rt2_update_interval;
int rt3_update_interval;
int hz_update_interval;
int ic_update_interval;
int cs_update_interval;
int as_update_interval;

//extracted values; not reported
unsigned long thermoblock_temperature; 
unsigned long ceramic_valve_temperature;
unsigned long last_dispense_volume;
unsigned long pump_duty_cycle;
unsigned long thermoblock_duty_cycle;

//ceramic valve position information? 
unsigned long ceramic_valve_position;
unsigned long last_task;
unsigned long last_task_previous;
unsigned long system_state;
unsigned long system_state_previous;

//eeprom stats
unsigned long grinder_operations;
unsigned long espresso_preparations;
unsigned long coffee_preparations;
unsigned long cappuccino_preparations;
unsigned long macchiato_preparations;
unsigned long milk_foam_preparations;
unsigned long water_preparations;
unsigned long low_pressure_pump_operations; //essentially only "rinse" cycles
unsigned long clean_cycles;
unsigned long spent_grounds;
unsigned long spent_beans_by_weight;
unsigned long preparations_since_last_clean;
unsigned long milk_clean_total;
unsigned long high_pressure_pump_operations;
unsigned long flow_meter_position;
unsigned long output_valve;
unsigned long input_board_state;

//dispense trackers
unsigned long volume_since_reservoir_fill; 
unsigned long volume_since_reservoir_fill_previous;

unsigned long drainage_since_tray_empty; 
unsigned long drainage_since_tray_empty_previous;

//ginder data
unsigned long grinder_start; 
unsigned long grinder_end; 
unsigned long last_grinder_duration_ms; 

//last custom execution
unsigned long custom_execution_started; 
unsigned long custom_execution_completed; 

//==== booleans 

//inferred master state
bool next_counter_is_custom_automation_part;
bool system_ready; //inferred value
bool input_sensors_ready; //value from IC
bool mclean_recommended; //inferred from milk preparation 
bool mrinse_recommended; //inferred from milk preparation 

//for automations
bool custom_execution; 
bool last_custom_execution;
long recommendation_state;

//direct values 
bool overdrainage_error; //implied by counter!
bool overdrainage_error_previous;
bool volume_since_reservoir_fill_error;
bool volume_since_reservoir_fill_error_previous;

bool drip_tray_error; //direct value from either HZ (not currently) or IC
bool hopper_cover_error; //direct value from either HZ (not currently) or IC
bool water_reservoir_error; //direct value from either HZ (not currently) or IC
bool powder_door_error; //direct value from either HZ (not currently) or IC
bool spent_grounds_error; //inferred from >= 8; if 255 (or greater than 10 in any way; cleaning cycle)

bool rinse_recommended; //based on direcrt HZ bit readout
bool clean_recommended; //based on preparation number; 

bool thermoblock_preheated; //this is non-blocking, but is an affirmative bit
bool flow_meter; //inferred from output valve state
bool brewgroup_init_position; // affirmative bit;
bool pump_active; //inferred from pump duty cycle
long pump_status;
long pump_inactive_start;

bool is_cleaning; //inferred from grounds counter
bool steam_mode; //affirmative from ceramic valve
bool water_mode; //affirmative from ceramic valve

bool grinder_active; //apparently affirmative? 
bool fill_beans_error_inferred;
bool fill_beans_soon_inferred;
bool thermoblock_active; 
bool flowing;

//for output valve
bool isOutputValveBrewPosition;
bool isOutputValveFlushPosition;
bool isOutputValveDrainPosition;
bool isOutputValveBlockPosition;

bool isCeramicValvePressureRelief;
bool isCeramicValveVenturiPosition;
bool isCeramicValveSteamCircuitPosition;
bool isCeramicValveTransitioning; 
bool isCeramicValveHotWaterCircuit;
bool isCeramicValveBlocking;
bool isCeramicValveCoffeeProductCircuit;
bool isCeramicValveUnknownPosition;

/*

cmd2jura

original: https://github.com/hn/jura-coffee-machine/blob/master/cmd2jura.ino

   cmd2jura.ino V1.00
   ESP8266 IP Gateway for Jura coffee machines
   Usage:
   Open "http://jura/" or "curl -d 'TY:' http://jura/api"
   (C) 2017 Hajo Noerenberg
   http://www.noerenberg.de/
   https://github.com/hn/jura-coffee-machine
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3.0 as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License along
   with this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.txt>.
          

modified: 

  andrew j freyer
  designed for multiprocessing, core1 dedicated to wifi core0 dedicated to UART comms with 
  jura machine

*/

String cmd2jura(String outbytes) {
  //lock here
  std::lock_guard<std::mutex> lock(mutex);
  
  //never  send AN:OA

  String inbytes;
  inbytes.reserve(100);
  int w = 0;

  //timeout for available read
  while (JuraSerial.available()) {
    JuraSerial.read();
  }

  outbytes += "\r\n";
  for (int i = 0; i < outbytes.length(); i++) {
    for (int s = 0; s < 8; s += 2) {
      char rawbyte = 255;
      bitWrite(rawbyte, 2, bitRead(outbytes.charAt(i), s + 0));
      bitWrite(rawbyte, 5, bitRead(outbytes.charAt(i), s + 1));
      JuraSerial.write(rawbyte);
    }
    delay(8);
  }

  int s = 0;
  char inbyte;
  while (!inbytes.endsWith("\r\n")) {
    if (JuraSerial.available()) {
      byte rawbyte = JuraSerial.read();
      bitWrite(inbyte, s + 0, bitRead(rawbyte, 2));
      bitWrite(inbyte, s + 1, bitRead(rawbyte, 5));
      if ((s += 2) >= 8) {
        s = 0;
        inbytes += inbyte; 
      }
    } else {
      delay(10);
    }
    if (w++ > 500) {
      return "";
    }
  }
 
  //return full rx response without status prefix (e.g., "IC:...")
  if (inbytes.length() > 3 ){
    return inbytes.substring(3, inbytes.length() - 2);
  }else{
    return inbytes.substring(0, inbytes.length() - 2);
  }
}

void setup() {

  //start UART port on appropriate pins, as defined above. 
  JuraSerial.begin(9600,SERIAL_8N1, GPIORX, GPIOTX);
  JuraSerial.setRxTimeout(UART_TIMEOUT);

  //mqtt set first
  first_publication = true;

  //are we saving durable prefrences??
  if (PREFERENCES_ENABLED){
    //save preferences?
    preferences.begin("jurabridge", false); 

    //boot count increment
    long bootcount = preferences.getInt( PREF_BOOT_COUNT, 0);
    preferences.putInt(PREF_BOOT_COUNT, bootcount++);

    //get durable proprerties
    mclean_recommended = preferences.getBool( PREF_MCLEAN_ERR, false);
    mrinse_recommended = preferences.getBool( PREF_MRINSE_ERR, false);
    drip_tray_error = preferences.getBool( PREF_DRIP_ERR, false);
    hopper_cover_error = preferences.getBool( PREF_BEAN_ERR, false);
    water_reservoir_error = preferences.getBool( PREF_WATER_ERR, false);
    powder_door_error = preferences.getBool( PREF_POWDER_DOOR, false);
    spent_grounds_error = preferences.getBool( PREF_GROUNDS_ERR, false);
    fill_beans_error_inferred = preferences.getBool( PREF_FILLBEANS_ERR, false);
    rinse_recommended = preferences.getBool( PREF_RINSE_REC, false);
    clean_recommended = preferences.getBool( PREF_CLEAN_REC, false);

    //get durable properties
    last_dispense_volume = preferences.getInt( PREF_LAST_OUTVOLUME, 0);
    last_task = preferences.getInt( PREF_HISTORY, 0);
    spent_beans_by_weight = preferences.getInt( PREF_HOPPER_VOLUME, 0);

    //get durable properties
    grinder_operations = preferences.getInt( PREF_NUM_GRINDS, 0);
    espresso_preparations = preferences.getInt( PREF_NUM_ESPRESSO, 0);
    coffee_preparations = preferences.getInt( PREF_NUM_COFFEE, 0);
    cappuccino_preparations = preferences.getInt( PREF_NUM_CAPPUCCINO, 0);
    macchiato_preparations = preferences.getInt( PREF_NUM_MACCHIATO, 0);
    milk_foam_preparations = preferences.getInt( PREF_NUM_MILK, 0);
    water_preparations = preferences.getInt( PREF_NUM_WATER, 0);
    low_pressure_pump_operations = preferences.getInt( PREF_NUM_LP_OPS, 0);
    clean_cycles = preferences.getInt( PREF_NUM_CLEANS, 0);
    spent_grounds = preferences.getInt( PREF_NUM_GROUND, 0);
    preparations_since_last_clean = preferences.getInt( PREF_NUM_PREP_SINCE, 0);
    milk_clean_total = preferences.getInt( PREF_NUM_MCLEAN, 0);
    high_pressure_pump_operations = preferences.getInt( PREF_NUM_HP_OPS, 0);

    //continue counting dispensing
    drainage_since_tray_empty = preferences.getInt( PREF_NUM_DRAINAGE_SINCE_EMPTY, 0);
    volume_since_reservoir_fill = preferences.getInt( PREF_VOL_SINCE_RESERVOIR_FILL, 0);

    //get durable properties
    output_valve = preferences.getInt( PREF_POS_OUTVALVE, 0);
    ceramic_valve_position = preferences.getInt( PREF_POS_CVALVE, 0);
  }

  //debugging
  Serial.begin(115200);
  Serial.println("Setup Starting...");

  //needs to start at 1; cannot start at zero else 0 % time == 0
  loop_iterator = 1;
  spent_grounds = 99;
  water_mode = true;
  system_ready = true;

  rt1_update_interval = RT1_UPDATE_INTERVAL_STANDBY; 
  rt2_update_interval = RT2_UPDATE_INTERVAL_STANDBY; 
  rt3_update_interval = RT3_UPDATE_INTERVAL_STANDBY; 
  ic_update_interval = IC_UPDATE_INTERVAL_STANDBY; 
  hz_update_interval = HZ_UPDATE_INTERVAL_STANDBY; 
  cs_update_interval = CS_UPDATE_INTERVAL_STANDBY; 
  as_update_interval = AS_UPDATE_INTERVAL_STANDBY; 

  //status update
  status_update_interval = 1;
  
  //wifi confiburaiton
  WiFi.mode(WIFI_STA); //only for conencting to access point

  //connect to Wi-Fi
  WiFi.begin(WIFINAME, WIFIPASS);

  //ensure we're connected to Wi-Fi
  set_display_WiFiInit();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());

  //mqtt stuffs
  set_display_MQTTInit();

  //init mqtt broker connection
  client.setServer(MQTTBROKER, 1883);
  client.setCallback(callback);
  client.setBufferSize(2048); //for receiving recipes

  //connect to qmtt
  mqtt_reconnect();

  //move jura uart stuffs to core 2
  xTaskCreatePinnedToCore(
    jura_bridge, /* Function to implement the task */
    "jura_bridge", /* Name of the task */
    50000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    0,  /* Priority of the task */
    &JURA_UPDATE_TASK,  /* Task handle. */
    0); /* Core where the task should run */

  //define pins 
  pinMode(GPIOLEDPULSE, OUTPUT);
}

//int for messing with the LED
int led_iterator = 0;

//main loop
void loop() {

  //mqtt connecting
  if (!client.connected()) {
    digitalWrite(GPIOLEDPULSE, HIGH);
    mqtt_reconnect();
  }

  //handle mqtt client in loop
  client.loop();

  //light to indicate working
  led_iterator = (led_iterator + 1) % 20000;
  digitalWrite(GPIOLEDPULSE, (led_iterator < 10000) ? LOW : HIGH);
}

//----------------------------------------------------------------------------
//
//      CUSTOM DISPLAY
//
//----------------------------------------------------------------------------
void set_display_automation_step_1()        {String cmd; cmd.reserve(20); cmd="DT:  STEP 1  "; cmd2jura(cmd);} 
void set_display_automation_step_2()        {String cmd; cmd.reserve(20); cmd="DT:  STEP 2  "; cmd2jura(cmd);} 
void set_display_automation_add_shot()      {String cmd; cmd.reserve(20); cmd="DT: ADD SHOT "; cmd2jura(cmd);} 
void set_display_custom()                   {String cmd; cmd.reserve(20); cmd="DT:  CUSTOM  "; cmd2jura(cmd);} 
void set_display_wait()                     {String cmd; cmd.reserve(20); cmd="DT:   WAIT   "; cmd2jura(cmd);} 
void set_display_done()                     {String cmd; cmd.reserve(20); cmd="DT:   DONE   "; cmd2jura(cmd);} 
void set_display_interrupt()                {String cmd; cmd.reserve(20); cmd="DT:    :(    "; cmd2jura(cmd);} 
void set_display_bridge_ready()             {String cmd; cmd.reserve(20); cmd=CURRENT_VERSION_DISPLAY; cmd2jura(cmd);}

//Synching display
void set_display_WiFiInit()             {String cmd; cmd.reserve(20); cmd="DT:  WIFI... "; cmd2jura(cmd);}
void set_display_MQTTInit()             {String cmd; cmd.reserve(20); cmd="DT:  MQTT... "; cmd2jura(cmd);}
void set_display_syncing_0()            {String cmd; cmd.reserve(20); cmd="DT:  SYNC  "; cmd2jura(cmd);}
void set_display_syncing_1()            {String cmd; cmd.reserve(20); cmd="DT:  SYNC. "; cmd2jura(cmd);}
void set_display_syncing_2()            {String cmd; cmd.reserve(20); cmd="DT:  SYNC.. "; cmd2jura(cmd);}
void set_display_syncing_3()            {String cmd; cmd.reserve(20); cmd="DT:  SYNC..."; cmd2jura(cmd);}

//----------------------------------------------------------------------------
//      DECODED COMMANDS
//----------------------------------------------------------------------------

//better phrasings for these functions? 
//FA = Function Automation
void press_button_power_off()             {String cmd; cmd.reserve(20); cmd="FA:01"; cmd2jura(cmd);} // don't need this if we have a relay power controller
void press_rotary_button_confirm_prompt() {String cmd; cmd.reserve(20); cmd="FA:02"; cmd2jura(cmd);}
void press_button_open_settings()         {String cmd; cmd.reserve(20); cmd="FA:03"; cmd2jura(cmd);}
void press_button_select_menu_item()      {String cmd; cmd.reserve(20); cmd="FA:04"; cmd2jura(cmd);}
void rotate_rotary_right()                {String cmd; cmd.reserve(20); cmd="FA:05"; cmd2jura(cmd);}
void rotate_rotary_left()                 {String cmd; cmd.reserve(20); cmd="FA:06"; cmd2jura(cmd);}

//what can this machine do? 
void press_button_espresso()              {String cmd; cmd.reserve(20); cmd="FA:07"; cmd2jura(cmd);}
void press_button_cappuccino()            {String cmd; cmd.reserve(20); cmd="FA:08"; cmd2jura(cmd);}
void press_button_coffee()                {String cmd; cmd.reserve(20); cmd="FA:09"; cmd2jura(cmd);}
void press_button_macchiato()             {String cmd; cmd.reserve(20); cmd="FA:0A"; cmd2jura(cmd);}
void press_button_water()                 {String cmd; cmd.reserve(20); cmd="FA:0B"; cmd2jura(cmd);}
void press_button_milk()                  {String cmd; cmd.reserve(20); cmd="FA:0C"; cmd2jura(cmd);}

//functions for custom automations
//FN = Function
void pump_full_duty_on()                  {String cmd; cmd.reserve(20); cmd="FN:01"; cmd2jura(cmd);} //verified; but needs to be in brewing position first, else will drain into tray
void pump_full_duty_off()                 {String cmd; cmd.reserve(20); cmd="FN:02"; cmd2jura(cmd);} //verified
void drain_valve_on()                     {String cmd; cmd.reserve(20); cmd="FN:1D"; cmd2jura(cmd);} //verified
void drain_valve_off()                    {String cmd; cmd.reserve(20); cmd="FN:1E"; cmd2jura(cmd);} //verified

/*
void thermoblock_on()                     {String cmd; cmd.reserve(20); cmd="FN:24"; cmd2jura(cmd);} //verified ?? may preheat to 107???
void thermoblock_off()                    {String cmd; cmd.reserve(20); cmd="FN:25"; cmd2jura(cmd);} //???
void grinder_on()                         {String cmd; cmd.reserve(20); cmd="FN:07"; cmd2jura(cmd);} //verified
void grinder_off()                        {String cmd; cmd.reserve(20); cmd="FN:08"; cmd2jura(cmd);} //verified

//other commands are verified, but don't use, they trigger error 8 (brewgroup failure) or leave the thermoblock on and do not turn it off! (thermal switch will disable power to machine)
void init_brewgroup()                     {String cmd; cmd.reserve(20); cmd="FN:0D"; cmd2jura(cmd);} //verified ; 
void brewing_position()                   {String cmd; cmd.reserve(20); cmd="FN:22"; cmd2jura(cmd);} //verified

void tamper_press()                       {String cmd; cmd.reserve(20); cmd="FN:0B"; cmd2jura(cmd);} //verified
void tamper_release()                     {String cmd; cmd.reserve(20); cmd="FN:0C"; cmd2jura(cmd);} //verified
void empty_steam_valve()                  {String cmd; cmd.reserve(20); cmd="FN:29"; cmd2jura(cmd);} //verified, be careful here as overheating is very possible

*/
//----------------------------------------------------------------------------
//
//      CUSTOM AUTOMATION / RECIPE HELPER METHODS
//
//----------------------------------------------------------------------------

bool wait_ready(int max_seconds = 60, int min_duration = 2){
  int delay_const = 500;
  int max_iter = (max_seconds * 1000) /  delay_const;
  for (int i = 0; i < max_iter; i++){
    //needs to be ready for at least...
    delay(delay_const);
    if (system_ready && (millis() - status_update > (min_duration * 1000))){
      break;
    }
  }
  return system_ready;
}

bool wait_thermoblock_ready (int max_seconds = 30){
  int delay_const = 500;
  int max_iter = (max_seconds * 1000) /  delay_const;
  for (int i = 0; i < max_iter; i++){
    //needs to be ready for at least...
    delay(delay_const);
    if (thermoblock_preheated || thermoblock_temperature > 100){
      return true;
    }
  }
  return false;
}

bool wait_pump_start (int max_seconds = 15){
  //wait for thermoblock to be ready!
  if (wait_thermoblock_ready()){
    int delay_const = 500;
    int max_iter = (max_seconds * 1000) /  delay_const;
    for (int i = 0; i < max_iter; i++){
      //needs to be ready for at least...
      delay(delay_const);
      if (pump_active){
        return true;
      }
    }
    return false;
  }
  return false;
}

bool wait_dispense_reset (){
  int max_seconds = 15;
  int delay_const = 100;
  int max_iter = (max_seconds * 1000) /  delay_const;

  for (int i = 0; i < max_iter; i++){
    //needs to be ready for at least...
    //stop condition
    delay(delay_const);
    if (last_dispense_volume <= 10 ){
        return true;
      }
  }
  return false;
}

bool wait_dispense_ml ( int max_ml = 40){
  //wait for dispense reset! 
  if (wait_dispense_reset()){
    int max_seconds = 60;
    int delay_const = 100;
    int max_iter = (max_seconds * 1000) /  delay_const;

    for (int i = 0; i < max_iter; i++){
      //needs to be ready for at least...
      delay(delay_const);
      //stop condition
      if (last_dispense_volume >= max_ml ){
        return true;
      }
    }
    return false;
  }
  return false;
}

//#############################################################
//
// custom automation controls and utility functions
//
//#############################################################

void custom_automation_start(){
  custom_execution = true;
  custom_execution_started = millis();
}

void custom_automation_end(){
  //return 
  custom_execution = false;
  custom_execution_completed = millis(); 
}

void custom_automation_display_reset(bool success){
  //did this run properly??
  if (success){
    set_display_done();
    delay(2500);
  }else{
    set_display_interrupt();
    delay(2500);
  }

  //return display to normal display
  set_display_bridge_ready();
}

// -------------------- MQTT PUBLICATION WRAPPER STUFFS BELOW HERE -----------------------

void mqttpub_long(long value, const char* path){
  if (MQTT_ENABLED){ 
    char mqttFloatToString[8]; 
    dtostrf(value, 1, 0, mqttFloatToString);
    client.publish(path, mqttFloatToString,  false);
  }  
}

void mqttpub_str(const char* value, const char* path){
  if (MQTT_ENABLED){ 
    int len = strlen(value) + 1;
    client.publish(path, value, false);
  }  
}


// -------------------------------------------------------------------
//
//    MAIN JURA CHECKING LOOPS
//
// -------------------------------------------------------------------

// secondary loop on second core
void jura_bridge (void * parameter) {

  //unnecssary animation??it's for fun. sometimes you can have fun too
  for (int i; i < 3; i++){
    set_display_syncing_0();  delay(200);
    set_display_syncing_1();  delay(200);
    set_display_syncing_2();  delay(200);
    set_display_syncing_3();  delay(200);
  }
  
  //set display to ready mode
  set_display_bridge_ready();

  while (true){
    jura_update();
  }
}

// MAIN JURA UPDATE FUNCTION HERE
void jura_update(){
  
  // STANDBY INTERVALS SHOULD BE DIFFERENT FOR FASTER INFORMATION DURING A PROCESS
  if (system_ready) {
    rt1_update_interval = RT1_UPDATE_INTERVAL_STANDBY; 
    rt2_update_interval = RT2_UPDATE_INTERVAL_STANDBY; 
    rt3_update_interval = RT3_UPDATE_INTERVAL_STANDBY; 
    ic_update_interval = IC_UPDATE_INTERVAL_STANDBY; 
    hz_update_interval = HZ_UPDATE_INTERVAL_STANDBY; 
    cs_update_interval = CS_UPDATE_INTERVAL_STANDBY; 
    as_update_interval = AS_UPDATE_INTERVAL_STANDBY; 

  }else{
    rt1_update_interval = RT1_UPDATE_INTERVAL_BREWING;
    rt2_update_interval = RT2_UPDATE_INTERVAL_BREWING;
    rt3_update_interval = RT3_UPDATE_INTERVAL_BREWING;
    ic_update_interval = IC_UPDATE_INTERVAL_BREWING; 
    hz_update_interval = HZ_UPDATE_INTERVAL_BREWING; 
    cs_update_interval = CS_UPDATE_INTERVAL_BREWING; 
    as_update_interval = AS_UPDATE_INTERVAL_BREWING; 
  }
  // -------------------------------------------------------------------------------------------------
  // DATA FROM EEPROM - WORD 1
  // 64 DATA BITS + 3 TAG BITS
  // -------------------------------------------------------------------------------------------------
  
  if ((((loop_iterator % rt1_update_interval) == 0) || ! rt1_initialized ) && update_rt1()){

    //set last update
    last_update = millis() / 1000;
    bool count_changed = false;

    //============================== extracted values EEPROM - 1
    if (update_espresso_preparations() || first_publication) {
      count_changed = true;
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_ESPRESSO, espresso_preparations);
      if (MQTT_ENABLED){ mqttpub_long(espresso_preparations, "jurabridge/counts/espresso") ;}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Espresso: %d",espresso_preparations);
    }  
    if (update_coffee_preparations() || first_publication) {
      count_changed = true;
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_COFFEE, coffee_preparations); 
      if (MQTT_ENABLED){ mqttpub_long(coffee_preparations, "jurabridge/counts/coffee");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Coffee: %d",coffee_preparations);
    }  
    if (update_cappuccino_preparations() || first_publication) {
      count_changed = true;
      if (MQTT_ENABLED){ mqttpub_str(mrinse_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/milk rinse");}
      if (MQTT_ENABLED){ mqttpub_str (mclean_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/milk clean");}
      if (MQTT_ENABLED){ mqttpub_long(cappuccino_preparations, "jurabridge/counts/cappuccino");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Cappuccino: %d",cappuccino_preparations);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_CAPPUCCINO, cappuccino_preparations);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_MCLEAN_ERR, mclean_recommended);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_MRINSE_ERR, mrinse_recommended);
    }  
    if (update_macchiato_preparations() || first_publication) {
      count_changed = true;
      if (MQTT_ENABLED){ mqttpub_str(mrinse_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/milk rinse");}
      if (MQTT_ENABLED){ mqttpub_str(mclean_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/milk clean");}
      if (MQTT_ENABLED){ mqttpub_long(macchiato_preparations, "jurabridge/counts/macchiato");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Macchiato: %d",macchiato_preparations);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_MACCHIATO, macchiato_preparations);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_MCLEAN_ERR, mclean_recommended);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_MRINSE_ERR, mrinse_recommended);
    }  
    if (update_spent_grounds() || first_publication) {
      if (MQTT_ENABLED){mqttpub_long(100 - spent_beans_by_weight / 2, "jurabridge/counts/beans");} //change to percentage
      if (MQTT_ENABLED){mqttpub_long(spent_grounds, "jurabridge/counts/grounds");}
      if (MQTT_ENABLED){mqttpub_str(spent_grounds_error ? "TRUE" : "FALSE", "jurabridge/errors/grounds");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Grounds: %d",spent_grounds);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_GROUND, spent_grounds);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_GROUNDS_ERR, spent_grounds_error);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_HOPPER_VOLUME, spent_beans_by_weight);
    }
    if (update_low_pressure_pump_operations() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_long(low_pressure_pump_operations, "jurabridge/counts/low pressure operations");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Low Pressure Operations: %d",low_pressure_pump_operations);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_LP_OPS, low_pressure_pump_operations);
    }  
    if (update_clean_cycles() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_long(clean_cycles, "jurabridge/counts/cleans");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Cleans: %d",clean_cycles);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_CLEANS, clean_cycles);
    }  
    if (update_preparations_since_last_clean() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_long(preparations_since_last_clean, "jurabridge/counts/since clean");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Preparations Since Clean: %d",preparations_since_last_clean);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_PREP_SINCE, preparations_since_last_clean);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_CLEAN_REC, clean_recommended);
    }
    //automations? 
    if (count_changed){
      if (MQTT_ENABLED){ mqttpub_long(total_automations(), "jurabridge/counts/total automations");}
    } 
    if (UNHANDLED_INVESTIGATION && EEPROM_INVESTIGATION){
      for (int i = 0; i< 16; i++){
        if (rt1_val[i] != rt1_val_prev[i] && !rt1_handled_change[i]){
          ESP_LOGI(TAG,"  rt1[%d] %d -> %d",i,rt1_val_prev[i], rt1_val[i]);
        }
      }
    }
  }
  
  // -------------------------------------------------------------------------------------------------
  // DATA FROM EEPROM - WORD 2
  // 64 DATA BITS + 3 TAG BITS
  // -------------------------------------------------------------------------------------------------
  
  if ((((loop_iterator % rt2_update_interval) == 0) || ! rt2_initialized ) && update_rt2()){
    //set last update
    last_update = millis() / 1000;
    bool count_changed = false;

    //============================== extracted values EEPROM - 2
    if (update_milk_foam_preparations() || first_publication) {
      //milk cleaning
      count_changed = true;
      if (MQTT_ENABLED){ mqttpub_str(mrinse_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/milk rinse");}
      if (MQTT_ENABLED){ mqttpub_str(mclean_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/milk clean");}

      if (MQTT_ENABLED){ mqttpub_long(milk_foam_preparations, "jurabridge/counts/milk foam");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Milk: %d",milk_foam_preparations);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_MCLEAN_ERR, mclean_recommended);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_MRINSE_ERR, mrinse_recommended);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_MILK, milk_foam_preparations);
    }  
    if (update_water_preparations() || first_publication) {
      count_changed = true;
      if (MQTT_ENABLED){ mqttpub_long(water_preparations, "jurabridge/counts/hot water");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Hot Water: %d",water_preparations);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_WATER, water_preparations);
    }  
    if (update_grinder_operations() || first_publication) { 
      if (MQTT_ENABLED){ mqttpub_long(grinder_operations - 7172, "jurabridge/counts/grinder");} //reset to zero
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Grind Operations: %d",grinder_operations);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_GRINDS, grinder_operations);
    }
    if (update_milk_clean_total() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(mrinse_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/milk rinse");}
      if (MQTT_ENABLED){ mqttpub_str(mclean_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/milk clean");}

      if (MQTT_ENABLED){ mqttpub_long(milk_clean_total, "jurabridge/counts/milk clean");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Milk Clean: %d",milk_clean_total);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_MCLEAN_ERR, mclean_recommended);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_MRINSE_ERR, mrinse_recommended);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_MCLEAN, milk_clean_total);
    }  
    if (update_high_pressure_pump_operations() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_long(high_pressure_pump_operations, "jurabridge/counts/high pressure operations");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"High Pressure Operations: %d",high_pressure_pump_operations);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_HP_OPS, high_pressure_pump_operations);
    } 

    //automations? 
    if (count_changed){
      if (MQTT_ENABLED){ mqttpub_long(total_automations(), "jurabridge/counts/total automations");}
    }

    if (UNHANDLED_INVESTIGATION && EEPROM_INVESTIGATION){
      for (int i = 0; i< 16; i++){
        if (rt2_val[i] != rt2_val_prev[i]  && ! rt2_handled_change[i] ){
          ESP_LOGI(TAG,"  rt2[%d] %d -> %d",i,rt2_val_prev[i], rt2_val[i]);
        }
      }
    }
  }
  
  // -------------------------------------------------------------------------------------------------
  // DATA FROM EEPROM - WORD 3
  // 64 DATA BITS + 3 TAG BITS
  // -------------------------------------------------------------------------------------------------
  
  /*
  if ((((loop_iterator % rt3_update_interval) == 0)  || ! rt3_initialized ) && update_rt3()){
    //set last update
    last_update = millis() / 1000;

    if (UNHANDLED_INVESTIGATION && EEPROM_INVESTIGATION){
      for (int i = 0; i< 16; i++){
        if (rt3_val[i] != rt3_val_prev[i] && ! rt3_handled_change[i]){
          ESP_LOGI(TAG,"  rt3[%d] %d -> %d",i,rt3_val_prev[i], rt3_val[i]);
        }
      }
    }
  }
  */

  // -------------------------------------------------------------------------------------------------
  // DATA FROM AS VALUE
  // 6 BYTES, SEPARATED BY COMMA + 3 TAG BITS
  // -------------------------------------------------------------------------------------------------
  
  /*
  if ((((loop_iterator % as_update_interval) == 0)  || ! as_initialized ) && update_as()){
    //set last update
    last_update = millis() / 1000;

    if (UNHANDLED_INVESTIGATION && AS_INVESTIGATION){
      for (int i = 0; i < 6; i++){
        if (as_val[i] != as_val_prev[i] && ! as_handled_change[i]){
          ESP_LOGI(TAG,"  as[%d] %d -> %d",i,as_val_prev[i], as_val[i]);
        }
      }
    }
  }*/

  // -------------------------------------------------------------------------------------------------
  // DATA FROM HZ - Heißwasser-Zubereitung (hot beverage preparation)
  //
  // note: HZ and CS change very fast, because of tenth of degree temperature sensors; 
  // temp variables are rounded down to the nearest ten
  // -------------------------------------------------------------------------------------------------
  
  if ((((loop_iterator % hz_update_interval) == 0) || ! hz_initialized ) && update_hz()){
    //set last update
    last_update = millis() / 1000;

    //============================== extracted values HZ -- heiße Zubereitung (hot preparation)
    if ((update_thermoblock_temperature() && thermoblock_temperature > 0) || first_publication) {
      if (MQTT_ENABLED){ mqttpub_long(thermoblock_temperature, "jurabridge/parts/thermoblock/temp");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Thermoblock Temp.: %d", thermoblock_temperature);
    }  
    if (update_output_valve() || first_publication) {
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_POS_OUTVALVE, output_valve);
      if (MQTT_ENABLED){               
        if (isOutputValveBrewPosition){  
          mqttpub_str("BREW", "jurabridge/parts/output valve/position");
        } else if (isOutputValveFlushPosition){
          mqttpub_str("FLUSH", "jurabridge/parts/output valve/position");
        } else if (isOutputValveDrainPosition){
          mqttpub_str("DRAIN", "jurabridge/parts/output valve/position");
        } else if (isOutputValveBlockPosition){
          mqttpub_str("BLOCK", "jurabridge/parts/output valve/position");
        } 
      }
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Output Valve Status: %d %d %d %d",isOutputValveBrewPosition, isOutputValveFlushPosition, isOutputValveDrainPosition, isOutputValveBlockPosition );
    }  
    if ((update_last_dispense_volume() && last_dispense_volume > 0) || first_publication) {
      if (MQTT_ENABLED){ mqttpub_long(last_dispense_volume, "jurabridge/machine/last dispense");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Last Dispense (ml): %d", last_dispense_volume);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_LAST_OUTVOLUME, last_dispense_volume);
    }  
    if (update_rinse_recommended() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(rinse_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/rinse");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Rinse Recommended: %d", rinse_recommended );
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_RINSE_REC, rinse_recommended);
    }
    if (update_thermoblock_preheated() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(thermoblock_preheated ? "TRUE" : "FALSE", "jurabridge/parts/thermoblock/preheated");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Thermoblock Preheated: %d", thermoblock_preheated );
    }
    if (update_brewgroup_init_position() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(brewgroup_init_position ? "TRUE" : "FALSE", "jurabridge/machine/brewgroup");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Brewgroup Init: %d", brewgroup_init_position );
    }
    if (update_ceramic_valve_position() || first_publication) {
      if (MQTT_ENABLED){ 
        
        mqttpub_str(steam_mode ? "TRUE" : "FALSE", "jurabridge/parts/ceramic valve/mode");
        mqttpub_str(mrinse_recommended ? "TRUE" : "FALSE", "jurabridge/recommendations/milk rinse");
        
        if (isCeramicValvePressureRelief){  
          mqttpub_str("PRESSURE RELIEF", "jurabridge/parts/ceramic valve/position");
        } else if (isCeramicValveVenturiPosition){
          mqttpub_str("VENTURI", "jurabridge/parts/ceramic valve/position");
        } else if (isCeramicValveSteamCircuitPosition){
          mqttpub_str("STEAM", "jurabridge/parts/ceramic valve/position");
        } else if (isCeramicValveTransitioning){
          mqttpub_str("TRANSITIONING", "jurabridge/parts/ceramic valve/position");
        }else if (isCeramicValveHotWaterCircuit){

          mqttpub_str("HOT WATER", "jurabridge/parts/ceramic valve/position");
        }else if (isCeramicValveBlocking){
          mqttpub_str("BLOCKING", "jurabridge/parts/ceramic valve/position");
        }else if (isCeramicValveCoffeeProductCircuit){
          mqttpub_str("BREW", "jurabridge/parts/ceramic valve/position");
        }else{
          mqttpub_str("UNKNOWN", "jurabridge/parts/ceramic valve/position");
        }
      }
    
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Water Mode: %d", water_mode);
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Ceramic Valve State: %d", ceramic_valve_position);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_POS_CVALVE, ceramic_valve_position);
    }  
    if (UNHANDLED_INVESTIGATION && HZ_INVESTIGATION){
      for (int i = 0; i < 22; i++){
        if (hz_val[i] != hz_val_prev[i] && ! hz_handled_change[i]){
          ESP_LOGI(TAG,"  hz[%d] %d -> %d",i,hz_val_prev[i], hz_val[i]);
        }
      }
    }
  }
  
  // -------------------------------------------------------------------------------------------------
  // DATA FROM CS - Circuit und Systeme?
  // -------------------------------------------------------------------------------------------------
  
  if ((((loop_iterator % cs_update_interval) == 0) || ! cs_initialized ) && update_cs()){

    //set last update
    last_update = millis() / 1000;

    //============================== extracted values CS
    if ((update_ceramic_valve_temperature() && ceramic_valve_temperature > 0) || first_publication) {
      if (MQTT_ENABLED){ mqttpub_long(ceramic_valve_temperature, "jurabridge/parts/ceramic valve/temp");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Ceramic Valve Temp.: %d", ceramic_valve_temperature);
    }

    if (update_grinder_active() || first_publication) {
      if (MQTT_ENABLED){mqttpub_long(last_grinder_duration_ms, "jurabridge/machine/last grind duration");}
      if (MQTT_ENABLED){ mqttpub_str(grinder_active ? "TRUE" : "FALSE", "jurabridge/parts/grinder/active");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Grinder (state): %d", grinder_active);      
    }

    if (update_thermoblock_duty_cycle() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_long(thermoblock_duty_cycle, "jurabridge/parts/thermoblock/duty");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Thermoblock (duty): %d", thermoblock_duty_cycle);
    }
    if (update_thermoblock_active() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(thermoblock_active ? "TRUE" : "FALSE", "jurabridge/parts/thermoblock/active");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Thermoblock: %s", thermoblock_active ? "HEATING" : "STANDBY");
    }
    if (update_pump_duty_cycle() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_long(pump_duty_cycle, "jurabridge/parts/pump/duty");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Water Pump (duty): %d", pump_duty_cycle);
    } 
    if (update_pump_status() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(pump_active ? "TRUE" : "FALSE", "jurabridge/parts/pump/active");}
      if (MQTT_ENABLED){ mqttpub_long(pump_status, "jurabridge/parts/pump/status");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Pump: %s", pump_active ? "ON" : "OFF" );
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Pump Status: %d", pump_status );
    } 
    if (UNHANDLED_INVESTIGATION && CS_INVESTIGATION){
      for (int i = 0; i < 16; i++){
        if (cs_val[i] != cs_val_prev[i] && ! cs_handled_change[i]){
          ESP_LOGI(TAG,"  cs[%d] %d -> %d",i,cs_val_prev[i], cs_val[i]);
        }
      }
    }
  } 
  
  // -------------------------------------------------------------------------------------------------
  // DATA FROM IC - input control
  // -------------------------------------------------------------------------------------------------
  //timouts are separate from the main update_ic; so that we timeout immediately;

  if (update_flow_meter_timeout() || first_publication) {
    if (MQTT_ENABLED){ mqttpub_str(flowing ? "TRUE" : "FALSE", "jurabridge/parts/pump/flowing");}
    if (MODE_INVESTIGATION)     ESP_LOGI(TAG,"Flow Meter Timeout: %d", flowing);
  }

  if ((((loop_iterator % ic_update_interval) == 0) || ! ic_initialized ) && update_ic()){
    //set last update
    last_update = millis() / 1000;

    //============================== extracted values IC
    if (update_hopper_cover_error() || first_publication) { // hopper cover error needs to calculate first, else the timeouts will race
      if (MQTT_ENABLED){ 
        mqttpub_str(hopper_cover_error ? "TRUE" : "FALSE", "jurabridge/errors/beans");
        mqttpub_long(100 - spent_beans_by_weight / 2, "jurabridge/counts/beans"); //change to percentage
      }
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Beans Hopper Error: %d", hopper_cover_error);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_BEAN_ERR, hopper_cover_error);
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_HOPPER_VOLUME, spent_beans_by_weight);
    }  
    if (update_drip_tray_error() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(drip_tray_error ? "TRUE" : "FALSE", "jurabridge/errors/tray removed");}

      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Drip Tray Error: %d", drip_tray_error);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_DRIP_ERR, drip_tray_error);
    } 
    if (update_drainage_since_tray_empty() || first_publication){
      if (MQTT_ENABLED){ mqttpub_long(drainage_since_tray_empty, "jurabridge/counts/tray volume");}
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_NUM_DRAINAGE_SINCE_EMPTY, drainage_since_tray_empty);
    }
    if (update_volume_since_reservoir_fill() || first_publication){
      if (MQTT_ENABLED){ mqttpub_long(100 - (volume_since_reservoir_fill/10), "jurabridge/counts/water tank/volume");} //1L tank by default, 1000ml
      if (PREFERENCES_ENABLED) preferences.putInt(PREF_VOL_SINCE_RESERVOIR_FILL, volume_since_reservoir_fill);
    }
    if (update_volume_since_reservoir_fill_error() || first_publication){
      if (MQTT_ENABLED){ mqttpub_str(volume_since_reservoir_fill_error ? "TRUE" : "FALSE", "jurabridge/errors/reservoir low");}
    }

    if (update_overdrainage_error() || first_publication){
      if (MQTT_ENABLED){ mqttpub_str(overdrainage_error ? "TRUE" : "FALSE", "jurabridge/errors/tray overfill");}
    }

    if (update_water_reservoir_error() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(water_reservoir_error ? "TRUE" : "FALSE", "jurabridge/errors/water");}
      if (MODE_INVESTIGATION)  ESP_LOGI(TAG,"Water Reservoir Error: %d", water_reservoir_error);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_WATER_ERR, water_reservoir_error);
    }  
    if (update_flow_meter_idle() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(flowing ? "TRUE" : "FALSE", "jurabridge/parts/pump/flowing");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Flow Meter: %d",flow_meter_position);
    }  
    if (update_powder_door_error() || first_publication) { 
      if (MQTT_ENABLED){ mqttpub_str(powder_door_error ? "TRUE" : "FALSE", "jurabridge/errors/powder");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Powder Door Error: %d", powder_door_error);
      if (PREFERENCES_ENABLED) preferences.putBool(PREF_POWDER_DOOR, powder_door_error);
    }  
    if (update_input_board_state() || first_publication) {
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Sensor(s) State: %d", input_board_state);
      if (MQTT_ENABLED){ mqttpub_str(input_sensors_ready ? "TRUE" : "FALSE", "jurabridge/machine/input board/state");}
    }
    if (update_input_sensors_ready() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(input_sensors_ready ? "TRUE" : "FALSE", "jurabridge/machine/input board");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Sensor(s) Ready: %d", input_sensors_ready);
    }

    //Yes, we are intentionally runnint this twice
    if (update_flow_meter_timeout() || first_publication) {
      if (MQTT_ENABLED){ mqttpub_str(flowing ? "TRUE" : "FALSE", "jurabridge/parts/pump/flowing");}
      if (MODE_INVESTIGATION) ESP_LOGI(TAG,"Flow Meter Timout: %d", flowing);
    }
    if (UNHANDLED_INVESTIGATION && IC_INVESTIGATION){
      for (int i = 0; i < 4; i++){
        if (i == 1 ){ continue;}// no need to debug the flow meter; it;ll change a lot
        if (ic_val[i] != ic_val_prev[i] && ! ic_handled_change[i]){
          ESP_LOGI(TAG,"  ic[%d] %d -> %d",i,ic_val_prev[i], ic_val[i]);
        }
      }
    }
  } 

  // -------------------------------------------------------------------------------------------------
  // STATUS UPDATE
  // -------------------------------------------------------------------------------------------------
  if ((((loop_iterator % status_update_interval) == 0 ) && update_last_task()) || first_publication) {
    if (MQTT_ENABLED){
      if (last_task == ENUM_CUSTOM){
        mqttpub_str("CUSTOM AUTOMATION", "jurabridge/history");
      }else if (last_task == ENUM_START){
        mqttpub_str("START AUTOMATION", "jurabridge/history");
      }else if (last_task == ENUM_ESPRESSO){
        mqttpub_str("ESPRESSO", "jurabridge/history");
      } else if (last_task == ENUM_CAPPUCCINO){
        mqttpub_str("CAPPUCCINO", "jurabridge/history");
      } else if (last_task == ENUM_COFFEE){
        mqttpub_str("COFFEE", "jurabridge/history");
      } else if (last_task == ENUM_MACCHIATO){
        mqttpub_str("MACCHIATO", "jurabridge/history");
      } else if (last_task == ENUM_WATER){
        mqttpub_str("WATER", "jurabridge/history");
      } else if (last_task == ENUM_MILK){
        mqttpub_str("MILK", "jurabridge/history");
      } else if (last_task == ENUM_MCLEAN){
        mqttpub_str("MILK CLEAN", "jurabridge/history");
      } else if (last_task == ENUM_CLEAN){
        mqttpub_str("SYSTEM CLEAN", "jurabridge/history");
      } else if (last_task == ENUM_RINSE){
        mqttpub_str("WATER RINSE", "jurabridge/history");
      }
    }
    if (PREFERENCES_ENABLED) preferences.putInt(PREF_HISTORY, last_task);
  }

  if ((((loop_iterator % status_update_interval) == 0 ) && update_recommendation_state()) || first_publication) {
    if (MQTT_ENABLED){
      //recommendations separate
      if  (recommendation_state == ENUM_SYSTEM_RECOMMENDATION_RINSE ) {mqttpub_str("RINSE RECOMMENDED", "jurabridge/recommendation");}
      else if  (recommendation_state == ENUM_SYSTEM_RECOMMENDATION_MRINSE ) {mqttpub_str("MILK RINSE RECOMMENDED", "jurabridge/recommendation");}
      else if  (recommendation_state == ENUM_SYSTEM_RECOMMENDATION_MCLEAN ) {mqttpub_str("MILK CLEAN RECOMMENDED", "jurabridge/recommendation");}
      else if  (recommendation_state == ENUM_SYSTEM_RECOMMENDATION_CLEAN ) {mqttpub_str("SYSTEM CLEAN RECOMMENDED", "jurabridge/recommendation");}
      else if  (recommendation_state == ENUM_SYSTEM_RECOMMENDATION_CLEAN_SOON ) {mqttpub_str("SYSTEM CLEAN SOON", "jurabridge/recommendation");}
      else if  (recommendation_state == ENUM_SYSTEM_RECOMMENDATION_EMPTY_GROUNDS_SOON ) {mqttpub_str("EMPTY GROUNDS SOON", "jurabridge/recommendation");}
      else if  (recommendation_state == ENUM_SYSTEM_RECOMMENDATION_BEANS ) {mqttpub_str("FILL BEANS SOON", "jurabridge/recommendation");}
      else {mqttpub_str("NOMINAL", "jurabridge/recommendation");}
    }
  }

  if ((((loop_iterator % status_update_interval) == 0 ) && update_is_custom())  || first_publication) {
    if (MQTT_ENABLED){
      mqttpub_str(custom_execution ? "TRUE" : "FALSE", "jurabridge/machine/custom execution");
    }
  }

  if ((((loop_iterator % status_update_interval) == 0 ) && update_inferred_status()) || first_publication) {
    if (MQTT_ENABLED) { 
      //binary
      mqttpub_str(system_ready ? "TRUE" : "FALSE", "jurabridge/ready");

      //the order here is important
      if  (system_state == ENUM_SYSTEM_ERROR_TRAY) {mqttpub_str("TRAY MISSING", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_ERROR_COVER ) {mqttpub_str("BEAN COVER", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_ERROR_BEANS ) {mqttpub_str("FILL BEANS", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_ERROR_WATER ) {mqttpub_str("FILL WATER", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_ERROR_GROUNDS ) {mqttpub_str("EMPTY GROUNDS", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_GRINDER_ACTIVE) {mqttpub_str("GRINDING", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_WATER_PUMPING ) {mqttpub_str("WATER PUMPING", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_STEAM_PUMPING ) {mqttpub_str("STEAM PUMPING", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_HEATING_STEAM ) {mqttpub_str("STEAM HEATING", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_HEATING_WATER ) {mqttpub_str("WATER HEATING", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_CUSTOM_AUTOMATION ) {mqttpub_str("CUSTOM AUTOMATION", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_STANDBY_TEMP_HIGH ) {mqttpub_str("HIGH STANDBY", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_STANDBY_TEMP_COLD ) {mqttpub_str("COLD STANDBY", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_STANDBY_TEMP_LOW ) {mqttpub_str("LOW STANDBY", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_STANDBY ) {mqttpub_str("READY", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_CLEANING ) {mqttpub_str("CLEANING", "jurabridge/system");}
      else if  (system_state == ENUM_SYSTEM_NOT_READY) {mqttpub_str("NOT READY", "jurabridge/system");}     
      else { mqttpub_str("UNCAPTURED STATE", "jurabridge/system");}

    }
  };

  //UPDATE THE COUNTER
  first_publication = false;
  loop_iterator++;
}

// -------------------------------------------------------------------------------------------------
// INFERRED MACHINE STATE
// -------------------------------------------------------------------------------------------------
long total_automations(){
  return espresso_preparations + coffee_preparations + cappuccino_preparations + macchiato_preparations + milk_foam_preparations + water_preparations;
}

bool update_last_task() {
  if (last_task_previous != last_task){
    last_task_previous = last_task;
    return true;
  }
  return false;
}

bool update_is_custom() {
  if (last_custom_execution != custom_execution){
    last_custom_execution = custom_execution;
    return true;
  }
  return false;
}

bool update_recommendation_state(){
  //update recomemndation data
  long comparator;
  
  if (mrinse_recommended){
    comparator = ENUM_SYSTEM_RECOMMENDATION_MRINSE;   
  } else if (rinse_recommended){
    comparator = ENUM_SYSTEM_RECOMMENDATION_RINSE; 
  } else if (mclean_recommended){
    comparator = ENUM_SYSTEM_RECOMMENDATION_MCLEAN; 
  } else if (clean_recommended ){
    comparator = ENUM_SYSTEM_RECOMMENDATION_CLEAN; 
  } else if (preparations_since_last_clean > 90){
    comparator = ENUM_SYSTEM_RECOMMENDATION_CLEAN_SOON; 
  } else if (fill_beans_soon_inferred ){
    comparator = ENUM_SYSTEM_RECOMMENDATION_BEANS;  
  } else if (spent_grounds > 5){
    comparator = ENUM_SYSTEM_RECOMMENDATION_EMPTY_GROUNDS_SOON; 
  }
  
  //timeout for this status update
  bool timeout = false; if ((millis() - status_update) > STATUS_UPDATE_TIMEOUT_MS){timeout = true;}

  if (comparator != recommendation_state || timeout){
    recommendation_state = comparator;
    return true;
  }
  return false;
}

bool update_inferred_status(){
  //make sure that we don't have a race condition here
  std::lock_guard<std::mutex> lock(mutex);

  //is there a fault?
  bool hasFault = (drip_tray_error || hopper_cover_error || water_reservoir_error || powder_door_error || spent_grounds_error || is_cleaning);
  bool comparator = ( (!flowing) && brewgroup_init_position && input_sensors_ready && (!hasFault) && (! grinder_active) && (! pump_active)); 

  //calculate system state
  long system_state_new = 0;

  //timeout?
  bool timeout = false; if ((millis() - status_update) > STATUS_UPDATE_TIMEOUT_MS){timeout = true;}

  //set the sytsem state
  if (comparator){
   
    //ready states
    if (custom_execution == true) {
      
      system_state_new = ENUM_SYSTEM_CUSTOM_AUTOMATION;
    } else if (thermoblock_temperature >= 110){
      system_state_new = ENUM_SYSTEM_STANDBY_TEMP_HIGH;
    } else if (thermoblock_temperature < 60){
      system_state_new = ENUM_SYSTEM_STANDBY_TEMP_COLD; 
    } else if (thermoblock_temperature < 75){
      system_state_new = ENUM_SYSTEM_STANDBY_TEMP_LOW; 
    } else {
      system_state_new = ENUM_SYSTEM_STANDBY; 
    }

  }else{
     //============= errors
    if (is_cleaning){
      system_state_new = ENUM_SYSTEM_CLEANING; 

    }else if (drip_tray_error && ! pump_active){
      system_state_new = ENUM_SYSTEM_ERROR_TRAY;
    
    }else if (hopper_cover_error && ! pump_active){
      system_state_new = ENUM_SYSTEM_ERROR_COVER; 
    
    }else if (water_reservoir_error && ! pump_active){
      system_state_new = ENUM_SYSTEM_ERROR_WATER; 
    
    } else if (spent_grounds_error && ! pump_active){
      system_state_new = ENUM_SYSTEM_ERROR_GROUNDS;     
    
    } else if (fill_beans_error_inferred && ! pump_active){
      system_state_new = ENUM_SYSTEM_ERROR_BEANS; 

    } else if (grinder_active){
      system_state_new = ENUM_SYSTEM_GRINDER_ACTIVE; 
    
    } else if (thermoblock_active && steam_mode && !pump_active){
      system_state_new = ENUM_SYSTEM_HEATING_STEAM ; 

    } else if (thermoblock_active && water_mode && !pump_active){
      system_state_new = ENUM_SYSTEM_HEATING_WATER; 

    } else if (pump_active && water_mode){
      system_state_new = ENUM_SYSTEM_WATER_PUMPING; 

    } else if (pump_active && steam_mode){
      system_state_new = ENUM_SYSTEM_STEAM_PUMPING; 

    } else if (custom_execution == true) {

      system_state_new = ENUM_SYSTEM_CUSTOM_AUTOMATION; 
    
    } else {
      system_state_new = ENUM_SYSTEM_NOT_READY; 
    }
  }

  //did the system state change? 
  if ((system_state_new != system_state) || (comparator != system_ready) || timeout) {
    //system state changed!
    system_state_previous = system_state;
    system_state = system_state_new;

    //record the
    status_update = millis();

    //system has just become unavailable - starting processes!
    if ((comparator != system_ready) && comparator == false && !timeout ) {
       last_task = ENUM_START;  //mark the start of this activity!
    }

    //sustem ready binary
    system_ready = comparator; 

    return true;
  }
  return false;
}

// -------------------------------------------------------------------------------------------------
// DATA FROM EEPROM - WORD 1
// 64 DATA BITS + 3 TAG BITS
// -------------------------------------------------------------------------------------------------
// HEADING:    0   RT:
// EEPROM 1    3   4 BYTE HEX - ESPRESSO   //
// EEPROM 2    7   4 BYTE HEX - DOUBLE ESPRESSO  // [NONE FOR ENA 90]
// EEPROM 3    11  4 BYTE HEX - COFFEE   //
// EEPROM 4    15  4 BYTE HEX - DOUBLE COFFEE   // [NONE FOR ENA 90]
// EEPROM 5    19  4 BYTE HEX - CAPPUCCINO   //
// EEPROM 6    23  4 BYTE HEX - MACCHIATO   //
// EEPROM 7    27  4 BYTE HEX - ???
// EEPROM 8    31  4 BYTE HEX - RINSE CYCLES ???
// EEPROM 9    35  4 BYTE HEX - CLEANING CYCLES
// EEPROM 10   39  4 BYTE HEX - ??? DESCALING CYCLES??
// EEPROM 11   43  4 BYTE HEX - ???
// EEPROM 12   47  4 BYTE HEX - ???
// EEPROM 13   51  4 BYTE HEX - ???
// EEPROM 14   55  4 BYTE HEX - ???
// EEPROM 15   59  4 BYTE HEX - SPENT GROUNDS GROUNDS SINCE LAST TRAY EMPTY
// EEPROM 16   63  4 BYTE HEX - PREPARATIONS SINCE LAST CLEAN
// -------------------------------------------------------------------------------------------------

bool update_rt1 (){
  String comparator_string;
  comparator_string.reserve(100);
  
  String command; command.reserve(10); command ="RT:0000";
  comparator_string = cmd2jura(command);

  //find the location of the difference for marking & debugging
  bool hasChanged = false;

  //populate array for comparison
  for (int i = 0; i < 16; i++) {
    //end 
    int start = i * 4; int end = start + 4;
    
    //decompose string to integer array
    long val = strtol(comparator_string.substring(start, end).c_str(), NULL, 16);

    //reset change
    rt1_handled_change[i] = false; 
    rt1_val_prev[i] = rt1_val[i];

    //set hasChanged flag if we've timed out 
    if ((millis() - rt1_val_last_changed[i]) > RT1_UPDATE_TIMEOUT_MS){hasChanged = true;}

    //compare val storage
    if (rt1_val[i] != val){
      hasChanged = true; 
      rt1_initialized = true;
      rt1_val_prev_last_changed[i] = rt1_val_last_changed[i];
      rt1_val_last_changed[i] = millis();
      rt1_val[i] = val;
    }
  }
  return hasChanged;
}

//#############################################################
//
//  RT1 - eeprom 0th
//
//  if (MODE_INVESTIGATION) esressos 
//
//#############################################################

bool update_espresso_preparations (){
  long comparator = rt1_val[0]; 
  bool timeout = false; if ((millis() - rt1_val_last_changed[0]) > RT1_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != espresso_preparations || espresso_preparations == 0 || timeout){ 
    if (espresso_preparations > 0 && (!timeout)) {
      drainage_since_tray_empty += DEFAULT_DRAINAGE_ML; volume_since_reservoir_fill += last_dispense_volume;
      last_task = ENUM_ESPRESSO; 
      if (next_counter_is_custom_automation_part) { 
        last_task = ENUM_CUSTOM; 
        next_counter_is_custom_automation_part = false; 
      }
    }
    espresso_preparations = comparator; 
    rt1_handled_change[0] = true; 
    return true;
  }
  return false;
}

//#############################################################
//
//  RT1 - eeprom 3rd
//
//   coffee
//
//#############################################################

bool update_coffee_preparations (){
  long comparator =  rt1_val[2]; 
  bool timeout = false; if ((millis() - rt1_val_last_changed[2]) > RT1_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != coffee_preparations || coffee_preparations == 0 || timeout ){ 
    if (coffee_preparations > 0 && (!timeout)) { 
      drainage_since_tray_empty += DEFAULT_DRAINAGE_ML;  volume_since_reservoir_fill += last_dispense_volume;
      last_task = ENUM_COFFEE; 
      if (next_counter_is_custom_automation_part) {
        last_task = ENUM_CUSTOM; next_counter_is_custom_automation_part = false;
      }
    }
    rt1_handled_change[2] = true; 
    coffee_preparations = comparator; 
    return true; 
  }
  return false;
}

//#############################################################
//
//  RT1 - eeprom 5th
//
//   cappuccino
//
//#############################################################

bool update_cappuccino_preparations (){
  long comparator = rt1_val[4]; 
  bool timeout = false; if ((millis() - rt1_val_last_changed[4]) > RT1_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != cappuccino_preparations || cappuccino_preparations == 0 || timeout){ 
    if (cappuccino_preparations > 0 && (!timeout)) {
      mclean_recommended = true; 
      mrinse_recommended = true; 
      drainage_since_tray_empty += DEFAULT_DRAINAGE_ML;  volume_since_reservoir_fill += last_dispense_volume;
      last_task = ENUM_CAPPUCCINO; 
      if (next_counter_is_custom_automation_part) {
       last_task = ENUM_CUSTOM; 
       next_counter_is_custom_automation_part = false;
      }
    }
    cappuccino_preparations = comparator; 
    rt1_handled_change[4] = true; 
    return true; 
  }
  return false;
}

//#############################################################
//
//  RT1 - eeprom 5th
//
//   macchiato
//
//#############################################################

bool update_macchiato_preparations (){
  long comparator = rt1_val[5]; 
  bool timeout = false; if ((millis() - rt1_val_last_changed[5]) > RT1_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != macchiato_preparations || macchiato_preparations == 0 || timeout){
    if (macchiato_preparations > 0 && (!timeout)) { 
      mclean_recommended = true; 
      mrinse_recommended = true;
      drainage_since_tray_empty += DEFAULT_DRAINAGE_ML;  volume_since_reservoir_fill += last_dispense_volume;
      last_task = ENUM_MACCHIATO; 
      if (next_counter_is_custom_automation_part) { 
       last_task = ENUM_CUSTOM; 
       next_counter_is_custom_automation_part = false; 
      }
    }
    macchiato_preparations = comparator; 
    rt1_handled_change[5] = true; 
    return true; 
  }
  return false;
}

//#############################################################
//
//  RT1 - eeprom 7th
//
//  low pressure pump operations, mostly rinsing
//
//#############################################################

bool update_low_pressure_pump_operations (){
  long comparator = rt1_val[7]; 
  bool timeout = false; if ((millis() - rt1_val_last_changed[7]) > RT1_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != low_pressure_pump_operations || low_pressure_pump_operations == 0 || timeout){ 
    if (low_pressure_pump_operations > 0 && (!timeout)) { 
      drainage_since_tray_empty += last_dispense_volume;   volume_since_reservoir_fill += last_dispense_volume; //presume all drainage into tray    
      last_task = ENUM_RINSE; 
      if (next_counter_is_custom_automation_part) { 
        last_task = ENUM_CUSTOM; 
        next_counter_is_custom_automation_part = false; 
      }
    }
    low_pressure_pump_operations = comparator; 
    rt1_handled_change[7] = true; 
    return true; 
  }
  return false;
}

//#############################################################
//
//  RT1 - eeprom 8th
//
//   clean cycles
//
//#############################################################

bool update_clean_cycles (){
  long comparator = rt1_val[8]; 
  bool timeout = false; if ((millis() - rt1_val_last_changed[8]) > RT1_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != clean_cycles || clean_cycles == 0 || timeout){ 
    if (clean_cycles > 0 && (!timeout)) { 
      last_task = ENUM_CLEAN; 
      if (next_counter_is_custom_automation_part) { 
        last_task = ENUM_CUSTOM; 
        next_counter_is_custom_automation_part = false; 
      }
    }
    clean_cycles = comparator; 
    rt1_handled_change[8] = true;
    return true;
  }
  return false;
}

//#############################################################
//
//  RT1 - eeprom 14th
//
//  grounds
//
//#############################################################

bool update_spent_grounds (){
  long comparator = rt1_val[14]; // strtol(rt1_str.substring(56, 60).c_str(), NULL, 16);
  bool timeout = false; if ((millis() - rt1_val_last_changed[14]) > RT1_UPDATE_TIMEOUT_MS) {timeout = true;}
  // 8 appears to be the trigger threshold for sending an error.
  // remember: once the tray is removed for more than 5 - 10 seconds, grounds counter is 

  if (comparator != spent_grounds || timeout || spent_grounds == 99){ 
    
    //add to the total, but only if we actually are changing in response to 
    //an addition to the hopper
    if (comparator > 0 && spent_grounds < 10){ // less than 10 for 99 special flat and clean cycle at 250
      //7 g in low strength setting
      //10g in high setting: https://www.coffeeness.de/en/jura-a1-review/
      spent_beans_by_weight += (comparator - spent_grounds) * 8.5; //should get about 23 strong grinds per 200g coffe in hopper
    }
    
    //inferred beans error
    fill_beans_error_inferred = spent_beans_by_weight > 180;
    fill_beans_soon_inferred = spent_beans_by_weight > 130;

    //set current
    spent_grounds = comparator; 

    rt1_handled_change[14] = true;
    spent_grounds_error = (bool) (spent_grounds > 7); 
    is_cleaning = spent_grounds > 50; 
    return true; 
  }
  return false;
}

//#############################################################
//
//  RT1 - eeprom 15th
//
//  preparatinos since last clean
//
//#############################################################

bool update_preparations_since_last_clean (){
  long comparator = rt1_val[15]; 
  bool timeout = false; if ((millis() - rt1_val_last_changed[15] ) > RT1_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != preparations_since_last_clean || timeout){ 
    preparations_since_last_clean = comparator; 
    rt1_handled_change[15] = true;

    //180 preparations OR 90 switches on per manua (how to find?)l;
    clean_recommended = preparations_since_last_clean > 180;

    //need to find turn on, turn off cycles !!
    return true; 
  }
  return false;
}

// -------------------------------------------------------------------------------------------------
// DATA FROM EEPROM - WORD 2
// 64 DATA BITS + 3 TAG BITS
// -------------------------------------------------------------------------------------------------
// EEPROM 0    0   3   4 BYTE HEX - high pressure pump operations
// EEPROM 1    4   7   4 BYTE HEX - ???
// EEPROM 2    8   11  4 BYTE HEX - ???
// EEPROM 3    12  15  4 BYTE HEX - milk dispense count
// EEPROM 4    16  19  4 BYTE HEX - TOTAL HOT WATER DISTRIBUTIONS??
// EEPROM 5    20  23  4 BYTE HEX - grinder operations
// EEPROM 6    24  27  4 BYTE HEX - ???
// EEPROM 7    28  31  4 BYTE HEX - ???
// EEPROM 8    32  35  4 BYTE HEX - ???
// EEPROM 9   36  39  4 BYTE HEX - ???
// EEPROM 10   40  43  4 BYTE HEX - --- 90 dec when filter needed
// EEPROM 11   44  47  4 BYTE HEX - milk clean total
// EEPROM 12   48  51  4 BYTE HEX - ??? <-- number
// EEPROM 13   52  55  4 BYTE HEX - ??? 
// EEPROM 14   56  59  4 BYTE HEX - ???
// EEPROM 15   60  63  4 BYTE HEX - ???
// -------------------------------------------------------------------------------------------------

bool update_rt2 (){
  String comparator_string;
  comparator_string.reserve(100);

  String command; command.reserve(10); command ="RT:0010";
  comparator_string = cmd2jura(command);
  //find the location of the difference for marking & debugging
  bool hasChanged = false;

  //populate array for comparison
  for (int i = 0; i < 16; i++) {
    //end 
    int start = i * 4; int end = start + 4;
    
    //decompose string to integer array
    long val = strtol(comparator_string.substring(start, end).c_str(), NULL, 16);

    //reset change
    rt2_handled_change[i] = false; 
    rt2_val_prev[i] = rt2_val[i];

    //set hasChanged flag if we've timed out 
    if ((millis() - rt2_val_last_changed[i]) > RT2_UPDATE_TIMEOUT_MS){hasChanged = true;}
    
    //compare val storage
    if (rt2_val[i] != val){
      hasChanged = true;
      rt2_initialized = true;
      rt2_val_prev_last_changed[i] = rt2_val_last_changed[i];
      rt2_val_last_changed[i] = millis();
      rt2_val[i] = val;
    }
  }
  return hasChanged;
}

//#############################################################
//
//  RT2 - 0
//
//   milk dispenses
//
//#############################################################

bool update_high_pressure_pump_operations (){
  long comparator = rt2_val[0]; 
  bool timeout = false; if ((millis() - rt2_val_last_changed[0] ) > RT2_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != high_pressure_pump_operations || timeout){ 
    high_pressure_pump_operations = comparator; 

    //this is likely a milk rinse operation here; presume that it's in the tray!
    if (last_task == ENUM_START && !timeout) {
      drainage_since_tray_empty += last_dispense_volume;
    }

    rt2_handled_change[0] = true;
    return true; 
  }
  return false;
}

//#############################################################
//
//  RT2 - 3
//
//   milk dispenses
//
//#############################################################

bool update_milk_foam_preparations (){
  long comparator = rt2_val[3]; // strtol(rt2_str.substring(12, 16).c_str(), NULL, 16);
  bool timeout = false; if ((millis() - rt2_val_last_changed[3]) > RT2_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != milk_foam_preparations || timeout){
    if (milk_foam_preparations > 0 && (!timeout)) {
      drainage_since_tray_empty += DEFAULT_DRAINAGE_ML; 
      mclean_recommended = true; 
      mrinse_recommended = true;
     last_task = ENUM_MILK; 
    }
    milk_foam_preparations = comparator; 
    rt2_handled_change[3] = true;
    return true; 
  }
  return false;
}

//#############################################################
//
//  RT2 - 4
//
//   water dispenses
//
//#############################################################

bool update_water_preparations (){
  long comparator =  rt2_val[4]; 
  bool timeout = false; if ((millis() - rt2_val_last_changed[4]) > RT2_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != water_preparations || timeout){ 
    if (water_preparations > 0 && (!timeout)) { 
      last_task = ENUM_WATER; 
      drainage_since_tray_empty += DEFAULT_DRAINAGE_ML; 
      if (next_counter_is_custom_automation_part) { 
        last_task = ENUM_CUSTOM; 
        next_counter_is_custom_automation_part = false; 
      }
    }
    water_preparations = comparator; 
    rt2_handled_change[4] = true;
    return true;
  }
  return false;
}

//#############################################################
//
//  RT2 - 4
//
//   water dispenses
//
//#############################################################

bool update_grinder_operations (){
  long comparator =  rt2_val[5]; 
  bool timeout = false; if ((millis() - rt2_val_last_changed[5]) > RT2_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != grinder_operations || timeout){ 
    grinder_operations = comparator; 
    rt2_handled_change[5] = true;
    return true;
  }
  return false;
}

//#############################################################
//
//  RT2 - eeprom 3 (0th)
//
//   milk rinse total
//
//#############################################################

bool update_milk_clean_total (){
  long comparator = rt2_val[11];
  bool timeout = false; if ((millis() - rt2_val_last_changed[11]) > RT2_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != milk_clean_total || timeout){ 
    if (milk_clean_total > 0 && (!timeout)) {
      mclean_recommended = false; 
      mrinse_recommended = false;
      last_task = ENUM_MCLEAN;
      if (next_counter_is_custom_automation_part) { 
        last_task = ENUM_CUSTOM; 
        next_counter_is_custom_automation_part = false; 
      }
    }
    milk_clean_total = comparator; 
    rt2_handled_change[11] = true;
    return true;
  }
  return false;
}

// -------------------------------------------------------------------------------------------------
// DATA FROM EEPROM - WORD 3
// 64 DATA BITS + 3 TAG BITS
// -------------------------------------------------------------------------------------------------
// EEPROM 1    0   3   4 BYTE HEX - ???
// EEPROM 2    4   7   4 BYTE HEX - ???
// EEPROM 3    8   11  4 BYTE HEX - ???
// EEPROM 4    12  15  4 BYTE HEX - ???
// EEPROM 5    16  19  4 BYTE HEX - ???
// EEPROM 6    20  23  4 BYTE HEX - ???
// EEPROM 7    24  27  4 BYTE HEX - ???
// EEPROM 8    28  31  4 BYTE HEX - ???
// EEPROM 9    32  35  4 BYTE HEX - ???
// EEPROM 10   36  39  4 BYTE HEX - ???
// EEPROM 11   40  43  4 BYTE HEX - ???
// EEPROM 12   44  47  4 BYTE HEX - ???
// EEPROM 13   48  51  4 BYTE HEX - ??? 
// EEPROM 14   52  55  4 BYTE HEX - ??? 
// EEPROM 15   56  59  4 BYTE HEX - ???
// EEPROM 16   60  63  4 BYTE HEX - ???
// -------------------------------------------------------------------------------------------------
bool update_rt3 (){
  String comparator_string;
  comparator_string.reserve(100);
  String command; command.reserve(10); command ="RT:0020";
  comparator_string = cmd2jura(command);  

  //find the location of the difference for marking & debugging
  bool hasChanged = false;

  //populate array for comparison
  for (int i = 0; i < 16; i++) {
    //end 
    int start = i * 4; int end = start + 4;
    
    //decompose string to integer array
    long val = strtol(comparator_string.substring(start, end).c_str(), NULL, 16);

    //reset change
    rt3_handled_change[i] = false; 
    rt3_val_prev[i] = rt3_val[i];  
    
    //set hasChanged flag if we've timed out 
    if ((millis() - rt3_val_last_changed[i]) > RT3_UPDATE_TIMEOUT_MS){hasChanged = true;}
 
    //compare val storage
    if (rt3_val[i] != val){
      hasChanged = true; 
      rt3_initialized = true;
      rt3_val_prev_last_changed[i] = rt3_val_last_changed[i];
      rt3_val_last_changed[i] = millis();
      rt3_val[i] = val;
    }
  }
  return hasChanged;
}

// -------------------------------------------------------------------------------------------------
// DATA FROM AS DATA
// 6 DATA BYTES 
// AS: as:07340BB10000,07340BFC0001 
// -------------------------------------------------------------------------------------------------
// AS 1    0   3   4 BYTE HEX - ???
// AS 2    4   7   4 BYTE HEX - ??? //analog sensor readout for thermoblock temperature
// AS 3    8   11  4 BYTE HEX - ???
//          ,
// AS 4    13  15  4 BYTE HEX - ???
// AS 5    17  19  4 BYTE HEX - ??? //analog sensor readonout for ceramic block temperture
// AS 6    21  23  4 BYTE HEX - ???
// -------------------------------------------------------------------------------------------------
bool update_as (){
  String comparator_string;
  comparator_string.reserve(100);
  String command; command.reserve(10); command = "AS:";
  comparator_string = cmd2jura(command);  

  //find the location of the difference for marking & debugging
  bool hasChanged = false;

  //populate array for comparison
  for (int i = 0; i < 6; i++) {
    //end 
    int start = i * 4; int end = start + 4;
    if (i >= 3){
      //skip over the comma at 3 byte
      start++; end++;
    }
    //decompose string to integer array
    long val = strtol(comparator_string.substring(start, end).c_str(), NULL, 16);

    //reset change
    as_handled_change[i] = false; 
    as_val_prev[i] = as_val[i];  

    //set hasChanged flag if we've timed out 
    if ((millis() - as_val_last_changed[i]) > AS_UPDATE_TIMEOUT_MS){hasChanged = true;}
   
    //compare val storage
    if (as_val[i] != val){
      hasChanged = true; 
      as_initialized = true;
      as_val_prev_last_changed[i] = as_val_last_changed[i];
      as_val_last_changed[i] = millis();
      as_val[i] = val;
    }
  }
  return hasChanged;
}


// -------------------------------------------------------------------------------------------------
// DATA FROM HZ - Heizung Zustand = Heater State; hydraulisch? Hydrauliksystem? ?
// 44 DATA BITS + 3 TAG BITS
//
// Notes: this is a different output from other machines and machine types; specific to
// each machine? Potentially each element here is an important
// Example: 11011100000,027C,00DB,0000,03D5,0000,3,00000
//-------------------------------------------------------------------------------------------------
// HEADING:    0   3   HZ:
// HZ 0-10       0   11  11-BIT BITFIELD
// HZ 11        12  4   4 ??? DOESN'T SEEM TO CHANGE -----  
// HZ 12        17  4   4 STATE OF THE LINEAR ACTUATOR STEPPER MOTOR & OUTPUT VALVE
// HZ 13        22  4   4 LAST DISPENSE - reset to zero??
// HZ 14        27  4   4 BYTE HEX THERMOBLOCK TEMPERATURE
// HZ 15        32  4   4 BYTE HEX ??? == eeprom 9
// HZ 16        37  1   1 INTEGER CERAMIC VALVE POSITION STATE
// HZ 17-21     39  5   5-BIT BITFIELD FOR SWITCH ERRORS (DOOR)
// -------------------------------------------------------------------------------------------------

//#############################################################
//
//  HZ - index 0
//
//  Bitfield with various status elements. 
//
//#############################################################

bool update_hz (){
  String comparator_string;
  comparator_string.reserve(100);

  String command; command.reserve(10); command ="HZ:";
  comparator_string = cmd2jura(command);

  //change comparator sensitivity of temperature at byte 30 == 0 
  comparator_string[30] = '0';

  //clear out the final error values too - the last bitstring
  comparator_string[39] = '0'; //grounds hooper error
  comparator_string[41] = '0'; //water reservoir
  comparator_string[43] = '0'; //tray error

  bool hasChanged = false;

  //populate array for comparison
  for (int i = 0; i < comparator_string.length(); i++) {
    int start; int end; int index;

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

    //reset
    hz_handled_change [index] = false; 
    hz_val_prev[index] = hz_val[index];

    //set hasChanged flag if we've timed out 
    if ((millis() - hz_val_last_changed[i]) > HZ_UPDATE_TIMEOUT_MS){hasChanged = true;} 

    //compare val storage
    if (hz_val[index] != val && index < 22){
      hasChanged = true; 
      hz_initialized = true;
      hz_val_prev_last_changed[index] = hz_val_last_changed[index];
      hz_val_last_changed[index] = millis();
      hz_val[index] = val;
    }
  }

  return hasChanged;
}

// -------------------------------------------------------------------------------------------------
// DATA FROM "HZ:"" - HEIZUNGSZUSTAND HEATER STATUS
// -------------------------------------------------------------------------------------------------
// HEADING:    0   HZ:
// HZ 1        3   11  11-BIT BITFIELD
//               0 - ??? <--ALWAYS ON? 
//               1 - ???
//               2 - RINSE NEEDED/REQUIRED (ONLY GOES TO ZERO ONCE RINSED)
//               3 - ??? // maybe 1 when venturi is active?? MILK RELATED? (steam mode)
//               4 - READY RE: TEMPERATURE
//               5 - ??? // RINSE-RELATED; PUMP IS ON?? ?? <--ALWAYS ON
//               6 - ???    maybe 0 when brewing?? << juramote; brewer on (hot water mode??)
//               7 - ??? // NOT RINSE RELATED;
//               8 - ??? // NOT RINSE RELATED; 
//               9 - ??? 
//               10 - ???
//
// -------------------------------------------------------------------------------------------------
bool update_rinse_recommended(){
  long comparator = hz_val[2]; //strtol(hz_str.substring(2, 3).c_str(), NULL, 2); 
  bool timeout = false; if ((millis() - hz_val_last_changed[2]) > HZ_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != rinse_recommended || timeout){ 
    rinse_recommended = comparator; 
    hz_handled_change [2] = true;
    return true; 
  }
  return false;
}

bool update_thermoblock_preheated(){
  long comparator = hz_val[4]; 
  bool timeout = false; if ((millis() - hz_val_last_changed[4]) > HZ_UPDATE_TIMEOUT_MS) {timeout = true;}
  if (comparator != thermoblock_preheated || timeout){ 
    thermoblock_preheated = comparator; 
    hz_handled_change [4] = true;
    return true; 
  }
  return false;
}

bool update_brewgroup_init_position (){
  long comparator = hz_val[5]; 
  bool timeout = false; if ((millis() - hz_val_last_changed[5]) > HZ_UPDATE_TIMEOUT_MS) {timeout = true;} 
  if (comparator != brewgroup_init_position || timeout){ 
    brewgroup_init_position = comparator; 
    hz_handled_change [5] = true;
    return true; 
  }
  return false;
}

//#############################################################
//
//  HZ - index 2
//
//  dispense voluem  
//
//#############################################################

bool update_output_valve (){
  //     17   18   19   20
  //0b 0000 0000 0000 0000
  //          ^^ ^
  long comparator = hz_val[12]; // strtol(hz_str.substring(17, 21).c_str(), NULL, 16); 
  bool timeout = false; if ((millis() - hz_val_last_changed[12]) > HZ_UPDATE_TIMEOUT_MS) {timeout = true;} 

  if (comparator != output_valve || timeout){ 

    //set to comparator
    output_valve = comparator; 

    //output valve state
    isOutputValveBrewPosition = ((int)(output_valve) & (int) 512) != 0; //brew position
    isOutputValveFlushPosition = ((int)(output_valve) & (int) 256) != 0; //flush position? observation???
    isOutputValveDrainPosition = ((int)(output_valve) & (int) 128) != 0; //drain position? observation???
    isOutputValveBlockPosition = !isOutputValveBrewPosition && !isOutputValveDrainPosition && !isOutputValveFlushPosition;

    //NOTE: need to figure out what we're doing with the other bits here...
    hz_handled_change [12] = true;

    //presumption is that among four bytes, only 12 bits are relevant
    //the 3 (4?) most significant bits appear to be positions 
    //of the output valve. The first six significant bits appear 
    //to be the stepper controls for the stepper motor (AA,BB) ??

    return true; 
  }
  return false;
}

//#############################################################
//
//  HZ - index 4
//
//  dispense voluem  
//
//#############################################################

// based on measurements from rinse cycle; resets to zero unless there's
// some water already in the pipe system (i.e., whether this resets to
// zero depends on whether we're making coffee through the brew group
// or whether we're pushing through the main valve)

bool update_last_dispense_volume (){
  long comparator =  hz_val[13] * 0.52845528;
  bool timeout = false; if ((millis() - hz_val_last_changed[13]) > HZ_UPDATE_TIMEOUT_MS) {timeout = true;} 
  if (comparator != last_dispense_volume || last_dispense_volume == 0 || timeout){ 
    //if last_dispense is reset to zero, we have started a new brew controller
    hz_handled_change [13] = true;

    last_dispense_volume = comparator; 
    return true; 
  }
  return false;
}

//#############################################################
//
//  HZ - index 4
//
//  Thermoblock temperature is the fifth element of the 
//  comma delimited HZ command return values 
//
//#############################################################

// apparent scaler based on observation; compared output temperature to
// raw sensor output, presumption of linear measurement in deg C, also
// presuming that based on the run length from ceramic to the spout
// presumed a drop of 10% in temperature.ceramic to the spout

bool update_thermoblock_temperature () {
  long comparator =  hz_val[14] / 10.0; //strtol(hz_str.substring(27, 31).c_str(), NULL, 16) / 10.0; //based on observation
  bool timeout = false; if ((millis() - hz_val_last_changed[14]) > HZ_UPDATE_TIMEOUT_MS) {timeout = true;} 
  if (comparator != thermoblock_temperature || thermoblock_temperature == 0 || timeout){
    thermoblock_temperature = comparator; 
    hz_handled_change [14] = true;
    return true; 
  }
  return false;
}


//#############################################################
//
//  HZ - index 7
//
//  ceramic valve position
//
//#############################################################

bool update_ceramic_valve_position (){
  long comparator = hz_val[16]; //strtol(cs_str.substring(16, 17).c_str(), NULL, 16); // used to be under the cs
  bool timeout = false; if ((millis() - hz_val_last_changed[16]) > HZ_UPDATE_TIMEOUT_MS) {timeout = true;} 
  if (comparator != ceramic_valve_position || timeout) { 
    //ceramic valve position
    ceramic_valve_position = comparator;     
    
    //mark
    hz_handled_change [16] = true; 

    //extract binaries from this
    steam_mode = ((int) ceramic_valve_position & (int) 4) != 0;
    water_mode = ! steam_mode; 

    //position bits; go back to comparator here so ceramic valve postiion can be 
    //saved as is (with bit 4 asserted for steam)
    if (comparator > 4) {comparator = comparator - 4;}

    //resetting positions
    isCeramicValveUnknownPosition = false;
    isCeramicValveHotWaterCircuit= false;
    isCeramicValveBlocking= false;
    isCeramicValveCoffeeProductCircuit= false;
    isCeramicValveTransitioning= false;
    isCeramicValveSteamCircuitPosition= false;
    isCeramicValveVenturiPosition= false;
    isCeramicValvePressureRelief = false;

    //steam mode!
    if (steam_mode) {

      if (comparator == 0){
        isCeramicValvePressureRelief = true;
     
      }else if (comparator == 1){
        isCeramicValveSteamCircuitPosition= true;
     
      }else if (comparator == 2) {
        //clear out mrinse here
        mrinse_recommended = false;
        isCeramicValveVenturiPosition= true;
     
      }else if (comparator == 3){
        isCeramicValveTransitioning= true;
     
      }else{
        isCeramicValveBlocking= true;
      }

    } else {
      if (comparator == 0){
        isCeramicValveTransitioning= true;
      }else if (comparator == 1){
        isCeramicValveHotWaterCircuit= true;
      }else if (comparator == 2) {
        isCeramicValveBlocking= true;
      }else if (comparator == 3){
        isCeramicValveCoffeeProductCircuit= true;
      }else{
        isCeramicValveUnknownPosition = true; 
      }
    }
    return true; 
  }
  return false;
}

// -------------------------------------------------------------------------------------------------
// DATA FROM CS
// 64 DATA BITS + 3 TAG BITS
// -------------------------------------------------------------------------------------------------
// HEADING:    0   CS:
// CS0  1    0   4 BYTE HEX - ceramic valve temperature
// CS1  2    4   4 BYTE HEX - identical to HZ 3
// CS2  3    8   4 BYTE HEX - identical to HZ 4  
// CS3  4    12  4 BYTE HEX - ??? always ??? 104 ??? 
// CS4  5    16  4 BYTE HEX - 
//               [MSB - 16] identical to HZ 7; ceramic valve position
//               [MSB - 17] ??? status of thermoblock
//               [MSB - 18] ??? duty cycle of thermoblock
//               [MSB - 19] ??? duty cycle of thermoblock
// CS5  6    20  4 BYTE HEX - DUTY CYCLE // PWM of the pump??; NEEDS TO BE ZERO FOR READY;
//               [MSB - 20] 
//               [MSB - 21] ???
//               [MSB - 22] ???
//               [MSB - 23] ???
// CS6  7    24  4 BYTE HEX - DUTY CYCLE ///?? extended PWM of the pump??
// CS7  8    28  4 BYTE HEX - ???
// CS8  9    32  4 BYTE HEX - ???
// CS9  10   36  4 BYTE HEX - ???
// CS10 11   40  4 BYTE HEX - ???
// CS11 12   44  4 BYTE HEX - ???
// CS12 13   48  4 BYTE HEX - ???
// CS13 14   52  4 BYTE HEX - ???
// CS14 15   56  4 BYTE HEX - ???
// CS15 16   60  4 BYTE HEX - ???
// -------------------------------------------------------------------------------------------------

bool update_cs (){
  String comparator_string;
  comparator_string.reserve(100);

  String command; command.reserve(10); command ="CS:";
  comparator_string = cmd2jura(command);
  
  //change comparator sensitivity of temperature at byte 3 == 0 
  comparator_string[3] = '0';

  //zero out the HZ 3
  comparator_string[4] = '0';
  comparator_string[5] = '0';
  comparator_string[6] = '0';
  comparator_string[7] = '0';

  //zero out the HZ 4
  comparator_string[8] = '0';
  comparator_string[9] = '0';
  comparator_string[10] = '0';
  comparator_string[11] = '0';

   //find the location of the difference for marking & debugging
  bool hasChanged = false;

  //populate array for comparison
  for (int i = 0; i < 16; i++) {
    //end 
    int start = i * 4; int end = start + 4;
    
    //decompose string to integer array
    long val = strtol(comparator_string.substring(start, end).c_str(), NULL, 16);

    //rest
    cs_handled_change[i] = true;
    cs_val_prev[i] = cs_val[i];

    //set hasChanged flag if we've timed out 
    if ((millis() - cs_val_last_changed[i]) > CS_UPDATE_TIMEOUT_MS){hasChanged = true;}

    //compare val storage
    if (cs_val[i] != val){
      hasChanged = true; 
      cs_initialized = true;
      cs_val_prev_last_changed[i] = cs_val_last_changed[i];
      cs_val_last_changed[i] = millis();
      cs_val[i] = val;
    }
  }
  return hasChanged;
}

//#############################################################
//
//  CS - index 0
//
//  Ceramic Valve temperature is the zeroth element 
//
//#############################################################

bool update_ceramic_valve_temperature (){
  long comparator =  cs_val[0] / 10.0; 
  bool timeout = false; if ((millis() - cs_val_last_changed[16]) > CS_UPDATE_TIMEOUT_MS) {timeout = true;} 
  if (comparator != ceramic_valve_temperature || ceramic_valve_temperature == 0 || timeout){ 
    ceramic_valve_temperature = comparator; 
    cs_handled_change[0] = true;
    return true; 
  }
  return false;
}

//#############################################################
//
//  CS - index 4 (fifth element; last byte)
//
//  thermoblock 
//
//#############################################################

bool update_thermoblock_active (){
  bool comparator = thermoblock_duty_cycle > 0;
  if (comparator != thermoblock_active) { 
    thermoblock_active = comparator; 
    return true; 
  }
  return false;
}

  //#############################################################
//
//  CS - index 6
//
//  pwm of thermoblock? hammingWeight + two bytes
//
//#############################################################

bool update_thermoblock_duty_cycle (){
  //0x  16   17   18   19
  //0b#### #### #### ####
  //            ^^^^ ^^^^  
  //  ??xx ????

  if (UNHANDLED_INVESTIGATION && CS_INVESTIGATION && false){
    int index = 4; 

    //=== testing for bit 22
    int bit = 256; 
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }

    bit = 512; 
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }

    bit = 1024; 
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }

    bit = 2048; 
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }
  }

  //manually calculate hamming weight and normalize to duty cycle
  long comparator = (
      (cs_val[4] & 1 != 0) + 
      (cs_val[4] & 2 != 0) + 
      (cs_val[4] & 4 != 0) + 
      (cs_val[4] & 8 != 0) + 

      (cs_val[4] & 16 != 0) + 
      (cs_val[4] & 32 != 0) + 
      (cs_val[4] & 64 != 0) + 
      (cs_val[4] & 128 != 0)
    
  ) * 100 / 8.0;

  bool timeout = false; if ((millis() - cs_val_last_changed[4]) > CS_UPDATE_TIMEOUT_MS) {timeout = true;} 

  // duty cycle is multiple bytes??
  if (comparator != thermoblock_duty_cycle || thermoblock_duty_cycle == 0 || timeout){ 
    thermoblock_duty_cycle = comparator;  

    //NOTE: need to figure out what we're doing with the other bits here
    cs_handled_change[4] = true; 
    return true; 
  }
  return false;
}

//#############################################################
//
//  CS - index 6
//
//  pwm of pump? hammingWeight
//
//#############################################################

bool update_pump_duty_cycle (){
  //0x  20   21   22   23
  //0b#### #### #### ####
  //  ^^^^ ^^^^ 
  //            ????    <- still investigating

  if (UNHANDLED_INVESTIGATION && CS_INVESTIGATION){
    int index = 5; 

    //=== testing for bit 22
    int bit = 16; 
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }

    bit = 32;
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }

    bit = 64;
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }

    bit = 128;
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }
  }

  //manually calculate hamming weight and normalize to duty cycle; two digits seem to always be zero??
  long comparator = (
      ((cs_val[5] & 256) != 0) + 
      ((cs_val[5] & 512) != 0) + 
      ((cs_val[5] & 1024) != 0) + 
      ((cs_val[5] & 2048) != 0) + 

      ((cs_val[5] & 4096) != 0) + 
      ((cs_val[5] & 8192) != 0) + 
      ((cs_val[5] & 16384) != 0) + 
      ((cs_val[5] & 32768) != 0)
    
  ) * 100 / 6.0;

  //timeout
  bool timeout = false; if ((millis() - cs_val_last_changed[5]) > CS_UPDATE_TIMEOUT_MS) {timeout = true;} 
  bool off_timeout = millis() - pump_inactive_start > PUMP_TIMEOUT;

  // duty cycle is multiple bytes??
  if (comparator != pump_duty_cycle || pump_duty_cycle == 0 || timeout){
    //mark as this part handled
    cs_handled_change[5] = true;  

    if (comparator == 0 && off_timeout){
      //we only turn this off if we have been off for a period of time
      pump_duty_cycle = comparator;   
      return true; 

    }else if (comparator > 0){
      pump_duty_cycle = comparator;   
      return true; 

    }else if (comparator == 0){
      //we're off, and we were recently on, keep the duty cycle as is for the time being

      pump_inactive_start = millis();
      return false; 
    }
  }
  return false;
}

bool update_pump_status(){

  //by observation; seems 0000 is pump off, 1111 is pump on
  long comparator = (
      (cs_val[5] & 16) + 
      (cs_val[5] & 32) + 
      (cs_val[5] & 64) + 
      (cs_val[5] & 128));
    
  bool timeout = false; if ((millis() - cs_val_last_changed[5]) > CS_UPDATE_TIMEOUT_MS) {timeout = true;} 
  bool changed = false;

  //various conditions to determine whether the pump is on or not
  if (comparator == 240 || pump_duty_cycle > 0 || flowing){
    pump_active = true;  
    changed = true;

  //gotta be rigid about how we define pump off condition
  }else if (comparator == 0 && pump_duty_cycle == 0 ){
    pump_active = false; 
    changed = true;
  }

  // duty cycle is multiple bytes??
  if (comparator != pump_status || timeout){ 
    pump_status = comparator;  
    //NOTE: need to figure out what we're doing with the other bits here
    cs_handled_change[4] = true; 
    return true; 
  }

  //return 
  return changed;
}

//#############################################################
//
//  CS - index 6
//
//  partial ?? grinding
//
//#############################################################

bool update_grinder_active (){
  //0x  20   21   22   23
  //0b#### #### #### ##11 
  //                   ^^
  //                 ??    <- still investigating
  int bit; 
  int index = 5;
  if (UNHANDLED_INVESTIGATION && CS_INVESTIGATION){
    bit = 4; 
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }

    bit = 8;
    if ((cs_val_prev[index] & bit) != (cs_val[index] & bit) ){
      ESP_LOGI(TAG, "  cs[%d & %d] %d -> %d",
        index,
        bit,
        ((cs_val_prev[index] & bit) != 0),
        ((cs_val[index] & bit) != 0));
    }
  }

  //compartor 
  bool comparator = cs_val[5] & (int) 3 != 0;

  //timeout
  bool timeout = false; if ((millis() - cs_val_last_changed[5]) > CS_UPDATE_TIMEOUT_MS) {timeout = true;} 

  // duty cycle is multiple bytes??
  if (comparator != grinder_active || timeout){ 

    //ignore if we're not timing this bit out
    if (!timeout){
      //if we have a change, find the direction and set the appropriate timestamps
      if (grinder_active == false){
        //the change here is that comparator is true, meaning the grinder just turned on
        grinder_start = millis();
        grinder_end = 0;
        last_grinder_duration_ms = 0;


      }else{
        //the change here is that the comparator is now false, grinder has stopped after having been on
        grinder_end = millis();
        last_grinder_duration_ms = grinder_end - grinder_start;
      }
    }

    grinder_active = comparator;
    cs_handled_change[5] = true;  

    return true; 
  }
  return false;
}

// -------------------------------------------------------------------------------------------------
// DATA FROM "IC:"" - INPUT CONTROLLER
// -------------------------------------------------------------------------------------------------
// HEADING:    0   IC:9F02
// IC 2        4
//                 4 - hopper lid // hopper 1 = closed; 0 = open
//                 5 - water tank // water tank 0 = ok; 1 = problem
//                 6 - bypass doser // bypass doser chute 0 = ok; 1 = in use
//                 7 - drip tray // drip tray 1 = ok; 0 = open
//

// -------------------------------------------------------------------------------------------------
// DATA FROM "IC:"" - INPUT CONTROLLER
// -------------------------------------------------------------------------------------------------
// HEADING:    0   IC:
// IC 1        0   BINARY STATE SENSORS FOR UPPER CONTROL BOARD
// IC 2        1   FLOW METER
// IC 3        2   ALWAYS ZERO - MAYBE CHANGES FOR ATYPICALL ERRORS
// IC 4        3   Not always 2, but often two...? Changes with brew controller?? 
//                  2 = normal
//                  3 = water??
// -------------------------------------------------------------------------------------------------

bool update_ic (){
  String comparator_string;
  comparator_string.reserve(10);

  //is this working/
  String command; command.reserve(10); command = "IC:";
  comparator_string = cmd2jura(command);
  
  //find the location of the difference for marking & debugging
  bool hasChanged = false;

  //populate array for comparison
  for (int i = 0; i < 4; i++) {
    //end 
    int start = i; int end = start + 1;
    
    //decompose string to integer array
    long val = strtol(comparator_string.substring(start, end).c_str(), NULL, 16);

    //resetingg
    ic_handled_change[i] = false;  
    ic_val_prev[i] = ic_val[i];

    //set hasChanged flag if we've timed out 
    if ((millis() - ic_val_last_changed[i]) > IC_UPDATE_TIMEOUT_MS){hasChanged = true;}

    //compare val storage
    if (ic_val[i] != val){
      hasChanged = true; 
      ic_initialized = true;
      ic_val_prev_last_changed[i] = ic_val_last_changed[i];
      ic_val_last_changed[i] = millis();
      ic_val[i] = val;
    }
  }
  return hasChanged;
}

//#############################################################
//
//  IC - index 0
//
//  Input controller. Bean Hopper is AND 1
//
//#############################################################

bool update_hopper_cover_error (){
  bool comparator = ! (ic_val[0] & (int) 1) != 0; //! ((strtol(ic_str.substring(0, 1).c_str(), NULL, 16) & (int) 1) !=0 );
   //timeout
  bool timeout = false; if ((millis() - ic_val_last_changed[0]) > IC_UPDATE_TIMEOUT_MS) {timeout = true;} 
  if (comparator != hopper_cover_error || timeout){ 
    
    //reset once it times out while still open
    if ((hopper_cover_error == true && comparator == true) && timeout ) {
      spent_beans_by_weight = 1;
      fill_beans_soon_inferred = false;
      fill_beans_error_inferred = false;
    }

    hopper_cover_error = comparator; 
    if (timeout) {ic_val_prev_last_changed[0] = ic_val_last_changed[0]; ic_val_last_changed[0] = millis();}
    ic_handled_change[0] = true;  
    return true;
  }
  return false;
}

//#############################################################
//
//  IC - index 0
//
//  Input controller. Water reservoier is AND 2
//
//#############################################################

bool update_water_reservoir_error (){
  bool comparator = (ic_val[0] & (int) 2) != 0; //((strtol(ic_str.substring(0, 1).c_str(), NULL, 16) & (int) 2) !=0 );
  bool timeout = false; if ((millis() - ic_val_last_changed[0]) > IC_UPDATE_TIMEOUT_MS) {timeout = true;} 
  if (comparator != water_reservoir_error || timeout){
    water_reservoir_error = comparator; 
    ic_handled_change[0] = true;
    if (timeout) {ic_val_prev_last_changed[0] = ic_val_last_changed[0]; ic_val_last_changed[0] = millis();}
    return true; 
  }
  return false;
}

//#############################################################
//
//  IC - index 0
//
//  Input controller. Powder door is AND 4
//
//#############################################################

bool update_powder_door_error (){
  bool comparator = (ic_val[0] & (int) 4) != 0; // ((strtol(ic_str.substring(0, 1).c_str(), NULL, 16) & (int) 4) !=0 );
  bool timeout = false; if ((millis() - ic_val_last_changed[0]) > IC_UPDATE_TIMEOUT_MS) {timeout = true;} 
  if (comparator != powder_door_error || timeout){ 
    powder_door_error = comparator; 
    ic_handled_change[0] = true;
    if (timeout) {ic_val_prev_last_changed[0] = ic_val_last_changed[0]; ic_val_last_changed[0] = millis();}
    return true; 
  }
  return false;
}

//#############################################################
//
//  IC - index 0
//
//  Input controller. Drip tray is AND 8 
//
//#############################################################

bool update_volume_since_reservoir_fill() {
  //timeout!
  bool timeout = false; if ((millis() - ic_val_last_changed[0]) > (IC_UPDATE_TIMEOUT_MS)) {timeout = true;} 

  //set to zero if we have 
  bool short_timeout = false; if ((millis() - ic_val_last_changed[0]) > (IC_UPDATE_TIMEOUT_MS / 3)) {short_timeout = true;} 

  //mark fill as zero if we timeout sufficiently
  if (water_reservoir_error == true && short_timeout){
    volume_since_reservoir_fill = 0;
  }
  //has the drainage 
  if ((volume_since_reservoir_fill != volume_since_reservoir_fill_previous) || timeout){
    volume_since_reservoir_fill_previous = volume_since_reservoir_fill;
    return true;
  }
  return false;
}

bool update_volume_since_reservoir_fill_error(){
  //Timeout!
  bool timeout = false; if ((millis() - ic_val_last_changed[0]) > (IC_UPDATE_TIMEOUT_MS)) {timeout = true;} 

  //generalize the threshold here? 
  bool comparator = (volume_since_reservoir_fill > 900);

  if ((comparator != volume_since_reservoir_fill_error) || timeout){
    //update drip tray error??
    volume_since_reservoir_fill_error = comparator;
    volume_since_reservoir_fill_error_previous = volume_since_reservoir_fill_error;
    return true;
  }
  return false;
}

bool update_drainage_since_tray_empty() {
  //set to zero if we have 
  bool short_timeout = false; if ((millis() - ic_val_last_changed[0]) > (IC_UPDATE_TIMEOUT_MS / 2)) {short_timeout = true;} 
  if (drip_tray_error == true && short_timeout){
    drainage_since_tray_empty = 0;
  }

  //has the drainage 
  if (drainage_since_tray_empty != drainage_since_tray_empty_previous){
    drainage_since_tray_empty_previous = drainage_since_tray_empty;
    return true;
  }
  return false;
}

bool update_overdrainage_error(){
  //generalize the threshold here? 
  bool comparator = drainage_since_tray_empty > 300;

  if (comparator != overdrainage_error){
    //update drip tray error??
    overdrainage_error = comparator;
    overdrainage_error_previous = overdrainage_error;
    return true;
  }
  return false;
}

bool update_drip_tray_error (){
  bool comparator = ! (ic_val[0] & (int) 8) != 0; 
  bool timeout = false; if ((millis() - ic_val_last_changed[0]) > IC_UPDATE_TIMEOUT_MS) {timeout = true;} 
 
  if (comparator != drip_tray_error || timeout){ 
    drip_tray_error = comparator; 

    ic_handled_change[0] = true;
    if (timeout) {ic_val_prev_last_changed[0] = ic_val_last_changed[0]; ic_val_last_changed[0] = millis();}
    return true; 
  }
  return false;
}

  //#############################################################
//
//  IC - index 1
//
//  Input controller. Related to readiness; changes during brewing
//  Typically F when ready; can also be 5, 7
//  Oscillates between 3-5 or 5-7 during brewing 
//
//#############################################################

bool update_flow_meter_idle (){
  long comparator = ic_val[1] ; //strtol(ic_str.substring(1, 2).c_str(), NULL, 16);
  bool timeout = false; if ((millis() - ic_val_last_changed[1]) > IC_UPDATE_TIMEOUT_MS) {timeout = true;} 
  
  if (comparator != flow_meter_position || flow_meter_position == 0 || timeout){ 
    flow_meter_position = comparator;

    //breat out into bits - binary states ???
    int bit0 = ((int) flow_meter_position & (int) 1) != 0;
    int bit1 = ((int) flow_meter_position & (int) 2) != 0;
    int bit2 = ((int) flow_meter_position & (int) 4) != 0;
    int bit3 = ((int) flow_meter_position & (int) 8) != 0;

    //0b 0000 
    //   1111 = F -> definitely ready
    //   1011 = 11 
    //   1001 = 9
    //   0111 = 7
    //   0101 = 5
    //   0011 = 3
    //   0001 = 1    
    // NO EVENS EVER? 

    //known idle state
    if (bit0 && bit1 && bit2 && bit3){
      ic_handled_change[1] = true;
      flowing = false;
      return true; 
    }
  }
  return false;
}

//flow meter gets split, since it's a timeout-based sensor
bool update_flow_meter_timeout (){

  //check duration here again
  long duration = (ic_val_last_changed[1] - ic_val_prev_last_changed[1]); 
  long timeout = (millis() - ic_val_last_changed[1]) ; 
  bool timed_out =  (timeout > 5000);

  // threshold by observation, depends on speed of sampling
  // MAY be unsuitable if future chagnes add to processing load
  // of device. 

  if ((! flowing) && (duration <= 1500) && (duration > 5) && (flow_meter_position > 0) && (!timed_out)){
    ic_handled_change[1] = true;
    flowing = true; 
    return true;
  }
  
  //if currently flowing, but we are 4 seconds away from last chagne
  if (flowing && timed_out){
      ic_handled_change[1] = true;
      flowing = false; 
      return true;
  }

  //are we in the flowspace? 
  if (flowing){
    //despite that the change isn't actually handled; suppres debug output 
    //by setting this flag
    ic_handled_change[1] = true;
    return false;
  }

  return false;
}

//#############################################################
//
//  IC - index 3
//
//  Input controller. State??
//  does not appear to be consistent??
//
//#############################################################

bool update_input_board_state (){
  bool comparator = ic_val[3];
  bool timeout = false; if ((millis() - ic_val_last_changed[3]) > IC_UPDATE_TIMEOUT_MS) {timeout = true;} 

  //1 = heating but not yet preparing??
  //2 = ready state
  //3 = water
  if (comparator != input_board_state || timeout){ 
    input_board_state = comparator; 
    return true; 
  }
  return false;
}

bool update_input_sensors_ready(){
  bool comparator = (ic_val[3] == 2);
  bool timeout = false; if ((millis() - ic_val_last_changed[3]) > IC_UPDATE_TIMEOUT_MS) {timeout = true;} 

  if (comparator != input_sensors_ready || timeout){ 
    input_sensors_ready = comparator; 
    if (comparator) ic_handled_change[3] = true; //only if true; else we don't know what this bit means yet
    return true; 
  }
  return false;
}


//#############################################################
//
//  MQTT Connection Stuffs
//
//  mqtt response stuffs - subscribe to events? 
//
//#############################################################

void mqtt_reconnect() {
  // Loop until we're mqtt_reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect(TAG, MQTT_USER, MQTT_PASS, "jurabridge/power", 0, false, "FALSE" ,true)) {
      
      delay(200);
      client.subscribe("jurabridge/command");
      client.subscribe("jurabridge/menu");
      client.subscribe("homeassistant/status");

      //first messages
      mqttpub_str("TRUE", "jurabridge/ready");
      mqttpub_str("TRUE", "jurabridge/power");
      return;
    } else {
      delay(5000);
    }
  }
}

//mqtt callback goes here
void callback(char* topic, byte* message, unsigned int length) {
 
  //send messageString messageTemp;
  String messageTemp; 
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  bool success = false; 

  //process available commands
  if (String(topic) == "homeassistant/status") {
      //send stats 
      if (messageTemp == "online"){
        //home assisatnt has restarted
        //TODO: eventually, we'll do something more sophistocated here; kill the task, but not wifi?
        ESP.restart();
      }else {

      }

  //valid command or skip? 
  }else if (String(topic) == "jurabridge/command") {

    //are we executing another? 
    if (custom_execution){
      Serial.println("Custom automation rejected. Currently executing.");
      return;
    } else if ((millis() - custom_execution_completed) < INTERVAL_BETWEEN_CUSTOM_AUTOMATIONS ){
      Serial.println("Custom automation rejected. Executed too recently.");
      return;
    }

    //try to serialize string 
    StaticJsonDocument<2048> doc;
    DeserializationError json_deserialization_error = deserializeJson(doc, messageTemp.c_str());

    // if we failed, then we have an old format command!
    if (json_deserialization_error) {
      Serial.print(F("Deserialize False: "));
      Serial.println(json_deserialization_error.f_str());

    }else{

      //custom 
      custom_automation_start();
      
      //parse as JSON array
      JsonArray array = doc.as<JsonArray>();
      for(JsonVariant instruction : array) {

        /*
          ************************************************************************************************
          JURA ENA 90 Command Sequence API v0.1

          JSON OBJECT WITH COMMANDS IN ORDER, IN DESURED SEQUENCE

          each command is an array [cmd, value]

          - wait commands will halt a sequence until the specified condition is satisfied
            once the condition is satisfied, will immediately move to the next
            item in the sequence
          
          - make commands press the button as programmed in the machine

          - interrupt command will press the 'water' button to interrupt a curerntly-ongong automation

          ************************************************************************************************
          MQTT Format: 

          Topic:              jurabridge/command
          Message/Payload:    
                              [
                                ["id", "DOUBLE RISTRETTO"],
                                ["msg", " MORNING!"],
                                ["ready"],
                                ["delay", 1],
                                ["msg", " PRE-WARM "],
                                ["delay", 1],
                                ["water"],
                                ["pump"],
                                ["dispense", 100],
                                ["msg", "   WAIT"],
                                ["interrupt"],
                                ["ready"],
                                ["delay", 4],
                                ["msg", " EMPTY CUP"],
                                ["delay", 3],
                                ["msg", " EMPTY 5"],
                                ["delay", 1],
                                ["msg", " EMPTY 4"],
                                ["delay", 1],
                                ["msg", " EMPTY 3"],
                                ["delay", 1],
                                ["msg", " EMPTY 2"],
                                ["delay", 1],
                                ["msg", " EMPTY 1"],
                                ["delay", 3],
                                ["msg", " STEP 1"],
                                ["delay", 3],
                                ["ready"],
                                ["espresso"],
                                ["pump"],
                                ["dispense", 30],
                                ["interrupt"],
                                ["msg", "  STEP 2"],
                                ["delay", 2],
                                ["ready"],
                                ["espresso"],
                                ["pump"],
                                ["dispense", 30],
                                ["interrupt"],
                                ["msg", "    :)"],
                              ]

         ************************************************************************************************
        */

        //extract into comands 
        const char* command = instruction[0];

        // ------ CUSTOM DISPLAY?
        if (strcmp(command, "msg") == 0 )       {
          String message =  instruction[1].as<String>();
          String prefix = "DT:";
          cmd2jura(prefix + message);
        } 

        // ------ WAIT READY OPERATION
        if (strcmp(command, "ready") == 0 )     {if (wait_ready()){continue;}else{success=false;break;}}
        if (strcmp(command, "pump") == 0 )      {if (wait_pump_start()){continue;}else{success=false;break;}}
        if (strcmp(command, "heat") == 0 )      {if (wait_thermoblock_ready()){continue;}else{success=false;break;}}
        if (strcmp(command, "dispense") == 0 )  {if (wait_dispense_ml(instruction[1])){continue;}else{success=false;break;}}

        // ------ PRESS BUTTONS
        if (strcmp(command, "espresso") == 0 )  {press_button_espresso();}
        if (strcmp(command, "cappucino") == 0 ) {press_button_cappuccino();}
        if (strcmp(command, "coffee") == 0 )    {press_button_coffee();}
        if (strcmp(command, "macchiato") == 0 ) {press_button_macchiato();}
        if (strcmp(command, "water") == 0 )     {press_button_water();}
        if (strcmp(command, "milk") == 0 )      {press_button_milk();}

        // ------ NAME
        if (strcmp(command, "id") == 0 )        {
          custom_automation_name = instruction[1].as<String>
        }

        // ------ INTERRUPT
        if (strcmp(command, "interrupt") == 0 ) {press_button_water();}
        
        // ------ DELAY
        if (strcmp(command, "delay") == 0 )     {delay(instruction[1].as<unsigned int>());}

      }
      success = true;
      
      custom_automation_end();
    }
  //process available menu items
  }else if (String(topic) == "jurabridge/menu") {
    //open settings
    press_button_open_settings();
    delay(100);

    //select 'RINSE' menu
    press_button_select_menu_item();
    delay(100);

    //how far into the menu do we go?
    int menu_item;

    if (messageTemp ==  "mclean")       { menu_item = 0;}
    else if (messageTemp ==  "rinse")   { menu_item = 1;}
    else if (messageTemp ==  "mrinse")  { menu_item = 2;}
    else if (messageTemp ==  "clean")   { menu_item = 3;}
    else if (messageTemp ==  "filter")  { menu_item = 4;}
     
    //navigate to message 
    for (int i = 0; i < menu_item; i++){
      rotate_rotary_right();
      delay(100);
    }

    //press the selected button and be done with it
    press_button_select_menu_item();
    delay(100);

    //success?
    success = true;
  }

  //reset the display
  custom_automation_display_reset(success);
}