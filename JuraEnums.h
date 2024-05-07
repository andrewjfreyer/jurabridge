#ifndef JURAENUM_H
#define JURAENUM_H


/* mqtt menu topics */
#define MQTT_SUBTOPIC_FUNCTION  "/machine/function/"
#define MQTT_SUBTOPIC_MENU      "/machine/menu"         /* message: mclean, rinse, mrinse, clean, filter */
#define MQTT_BRIDGE_RESTART     "/bridge/restart"       /* message: none */
#define MQTT_CONFIG_SEND        "/configuration"        /* message: none */
#define HA_STATUS_MQTT          "homeassistant/status"  /* message: online (when HA reboots) */
#define MQTT_DISPENSE_CONFIG    "/limits"               /* message:  {"water":50, "brew" : 15, "milk" : 50, "add" : 1} */


/*

  name:         JuraMachineMenuCommand
  type:         enum
  description:  enumerated selectable menu items by button manipulation

*/
enum class JuraMachineMenuCommand {
  None,
  MRinse, 
  MClean, 
  Rinse, 
  Clean,
  Filter
};

/*

  name:         JuraServicePortCommand
  type:         enum
  description:  enumerated known commands to prevent unknown commands
                from being processed or received. 

*/

enum class JuraServicePortCommand {
  None = 0, 
  RT0, 
  RT1, 
  RT2,  // all historical dump commands and eeprom values for words 0 - 15
  RT3,  // investigation??
  HZ,   // dump real-time values related to hot drink preparation
  CS,   // dump real-time hardware data related to circuitry and systems
  IC,   // dump real-time status of input board

  /* known commands */
  FA_01, /* ena micro 90 - power off */
  FA_02, /* ena micro 90 - press rotary center button; confirm prompt */
  FA_03, /* ena micro 90 - open settings */
  FA_04, /* ena micro 90 - select menu item */
  FA_05, /* ena micro 90 - rotary right */
  FA_06, /* ena micro 90 - rotary left */
  FA_07, /* ena micro 90 - make espresso*/
  FA_08, /* ena micro 90 - make cappuccino */
  FA_09, /* ena micro 90 - make coffee */
  FA_0A, /* ena micro 90 - make macchiato */
  FA_0B, /* ena micro 90 - make hot water */
  FA_0C  /* ena micro 90 - make milk foam */
};

/*

  name:         JuraMachineStateIdentifier
  type:         enum
  description:  this enum represents classifiable data 
                from UART command responses. 

*/

enum class JuraMachineStateIdentifier {
  /* META STATES */
  OperationalState = 0,
  ReadyStateDetail,

  /* RT0 */
  NumEspresso,                            
  NumCoffee,                        
  NumCappuccino,                    
  NumMacchiato,                     
  NumPreground,                     
  NumLowPressurePumpOperations, 
  NumDriveMotorOperations,            
  NumBrewGroupCleanOperations,           
  NumSpentGrounds,                 
  NumPreparationsSinceLastBrewGroupClean, 

  /* RT1 */
  NumHighPressurePumpOperations,            
  NumMilkFoamPreparations,                     
  NumWaterPreparations,            
  NumGrinderOperations,            
  NumCleanMilkSystemOperations,   

  /* RT2 */
  HasFilter,                       

  /* IC  */
  BeanHopperCoverOpen, 
  WaterReservoirNeedsFill,       
  BypassDoserCoverOpen,          
  DrainageTrayRemoved,  
  DrainageTrayFull,          
  FlowState,                
  BrewGroupEncoderState,             
  OutputValveEncoderState,     
  OutputValveNominalPosition,
  HasError,
  HasMaintenanceRecommendation,
  BeanHopperEmpty,

  /* HZ */
  RinseBrewGroupRecommended,
  ThermoblockPreheated,
  ThermoblockReady,
  OutputValveIsBrewing,
  OutputValveIsFlushing, 
  OutputValveIsDraining, 
  OutputValveIsBlocking,
  LastDispenseType, 
  LastDispensePumpedWaterVolume,
  LastMilkDispenseVolume,
  LastWaterDispenseVolume,
  LastBrewDispenseVolume,
  LastDispenseMaxTemperature,
  LastDispenseMinTemperature,
  LastDispenseAvgTemperature,
  LastDispenseGrossTemperatureTrend,
  LastDispenseDuration,

