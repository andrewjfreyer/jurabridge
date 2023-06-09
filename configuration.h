//for inferring fill beams error
#define EMPTY_HOPPER_GRIND_DURATION 2000
#define INTERVAL_BETWEEN_CUSTOM_AUTOMATIONS 5000
#define DEFAULT_DRAINAGE_ML 15

//update interval values
#define RT1_UPDATE_INTERVAL_BREWING 99
#define RT2_UPDATE_INTERVAL_BREWING 99
#define RT3_UPDATE_INTERVAL_BREWING 99
#define RM_UPDATE_INTERVAL_BREWING 99
#define HZ_UPDATE_INTERVAL_BREWING 3
#define IC_UPDATE_INTERVAL_BREWING 1
#define CS_UPDATE_INTERVAL_BREWING 2
#define AS_UPDATE_INTERVAL_BREWING 99 //analog sensor readouts

//intervals when on standby (ready state)
#define RT1_UPDATE_INTERVAL_STANDBY 9
#define RT2_UPDATE_INTERVAL_STANDBY 11 
#define RT3_UPDATE_INTERVAL_STANDBY 99 //67
#define RM_UPDATE_INTERVAL_STANDBY 99  
#define HZ_UPDATE_INTERVAL_STANDBY 5
#define IC_UPDATE_INTERVAL_STANDBY 1 //should update as fast as possible  
#define CS_UPDATE_INTERVAL_STANDBY 1 //need for grinder status to transition from standby to ready
#define AS_UPDATE_INTERVAL_STANDBY 99  //analog sensor readouts

//force updates to the data items 
#define RT1_UPDATE_TIMEOUT_MS    180000
#define RT2_UPDATE_TIMEOUT_MS    180000
#define RT3_UPDATE_TIMEOUT_MS    180000
#define HZ_UPDATE_TIMEOUT_MS     20000
#define IC_UPDATE_TIMEOUT_MS     15000
#define CS_UPDATE_TIMEOUT_MS     30000
#define AS_UPDATE_TIMEOUT_MS     1800000
#define RM_UPDATE_TIMEOUT_MS     20000
#define STATUS_UPDATE_TIMEOUT_MS 10000

//state expirations
#define PUMP_TIMEOUT 1000
#define FLOW_SENSOR_TIMEOUT 2500

//Define pins for the board
#define GPIORX        16
#define GPIOTX        17
#define GPIOLEDPULSE  2 

//cheater enum 
#define ENUM_BOOT       0
#define ENUM_START      1
#define ENUM_ESPRESSO   2
#define ENUM_CAPPUCCINO 3
#define ENUM_COFFEE     4
#define ENUM_MACCHIATO  5
#define ENUM_WATER      6
#define ENUM_MILK       7
#define ENUM_MCLEAN     8
#define ENUM_CLEAN      9
#define ENUM_RINSE      10
#define ENUM_CUSTOM     11
#define ENUM_PREGROUND  12

//cheater enum
#define ENUM_SYSTEM_ERROR_TRAY  1
#define ENUM_SYSTEM_ERROR_COVER 2
#define ENUM_SYSTEM_ERROR_WATER 3
#define ENUM_SYSTEM_ERROR_GROUNDS 4
#define ENUM_SYSTEM_RECOMMENDATION_RINSE 5
#define ENUM_SYSTEM_RECOMMENDATION_MRINSE 6
#define ENUM_SYSTEM_RECOMMENDATION_MCLEAN 7
#define ENUM_SYSTEM_RECOMMENDATION_CLEAN 8
#define ENUM_SYSTEM_RECOMMENDATION_CLEAN_SOON 9
#define ENUM_SYSTEM_RECOMMENDATION_EMPTY_GROUNDS_SOON 10
#define ENUM_SYSTEM_STANDBY_TEMP_HIGH 11
#define ENUM_SYSTEM_STANDBY_TEMP_COLD 12
#define ENUM_SYSTEM_STANDBY_TEMP_LOW 13
#define ENUM_SYSTEM_STANDBY 14
#define ENUM_SYSTEM_CLEANING 15
#define ENUM_SYSTEM_HEATING 16
#define ENUM_SYSTEM_WATER_PUMPING 17
#define ENUM_SYSTEM_STEAM_PUMPING 18
#define ENUM_SYSTEM_NOT_READY 19
#define ENUM_SYSTEM_GRINDER_ACTIVE 20
#define ENUM_SYSTEM_ERROR_BEANS 21
#define ENUM_SYSTEM_CUSTOM_AUTOMATION 22
#define ENUM_SYSTEM_RECOMMENDATION_BEANS 23
#define ENUM_SYSTEM_HEATING_WATER 24
#define ENUM_SYSTEM_HEATING_STEAM 25
#define ENUM_SYSTEM_RECOMMENDATION_NONE 26


//Preferences 
#define PREF_BOOT_COUNT "PREF_X"
#define PREF_MCLEAN_ERR "PREF_0"
#define PREF_MRINSE_ERR "PREF_1"
#define PREF_DRIP_ERR "PREF_2"
#define PREF_BEAN_ERR "PREF_3"
#define PREF_WATER_ERR "PREF_4"
#define PREF_POWDER_DOOR "PREF_5"
#define PREF_GROUNDS_ERR "PREF_6"
#define PREF_RINSE_REC "PREF_7"
#define PREF_CLEAN_REC "PREF_8"
#define PREF_LAST_OUTVOLUME "PREF_9"
#define PREF_HISTORY "PREF_10"
#define PREF_NUM_GRINDS "PREF_11"
#define PREF_NUM_ESPRESSO "PREF_12"
#define PREF_NUM_COFFEE "PREF_13"
#define PREF_NUM_CAPPUCCINO "PREF_14"
#define PREF_NUM_MACCHIATO "PREF_15"
#define PREF_NUM_MILK "PREF_16"
#define PREF_NUM_WATER "PREF_17"
#define PREF_NUM_LP_OPS "PREF_18"
#define PREF_NUM_CLEANS "PREF_19"
#define PREF_NUM_GROUND "PREF_20"
#define PREF_NUM_PREP_SINCE "PREF_21"
#define PREF_NUM_MCLEAN "PREF_22"
#define PREF_NUM_HP_OPS "PREF_23"
#define PREF_POS_OUTVALVE "PREF_24"
#define PREF_POS_CVALVE "PREF_25"
#define PREF_FILLBEANS_ERR "PREF_26"
#define PREF_HOPPER_VOLUME "PREF_27"
#define PREF_NUM_DRAINAGE_SINCE_EMPTY "PREF_28"
#define PREF_VOL_SINCE_RESERVOIR_FILL "PREF_29"
#define PREF_NUM_PREGROUND "PREF_30"
