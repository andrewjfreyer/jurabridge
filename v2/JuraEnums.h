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
  /* known memory locations */
  RT0, 
  RT1, 
  RT2, 
  
  /* EEPROM investigations */
  RT3,
  RT4,
  RT5,
  RT6,
  RT7,
  RT8,
  RT9,
  RTA,
  RTB,
  RTC,
  RTD,
  RTE,
  RTF,

  /* knonwn hardware responses */
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
  FA_0C,  /* ena micro 90 - make milk foam */

  /* working memroy */
  RM00,
  RM01,
  RM02,
  RM03,
  RM04,
  RM05,
  RM06,
  RM07,
  RM08,
  RM09,
  RM0A,
  RM0B,
  RM0C,
  RM0D,
  RM0E,
  RM0F,
  RM10,
  RM11,
  RM12,
  RM13,
  RM14,
  RM15,
  RM16,
  RM17,
  RM18,
  RM19,
  RM1A,
  RM1B,
  RM1C,
  RM1D,
  RM1E,
  RM1F,
  RM20,
  RM21,
  RM22,
  RM23,
  RM24,
  RM25,
  RM26,
  RM27,
  RM28,
  RM29,
  RM2A,
  RM2B,
  RM2C,
  RM2D,
  RM2E,
  RM2F,
  RM30,
  RM31,
  RM32,
  RM33,
  RM34,
  RM35,
  RM36,
  RM37,
  RM38,
  RM39,
  RM3A,
  RM3B,
  RM3C,
  RM3D,
  RM3E,
  RM3F,
  RM40,
  RM41,
  RM42,
  RM43,
  RM44,
  RM45,
  RM46,
  RM47,
  RM48,
  RM49,
  RM4A,
  RM4B,
  RM4C,
  RM4D,
  RM4E,
  RM4F,
  RM50,
  RM51,
  RM52,
  RM53,
  RM54,
  RM55,
  RM56,
  RM57,
  RM58,
  RM59,
  RM5A,
  RM5B,
  RM5C,
  RM5D,
  RM5E,
  RM5F,
  RM60,
  RM61,
  RM62,
  RM63,
  RM64,
  RM65,
  RM66,
  RM67,
  RM68,
  RM69,
  RM6A,
  RM6B,
  RM6C,
  RM6D,
  RM6E,
  RM6F,
  RM70,
  RM71,
  RM72,
  RM73,
  RM74,
  RM75,
  RM76,
  RM77,
  RM78,
  RM79,
  RM7A,
  RM7B,
  RM7C,
  RM7D,
  RM7E,
  RM7F,
  RM80,
  RM81,
  RM82,
  RM83,
  RM84,
  RM85,
  RM86,
  RM87,
  RM88,
  RM89,
  RM8A,
  RM8B,
  RM8C,
  RM8D,
  RM8E,
  RM8F,
  RM90,
  RM91,
  RM92,
  RM93,
  RM94,
  RM95,
  RM96,
  RM97,
  RM98,
  RM99,
  RM9A,
  RM9B,
  RM9C,
  RM9D,
  RM9E,
  RM9F,
  RMA0,
  RMA1,
  RMA2,
  RMA3,
  RMA4,
  RMA5,
  RMA6,
  RMA7,
  RMA8,
  RMA9,
  RMAA,
  RMAB,
  RMAC,
  RMAD,
  RMAE,
  RMAF,
  RMB0,
  RMB1,
  RMB2,
  RMB3,
  RMB4,
  RMB5,
  RMB6,
  RMB7,
  RMB8,
  RMB9,
  RMBA,
  RMBB,
  RMBC,
  RMBD,
  RMBE,
  RMBF,
  RMC0,
  RMC1,
  RMC2,
  RMC3,
  RMC4,
  RMC5,
  RMC6,
  RMC7,
  RMC8,
  RMC9,
  RMCA,
  RMCB,
  RMCC,
  RMCD,
  RMCE,
  RMCF,
  RMD0,
  RMD1,
  RMD2,
  RMD3,
  RMD4,
  RMD5,
  RMD6,
  RMD7,
  RMD8,
  RMD9,
  RMDA,
  RMDB,
  RMDC,
  RMDD,
  RMDE,
  RMDF,
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

  /* RT5*/
  MachineSettingDispenseUnits,
  MachineSettingOffAfter,
  MachineSettingNumFilters,

  /* RT7*/
  MachineSettingWaterSettings,
  MachineSettingWaterDispenseVolume,

  /* RT 8*/
  MachineSettingMilkSettings,
  MachineSettingMilkDispenseTime,

  /* RTA*/
  MachineSettingEspressoSettings,
  MachineSettingEspressoTemperature, 
  MachineSettingEspressoDispenseVolume, 
  MachineSettingEspressoFlavor,

  MachineSettingCoffeeSettings,
  MachineSettingCoffeeTemperature, 
  MachineSettingCoffeeDispenseVolume, 
  MachineSettingCoffeeFlavor,

  MachineSettingCappuccinoSettings,
  MachineSettingCappuccinoTemperature, 
  MachineSettingCappuccinoDispenseVolume, 
  MachineSettingCappuccinoFlavor,

  MachineSettingCappuccinoExtendedSettings,
  MachineSettingCappuccinoMilkTime,
  MachineSettingCappuccinoMilkPause,

  MachineSettingMacchiatoSettings,
  MachineSettingMacchiatoTemperature, 
  MachineSettingMacchiatoDispenseVolume, 
  MachineSettingMacchiatoFlavor,
  
  MachineSettingMacchiatoExtendedSettings,
  MachineSettingMacchiatoMilkTime,
  MachineSettingMacchiatoMilkPause,

  /* RTD */
  DrainageTrayMeter,
  DrainageTrayLevel,
  DrainageTrayFull,          

  /* IC  */
  BeanHopperCoverOpen, 
  WaterReservoirNeedsFill,       
  BypassDoserCoverOpen,          
  DrainageTrayRemoved,  
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
  OverextractionLikely, 
  UnderextractionLikely,

  /* INFERRED STATES */
  RegulateThermoblockTemperatureRecommended,
  RinseMilkSystemRecommended, 
  CleanMilkSystemRecommended,
  CleanBrewGroupRecommendedSoon,
  CleanBrewGroupRecommended,
  HasDose,

  /* Limits */
  BrewLimit, 
  MilkLimit, 
  WaterLimit,
};

/* meta states */
enum class JuraMachineOperationalState { 
                                  Starting = 0,     /* */
                                  AddShotCommand,   /* custom command */
                                  Disconnected,     /* disconnected from the machine */
                                  Idle,             /* idle for some time */
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
                                  BridgeStarted, 
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
enum class JuraMachineSubsystem                     { Controller, Thermoblock, BrewGroup, Water, Dosing, Milksystem, Bridge };
enum class JuraMachineSubsystemAttributeType        { Error, Function, MeterValue, DataValue, StateValue, ServiceRecommendation};
enum class JuraMachineDispenseLimitType             { Water, Milk, Brew, None};
enum class JuraMachineStateDataType                 { Boolean, Integer, String };
enum class JuraMachineStateCategory                 { Config, Diagnostic };
enum class JuraMachineDeviceClass                   { Opening, Problem, Door, Moving, Power, Running, None };
enum class JuraMachineStateUnit                     { Preparation, Operation, Celcius, Milliliters, MicrolitersPerSecond, Grams, Seconds, Hours, Percent, Dose, Cycle, None};
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