#ifndef JURAMACHINE_H
#define JURAMACHINE_H
#include "JuraConfiguration.h"
#include "JuraMemoryLine.h"
#include "JuraInputControlBoard.h"
#include "JuraHeatedBeverage.h"
#include "JuraSystemCircuitry.h"

/* string index (left to right) locations of useful values: DO NOT MODIFY!!! */

/* ----------------------- RT0 ----------------------- */
#define SUBSTR_INDEX_NUM_ESPRESSO_PREPARATIONS          0
//                                                      1 // probably double espresso count
#define SUBSTR_INDEX_NUM_COFFEE_PREPARATIONS            2
//                                                      3 // probably double coffee
#define SUBSTR_INDEX_NUM_CAPPUCCINO_PREPARATIONS        4
#define SUBSTR_INDEX_NUM_MACCHIATO_PREPARATIONS         5
#define SUBSTR_INDEX_NUM_PREGROUND_PREPARATIONS         6
#define SUBSTR_INDEX_NUM_LOW_PRESSURE_PUMP_OPERATIONS   7
#define SUBSTR_INDEX_NUM_CLEAN_SYSTEM_OPERATIONS        8
//                                                      9 // num descaling? 
#define SUBSTR_INDEX_NUM_DRIVE_MOTOR_OPERATIONS         10 
//                                                      11
//                                                      12 
//                                                      13 // water operations since descaling??
#define SUBSTR_INDEX_NUM_SPENT_GROUNDS                  14
#define SUBSTR_INDEX_NUM_PREPARATIONS_SINCE_LAST_CLEAN  15
/* ----------------------- RT1 ----------------------- */
#define SUBSTR_INDEX_NUM_HIGH_PRESSURE_PUMP_OPERATIONS  0 
//                                                      1 //changed from 83 to 0 after cleaning started; 19 -> 20 after rinse/turn on ???; 44 - 45 after turn on??
//                                                      2
#define SUBSTR_INDEX_NUM_MILK_FOAM_PREPARATIONS         3
#define SUBSTR_INDEX_NUM_WATER_PREPARATIONS             4
#define SUBSTR_INDEX_NUM_GRINDER_OPERATIONS             5
//                                                      6 //changed from 2 to 0 after cleaning started; (number of preparations after clean warning??)
//                                                      7
//                                                      8
//                                                      9
//                                                      10 // needed filter?? changed from 42 to 0 when filter replaced?? from 17 to 0 when filter replaced
#define SUBSTR_INDEX_NUM_CLEAN_MILK_SYSTEM_OPERATIONS   11
//                                                      12 // val = 195 ??? - Interrupted?? Preparations since last filter??; 204 - 205 when interrupted water; changed to 238 from 237 after filter; when interrupted water
//                                                      13
//                                                      14
//                                                      15
/* ----------------------- RT2 ----------------------- */
//                                                      0
//                                                      1
//                                                      2 // iterating from 51 to 52 after boot? 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8
#define SUBSTR_INDEX_HAS_FILTER                         9 // has a filter
//                                                      10 39-> when filter is inserted and flushed?? 
//                                                      11
//                                                      12 increased on water serve? from 317 to 318...? related to filter? , 340 5o 341 after 70ml dispensed;
//                                                      13
//                                                      14
//                                                      15
/* ----------------------- RT 3 ----------------------- */
//                                                      0
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8 
//                                                      9
//                                                      10
//                                                      11
//                                                      12
//                                                      13
//                                                      14
//                                                      15
/* ----------------------- RT 4 ----------------------- */
//                                                      0
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8 
//                                                      9
#define SUBSTR_INDEX_ML_OR_OZ                           10 // Oz or ML setting 
//                                                      11
//                                                      12
//                                                      13
//                                                      14
//                                                      15
/* ----------------------- RT 5 ----------------------- */
//                                                      0
//                                                      1
#define SUBSTR_INDEX_NUM_FILTERS                        2 // number of filters used? 
#define SUBSTR_INDEX_OFF_AFTER                          3 // off after min = 0.25hr 3073; 0.5hr 3074; 9 hours = 3108
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8 
//                                                      9
//                                                      10
//                                                      11
//                                                      12
//                                                      13
//                                                      14
//                                                      15
/* ----------------------- RT 6 ----------------------- */
//                                                      0
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8 
//                                                      9
//                                                      10
//                                                      11
//                                                      12
//                                                      13
//                                                      14
#define SUBSTR_INDEX_FILTER_USECOUNT                    15 //decreased from 69 to 0 when new filter was applied; filter life? 
/* ----------------------- RT 7 ----------------------- */
//                                                      0
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
#define SUBSTR_INDEX_WATER_DISPENSE_CONFIGURATION       8 // water diespnse configuration *5 = ml <--- NEEDS ADD
//                                                      9
//                                                      10
//                                                      11
//                                                      12 //incremented one when filter was changed; iterates after 15 minutes of being on? turn ons?
//                                                      13
//                                                      14
//                                                      15
/* ----------------------- RT 8 ----------------------- */
//                                                      0
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
#define SUBSTR_INDEX_MILK_DISPENSE_CONFIGURATION        8 // milk diespnse configuration *5 = ml 
//                                                      9
//                                                      10
//                                                      11
//                                                      12
//                                                      13
//                                                      14
//                                                      15
/* ----------------------- RT 9     ------------------- */
//                                                      0
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8 
//                                                      9
//                                                      10
//                                                      11
//                                                      12
//                                                      13
//                                                      14
//                                                      15
/* ----------------------- RTA/10 ------------------- */
//                                                      0
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
#define SUBSTR_INDEX_ESPRESSO_BREW_SETTINGS             8 // Espresso preparation settings 0b0000 0000 00000011 [strength 0-9] [1 = high; 0 = normal] [5x = ml dispense]
#define SUBSTR_INDEX_COFFEE_BREW_SETTINGS               9 // coffee  preparation settings  0b0000 0000 00000011 [strength 0-9] [1 = high; 0 = normal] [5x = ml dispense]
//                                                      10
//                                                      11
#define SUBSTR_INDEX_CAPPUCCINO_BREW_SETTINGS           12 // cappucino preparation settings  0b0000 0000 00000011 [strength 0-9] [1 = high; 0 = normal] [5x = ml dispense]
#define SUBSTR_INDEX_CAPPUCCINO_MILK_SETTINGS           13 // cappucino milk time & pause time0b0010 0000 0001 1110 = [delay/pause 0000 0000] [milk volume by time]
#define SUBSTR_INDEX_MACCHIATO_BREW_SETTINGS            14 // latte macchiato 0b0111 0001 00010010 [strength 0-9] [1 = high; 0 = normal] [5x = ml dispense]
#define SUBSTR_INDEX_MACCHIATO_MILK_SETTINGS            15 // latte macchiato milk time & pause time 0b0010 0000 0001 1110 = [delay/pause 0000 0000] [milk volume by time]
/* ----------------------- RTB/11 ------------------- */
//                                                      0
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8 
//                                                      9
//                                                      10
//                                                      11
//                                                      12
//                                                      13
//                                                      14
//                                                      15