  WaterPumpFlowRate,
  BrewGroupLastOperation,
  ThermoblockTemperature,
  ThermoblockStatus,
  ThermoblockColdMode,
  ThermoblockLowMode,
  ThermoblockHighMode,
  ThermoblockOvertemperature,
  ThermoblockSanitationTemperature,
  SystemSteamMode,
  SystemWaterMode,
  CeramicValvePosition,
  CeramicValveCondenserPosition,		
  CeramicValveHotWaterPosition,
  CeramicValvePressurizingPosition,	
  CeramicValveBrewingPosition,	
  CeramicValveChangingPosition,		
  CeramicValveSteamPosition,	
  CeramicValveVenturiPosition,	
  CeramicValvePressureReliefPosition,
  ThermoblockMilkDispenseMode,
  VenturiPumping,
  
  /* CALCULATED & EXTRACTED */
  OutputValvePosition,
  BrewGroupDriveShaftPosition,
  BrewGroupPosition,

  BrewGroupOutputStatus,
  SpentBeansByWeight,
  BeanHopperLevel,
  SpentGroundsLevel,
  DrainageSinceLastTrayEmpty,
  DrainageTrayLevel,

  /* CS */
  ThermoblockDutyCycle,
  ThermoblockActive,
  PumpDutyCycle,
  PumpActive,
  GrinderDutyCycle,
  GrinderActive,
  BrewGroupDutyCycle,
  BrewGroupActive,

  /* CALCULATED */
  BrewGroupIsRinsing,
  BrewGroupIsReady,
  BrewGroupIsLocked,
  BrewGroupIsOpen,
  BrewProgramNumericState,
  BrewProgramIsReady,

  /* CALCULATED */
  BrewProgramIsCleaning,
  GroundsNeedsEmpty,
  SystemIsReady,

  /* INFERRED STATES */
  RinseMilkSystemRecommended, 
  CleanMilkSystemRecommended,
  CleanBrewGroupRecommendedSoon,
  CleanBrewGroupRecommended,
  HasDoes,

  /* Limits */
  BrewLimit, 
  MilkLimit, 
  WaterLimit,
};

/* meta states */
enum class JuraMachineOperationalState { 
                                  Starting = 0,     /* */
                                  AddShotCommand, /* custom command */
                                  Disconnected,     /* disconnected from the machine */
                                  Ready,            /* ready to brew */
                                  Finishing,        /* finishing before marking as ready */
                                  BlockingError,    /* not ready to brew until an error is cleared */
                                  GrindOperation,   /* preparing to brew; currently grinding */ 
                                  BrewOperation,    /* not ready to brew; currently brewing */
                                  WaterOperation,   /* not ready to brew; currently making water hot */
                                  RinseOperation,   /* not ready to brew; currently rinsing*/
                                  MilkOperation,    /* not ready to brew; currently making milk something */
                                  HeatingOperation, /* not ready to brew; temperature too low*/
                                  Cleaning,         /* not ready to brew; cleaning */
                                  AwaitRotaryInput, /* awiting the user */
                                  ProgramPause,      /* awiting the user */
                                  Unknown,          /* uknown state */
                                };

/* these states describe what the machine most recently did; finished a product? cleaned? rinsed? */
enum class JuraMachineReadyState { 
                                  ExecutingOperation = 0,
                                  ReadyForNext,
                                  ErrorCorrected ,
                                  EspressoReady,
                                  CoffeeReady,
                                  CappuccinoReady,
                                  MacciattoReady,
                                  MilkFoamReady,
                                  HotWaterReady,
                                  CleanBrewGroupComplete,
                                  RinseBrewGroupComplete,
                                  CleanMilkSystemComplete,
                                  RinseMilkSystemComplete,
                                  FilterReplaceComplete,
                                  ReplacedDrainageTrayAndEmptyKnockbox,
                                  FilledBeans,
                                  FilledWater,
                                  ClosedBypassDoserDoor,
                                  Unknown,
                                  None,
                                };

