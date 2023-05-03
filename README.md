# jurabridge ☕


## Description

This is an arduino project for bridging a Jura Ena Micro 90 to home automation platforms via MQTT. Main controller is an ESP32. A 3v to 5v level shifter is required between an available UART of the ESP32 to the debug/service port of the Jura. 

Optionally, a second ESP32 or other controller can be used to simulate the dual throw momentary switches that power on the input board and the power control boards, respectively. 

## MQTT Topics 

| Topic | Description |
| --- | --- |
| jurabridge/command | Post a custom automation or recipe formatted as an array of command arrays |
| jurabridge/counts/beans  | Approximate bean hopper percentage remaining |
| jurabridge/counts/cappuccino | Lifetime counts of cappuccino preparations |
| jurabridge/counts/cleans | Lifetime count of tablet clean operations |
| jurabridge/counts/coffee | Lifetime count of coffee preparations |
| jurabridge/counts/espresso | Lifetime count of espresso preparations |
| jurabridge/counts/grinder | Lifetime count of grinder operations |
| jurabridge/counts/grounds | Pucks in grounds hopper  |
| jurabridge/counts/high pressure operations | Lifetime count of high pressure pump cycles |
| jurabridge/counts/hot water | Lifetime count of hot water preparations  |
| jurabridge/counts/low pressure operations | Lifetime count of low pressure pump cycles |
| jurabridge/counts/macchiato | Lifetime count of macchiato preparations  |
| jurabridge/counts/milk clean | Lifetime count of m-clean operations  |
| jurabridge/counts/milk foam | Lifetime count of milk foam preparations  |
| jurabridge/counts/since clean | Preparations since last clean (clean recommended at 180)  |
| jurabridge/counts/total automations | Lifetime count total preparations  |
| jurabridge/counts/tray volume | Estimated ml in tray |
| jurabridge/counts/water tank/volume | Estimated fill percentage in reservoier |
| jurabridge/errors/beans | Beans hopper lid removed  |
| jurabridge/errors/grounds | Grounds needs emptying |
| jurabridge/errors/powder | Powder door / bypass doser is open |
| jurabridge/errors/reservoir low | Water fill required soon |
| jurabridge/errors/tray overfill | Tray likely needs emptying  |
| jurabridge/errors/tray removed | Tray is removed |
| jurabridge/errors/water | Water fill required; replace reservoir |
| jurabridge/history | Last automation/task completed |
| jurabridge/machine/brewgroup | Status position of brewgroup |
| jurabridge/machine/custom execution | Currently executing a custom program |
| jurabridge/machine/input board | Input board ready |
| jurabridge/machine/input board/state | Input board state |
| jurabridge/machine/last dispense | Volume of last dispense |
| jurabridge/machine/last grind duration | Approximate duration of last grind operation |
| jurabridge/menu | Post a message to manipulate buttons and menus |
| jurabridge/parts/ceramic valve/mode | Ceramic valve in steam or water mode / position |
| jurabridge/parts/ceramic valve/position | Ceramic valve position |
| jurabridge/parts/ceramic valve/temp | Ceramic valve temperature |
| jurabridge/parts/grinder/active | Grinder is currently grinding |
| jurabridge/parts/output valve/position | Output valve position |
| jurabridge/parts/pump/active | Pump is pumping |
| jurabridge/parts/pump/duty | Duty cycle of pump |
| jurabridge/parts/pump/flowing | Flow sensor indicates flow in hydraulic system |
| jurabridge/parts/pump/status | Status of pump |
| jurabridge/parts/thermoblock/active | Thermoblock is actively heating |
| jurabridge/parts/thermoblock/duty | Duty cycle of thermoblock (nearly always 100%) |
| jurabridge/parts/thermoblock/preheated | Thermoblock is preheated |
| jurabridge/parts/thermoblock/temp | Temperature of thermoblock |
| jurabridge/power | Last will message of "off" to indicate bridge is off (not machine) |
| jurabridge/ready | Boolean for whether system is ready or not  |
| jurabridge/recommendation | Current highest priority recommendation |
| jurabridge/recommendations/milk clean | Whether m-clean is recommended |
| jurabridge/recommendations/milk rinse | Whether m-rinse is reecommended |
| jurabridge/recommendations/rinse | Whether water rinse is recommended |
| jurabridge/system | Narrative description of system status |


*Under active development, README still being written...*