/* ----------------------- RTC/12 ------------------- */
//                                                      0
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8 
//                                                      9
//                                                      10
//                                                      11 // changes to 0 when rinse selected from 32768
//                                                      12
//                                                      13 
//                                                      14 // changes after rinse from 50246 -> 19606; after tinse to 40052 -> 39993
//                                                      15
/* ----------------------- RTD/12 ------------------- */
//                                                      0 // filter off, turns from 332 to 2313; changes from 1558 to 1190 when unneeded rinse starts; returns to 1138; flush turns to  829 -> 1422
//                                                      1 // changes to 0 when rinse selected from 222; changes from 221 to 0 when unneeded rinse starts
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8 
//                                                      9
//                                                      10
//                                                      11 
//                                                      12
#define SUBSTR_INDEX_DRAINAGE_TRAY_VOLUME               13 // drainage tray volume! iterates by exactly "lastDispenseVolume" for rinse 
//                                                      14 
//                                                      15
/* ----------------------- RTE/13 ------------------- */
//                                                      0 // goes down by 55 or 56, then down by 20 (=5s?) during milk rinse??
//                                                      1
//                                                      2 
//                                                      3
//                                                      4
//                                                      5
//                                                      6
//                                                      7
//                                                      8 
//                                                      9
//                                                      10
//                                                      11
//                                                      12
//                                                      13 
//                                                      14
//                                                      15

/* ----------------------- IC ----------------------- */
/* IC: leftmost hex string index 0, results in 4 bits */
#define SUBSTR_INDEX_DRAINAGE_TRAY_REMOVED_IC           0 
#define SUBSTR_INDEX_BYPASS_DOSER_COVER_OPEN_IC         1 
#define SUBSTR_INDEX_WATER_RESERVOIR_NEEDS_FILL_IC      2 
#define SUBSTR_INDEX_BEAN_HOPPER_COVER_OPEN_IC          3 