/* structures & enums*/
enum class JuraMachineSubsystem                     { Controller, Thermoblock, Brewgroup, Water, Dosing, Milksystem, Bridge };
enum class JuraMachineSubsystemAttributeType        { Error, Function, MeterValue, DataValue, StateValue, ServiceRecommendation};
enum class JuraMachineDispenseLimitType             { Water, Milk, Brew, None};
enum class JuraMachineStateDataType                 { Boolean, Integer, String };
enum class JuraMachineStateCategory                 { Config, Diagnostic };
enum class JuraMachineDeviceClass                   { Opening, Problem, Door, Moving, Power, Running, None };
enum class JuraMachineStateUnit                     { Preparation, Operation, Celcius, Milliliters, MicrolitersPerSecond, Grams, Seconds, Percent, Dose, Cycle, None};
enum class JuraMachineStateIcon                     { Info, Coffee, Counter, Water, Alert, Check, Thermometer, Valve, Speedometer, Function};
enum class JuraEntityEnabled                        { Yes, No };
enum class JuraEntitySerialPrintable                { Yes, No };
enum class JuraEntityNonvolatile                    { Yes, No };
enum class JuraEntityAvailabilityFollowsReadyState  { Yes, No };

/*characterizing operational states */
enum class JuraMachineOperationalStateTemperatureType          { Steam, High, Normal, Low, Undeterminable};
enum class JuraMachineOperationalStateFlowRateType             { Unrestricted, PressureBrew, Venturi, Undeterminable};
enum class JuraMachineOperationalStateDispenseQuantityType     { Beverage, MilkRinse, BrewGroupRinse, FilterFlush, NoDispense, Undeterminable };

/* configuration structure for tracked entities */
struct JuraEntityConfiguration {
  JuraMachineStateIdentifier state;
  const char name[255];
  const char entity_id[255];
  JuraMachineStateDataType dataType;
  JuraMachineStateCategory category;
  JuraMachineDeviceClass deviceClass;
  JuraMachineStateIcon icon;
  JuraMachineStateUnit unitOfMeasure;
  JuraEntityEnabled enabled;
  JuraEntitySerialPrintable serialPrintable;
  JuraEntityAvailabilityFollowsReadyState availabilityFollowsMachine; 
  JuraMachineSubsystemAttributeType subsystemAttributeType;
  JuraMachineSubsystem machineSubsystemType;
  JuraEntityNonvolatile nonvolatile;
  int defaultValue;
};

enum class JuraFunctionIdentifier {
  /* physical buttons */
  Startup = 0,
  PowerOff,
  ConfirmDisplayPrompt,
  SettingsMenu,
  SelectMenuItem,
  RotaryRight,
  RotaryLeft,
  MakeEspresso,
  MakeCappuccino,
  MakeCoffee,
  MakeMacchiato,
  MakeHotWater,
  MakeMilkFoam,

  /* custom functions */
  RefreshMQTTConfiguration,

  /* menu items */
  MenuSelectRinseBrewGroup, 
  MenuSelectRinseMilkSystem,
  MenuSelectCleanMilkSystem,
  MenuSelectCleanBrewGroup, 
  MenuFilterSettings,
  MenuDescaleSettings,

  /**/
  None
};

struct JuraMachineFunctionEntityConfiguration {
  JuraFunctionIdentifier function;
  const char name[255];
  const char entity_id[255];
  const char command_topic [255];
  JuraMachineStateIcon icon;
  JuraEntityEnabled enabled;
  JuraEntitySerialPrintable serialPrintable;
  JuraEntityAvailabilityFollowsReadyState availabilityFollowsMachine; 
  JuraServicePortCommand command;
  JuraMachineMenuCommand menu_command;
  JuraMachineSubsystemAttributeType subsystemAttributeType;
  JuraMachineSubsystem machineSubsystemType;
  JuraMachineReadyState associatedReadyState;
};

struct JuraCustomMenuItemConfiguration {
  const char name[255];
  const char topic[255];
  const char payload[255];
};

#endif