/* IC: leftmost hex string index 0, results in 4 bits */
#define SUBSTR_INDEX_BREW_GROUP_ENCODER_STATE           4 /*-5*/
#define SUBSTR_INDEX_FLOW_METER_STATE                   6
#define SUBSTR_INDEX_OUTPUT_VALVE_ENCODER_STATE         14/*-15*/

/* ----------------------- HZ ----------------------- */
//                                                      0   //bin // machine on? seems to be on when machine is on?
//                                                      1   //bin
#define SUBSTR_INDEX_RINSE_BREW_GROUP_RECOMMENDED       2   //bin
//                                                      3   //bin
#define SUBSTR_INDEX_THERMOBLOCK_PREHEATED              4   //bin
#define SUBSTR_INDEX_THERMOBLOCK_READY                  5   //bin 
#define SUBSTR_INDEX_THERMOBLOCK_MILK_DISPENSE_MODE     6   //bin
//                                                      7   //bin
#define SUBSTR_INDEX_VENTURI_PUMPING                    8   //bin
//                                                      9   //bin  //something related to milk rinsing? filter? start a run ?? seems to be always on? 
//                                                      10  //bin
//                                                      11  //dec //643 on boot? (before filter replacement??) 644 on boot? mostly 664
#define SUBSTR_INDEX_OUTPUT_VALVE_POSITION              12  //dec
#define SUBSTR_INDEX_LAST_DISPENSE_PUMPED_WATER_VOLUME_ML            13  //dec
#define SUBSTR_INDEX_THERMOBLOCK_TEMPERATURE            14  //dec
#define SUBSTR_INDEX_LAST_BREW_OPERATION                15  //dec ?? 21 == last brew was high pessure; 15 = last brew was low pressure??
#define SUBSTR_INDEX_CERAMIC_VALVE_POSITION             16  //dec 1 byte
#define SUBSTR_INDEX_BEAN_HOPPER_COVER_OPEN_HZ          17  //bin
#define SUBSTR_INDEX_BYPASS_DOSER_COVER_OPEN_HZ         18  //bin
#define SUBSTR_INDEX_WATER_RESERVOIR_NEEDS_FILL_HZ      19  //bin
//                                                      20  //bin
#define SUBSTR_INDEX_DRAINAGE_TRAY_REMOVED_HZ           21  //bin
//                                                      22  //bin

/* ----------------------- CS ----------------------- */
#define SUBSTR_DEC_INDEX_THERMOBLOCK_TEMPERATURE        0   // dec = bin 0 - 15
#define SUBSTR_DEC_INDEX_OUTPUT_STATUS                  1   // dec = bin 16 - 31  
//                                                              // bin 25 - 31 ???
#define SUBSTR_DEC_INDEX_LAST_DISPENSE_PUMPED_WATER_VOLUME_ML        2   // dec = bin 32 - 47
#define SUBSTR_DEC_INDEX_CIRCUIT_READY_STATE            3   // dec = bin 48 - 63 /* 260 == normal*/
//                                                      4   // dec = bin 64 - 79
#define SUBSTR_BIN_INDEX_CERAMIC_VALVE_POSITION             64    // bin 64 - 67
#define SUBSTR_BIN_INDEX_THERMOBLOCK_DUTY_CYCLE             68    // bin 68 - 78

//                                                      5   // dec = bin 80 - 95
#define SUBSTR_BIN_INDEX_PUMP_DUTY_CYCLE                    80    // bin 80 - 90 
//                                                                // bin 91 - 93 // always 0x00???
#define SUBSTR_BIN_INDEX_GRINDER_DUTY_CYCLE                 94    // 12 bits
//                                                      6   //dec = bin 96 - 111
#define SUBSTR_BIN_INDEX_BREW_GROUP_DUTY_CYCLE              106  // bin 106 - 115

//                                                      7   //dec = bin 112 - 127
//                                                              //106 - 114 group???
#define SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_1                    118  // bin 118 - 127 (10)
//                                                      8   //dec = bin 128 - 143
#define SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_2                    130  // bin 130 - 138 (10)
#define SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_3                    142  // bin 143 - 149 (10)
//                                                      9   //dec = bin 144 - 159
//                                                      10  //dec = bin 160 - 175
#define SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_4                    152 // 10
#define SUBSTR_BIN_INDEX_UNKNOWN_MACHINE_CONFIGURATION_GROUP_5                    166  // bin 166 - 175 (10)
//                                                      11  //dec = bin 176 - 191


/* state histiory */
#define MAX_STATE_EVENT_HISTORY 20
#define MAX_DISPENSE_SAMPLES 20

/* forward declaration */
class JuraBridge; 

class JuraMachine {
public:
  JuraMachine(JuraBridge &, SemaphoreHandle_t &);
  void handlePoll(int);
  
  /* machine statess */
  int states[199];
  int last_changed[199]; 
  
  /* dispense limit*/
  void resetDispenseLimits();
  void setDispenseLimit(int, JuraMachineDispenseLimitType);
  void clearDispenseLimit(JuraMachineDispenseLimitType);

  /* addshot */
  void startAddShotPreparation();

  /* meta calculated states */
  int operationalStateHistory[MAX_STATE_EVENT_HISTORY];
  int dispenseHistory[MAX_STATE_EVENT_HISTORY];
  int flowRateHistory[MAX_STATE_EVENT_HISTORY];
  int durationHistory[MAX_STATE_EVENT_HISTORY];

  int temperatureAverageHistory[MAX_STATE_EVENT_HISTORY];
  int temperatureMaximumHistory[MAX_STATE_EVENT_HISTORY];
  int temperatureMinimumHistory[MAX_STATE_EVENT_HISTORY];

  /* during a dispense, take samples */
  int temperatureSamples[MAX_DISPENSE_SAMPLES];
  int flowRateSamples[MAX_DISPENSE_SAMPLES];
  int timestampSamples[MAX_DISPENSE_SAMPLES];

  /* non captured tracked states */
  int thermoblock_status = 0;
  int flow_meter_state = 0; 
  int flow_meter_rolling_state = 0; 
  int flow_meter_stationary_counter = 0;
  
  int output_valve_position = 0;
  int ceramic_valve_position = 0;

  /* calculated values */
  int is_cleaning_brew_group = false;
  int grounds_needs_empty = false;
  int system_is_ready = true; 

private:
  /* pointers & refs for service port comms */
  JuraBridge * _bridge;
  SemaphoreHandle_t & xMachineReadyStateVariableSemaphore;
  SemaphoreHandle_t xDispenseLimitSemaphore; 

  /* value/history characterization values */
  JuraMachineOperationalStateFlowRateType characterizeFlowRate(int);
  JuraMachineOperationalStateTemperatureType characterizeTemperature(int);
  JuraMachineOperationalStateDispenseQuantityType characterizeDispenseQuantity(int);

  /* classes for service port comms handling */
  JuraMemoryLine _rt0;
  JuraMemoryLine _rt1;
  JuraMemoryLine _rt2;

  /* Memory Line Research */
  JuraMemoryLine _rt3;
  JuraMemoryLine _rt4;
  JuraMemoryLine _rt5;
  JuraMemoryLine _rt6;
  JuraMemoryLine _rt7;
  JuraMemoryLine _rt8;
  JuraMemoryLine _rt9;
  JuraMemoryLine _rtA;
  JuraMemoryLine _rtB;
  JuraMemoryLine _rtC;
  JuraMemoryLine _rtD;
  JuraMemoryLine _rtE;
  JuraMemoryLine _rtF;

  JuraInputControlBoard _ic;
  JuraHeatedBeverage _hz;
  JuraSystemCircuitry _cs;

  /* ensuring values fall in ranges*/
  int filteredLong(int, int, int);
  void pushElement(int[], int, int);

  /*calculated value */
  bool recommendationStateHasChanged(int, int);
  bool handleErrorStateChange(int, int);
  bool handleMachineState(int, int);
  bool flowStateHasChanged(int, int);
  void determineReadyStateType();

  /* convenience function for handling common types (e.g., long in this case) */
  bool didUpdateJuraMemoryLineValue         (JuraMemoryLine *, int *, int, int, int);
  bool didUpdateJuraInputControlBoardValue  (int *, int, int, JuraInputBoardBinaryResponseInterpretation);
  bool didUpdateJuraHeatedBeverageValue     (int*, int, int, int);
  bool didUpdateJuraSystemCircuitValue      (int*, int, int, JuraSystemCircuitryBinaryResponseInterpretation, JuraSystemCircuitryResponseDataType);

  /* special repeated handlers*/
  void handleLastDispenseChange             (int);
  void handleBeanHopperCoverOpen            ();
  void handleThermoblockTemperature         (int);
  void handleBrewGroupOutput                ();
  void handleCeramicValve                   ();

};

#endif
