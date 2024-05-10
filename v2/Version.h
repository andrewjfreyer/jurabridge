#ifndef VERSION_H
#define VERSION_H

/* current version */
#define VERSION_STR         "0.7.5" /* reported via mqtt device discovery as version number*/
#define VERSION_INT         4       /* iteration of this value will trigger an automatic mqtt configuration update on boot*/
#define VERSION_MAJOR_STR   "7"     /* needs to be string type; displayed in the display*/

/*
0.7.5 - bugfixes relating to overextraction warning temperatures
0.7.4 - addshot support for preparation agnostic usemode
0.7.3 - fix logic error in error handler
0.7.2 - overextraction/underextraction warnings
0.7.1 - typoswatting
0.7.0 - version up; bug checking and feature adding 
---
0.6.27 - button hold feedback on machine
0.6.26 - update dispense limits
0.6.25 - dispense limit timeout 
0.6.24 - rename enums with titlecase
0.6.23 - button functionality returned; custom recipe menu returned; supports {"water":50, "brew" : 15, "milk" : 50 , "add" : 1}
0.6.22 - update all nums for display in UI
0.6.21 - maintenance flag 
0.6.20 - typo corrections 
0.6.19 - remove double espresso, add "add" feature: {"water":50, "brew" : 15, "milk" : 50 , "add" : 1} (max of add 2 shots)
0.6.18 - fix double-espresso; document machine.cpp
0.6.17 - remove button handling; shift to ISR topology necssary??
0.6.16 - button shortcut for double espresso? 
0.6.15 - button interrupt for MQTT insructed things 
0.6.14 - json body of commands follows: {"water":50, "brew" : 15, "milk" : 50} <-- NEW FEATURE; SIMPLFIIED RECIPE
0.6.13 - separated dispense limits
0.6.12 - timeout for bean refill re-add
0.6.11 - bugfixes for iteration counter re: rt0
0.6.10 - calibration coefficients for each dispense type
0.6.9 - semaphore handling of global ready state setting; set and release from same task
0.6.8 - bugfixes
0.6.7 - configuration debugging
0.6.6 - initial configuration syncrhnoization 
0.6.5 - debug issue with Wifi handling
0.6.4 - vTaskDelayMillisconds macro added; annotatoins of  functions from main .ino
0.6.3 - refactor bugfixes; leverage version number as data for forcing recongiguration
0.6.2 - code cleanup & refactoring; mqtt ready flags; re-add LED blinker
0.6.1 - reduce thresholds for system clean & system clean recommend; remove core pinning for MQTT and UART - faster? 
0.6.0 - refactor mqtt and wifi connectivity ... 
---
0.5.35 - remove pinning to core; 
0.5.34 - mqtt semaphore AND uqrt semaphore? 
0.5.33 - continued problems with semaphore signaling; task suspension seems best? 
0.5.32 - retire use of arduino-specific delay function
0.5.31 - unacceptable delay for vTask suspend; diispense specific task? 
0.5.30 - work on button as confimr
0.5.29 - button as confirm? 
0.5.28 - mqtt messaging supprot for dispense selection for all default operations
0.5.27 - move dispense from bridge interrupt to machine object interrupt 
0.5.26 - special recipe from MQTT;
0.5.25 - discontinue button work; use for debugging? use for information displayu? 
0.5.24 - button work 
0.5.23 - button handling initial commit; fix temperature 
0.5.22 - reorganize defaults
0.5.21 - finzligin step made imsultaenous with "ready" state reporting; loop prevention
0.5.20 - add fianlizing step preceding "READY"
0.5.19 - add time/duration for each dispense operation
0.5.18 - millins timestamping for all
0.5.18 - functionizing repeated handlePoll methods
0.5.17 - add flow, temperature, and volume characterizations
0.5.16 - flow rate analysis; post-hoc state determination 
0.5.15 - reboot counter-based reporting
0.5.14 - averages for flow rate; report at dL/s
0.5.13 - milk rinse and milk clean flag move to operational state processor
0.5.12 - animation across display; fix milk counters 
0.5.11 - split brew group into two words universally
0.5.10 - rename "system clean" to brew group clean
0.5.9 - correct bugs with system clean meter
0.5.8 - correct bugs with milk rinse flag
0.5.7 - send config in response to message to MQTT_ROOT MQTT_CONFIG_SEND
0.5.6 - availability follows main machine  
0.5.5 - rename to jurabridge, rename legacy 
0.5.4 - separate version history to own file
0.5.3 - roll back mqtt request aggregation (too slow)
0.5.2 - aggreate mqtt requests to send together 
0.5.1 - task suspend during mqtt event processing; 
---
0.4.82 - enum for specific commands; prevent bad commands from accidentally being sent 
0.4.81 - button press emulation; enums for jura specific functions
0.4.80 - additional data
0.4.75 - additional sensors
0.4.74 - additional sensors
0.4.73 - jura configuration by enum/array
0.4.72 - wifi debugging; mqtt debugging 
0.4.71 - separate mqtt process from uart process
0.4.70 - **** COMPLETE REFACTOR ****
0.4.64 - update github
0.4.63 - wait for button option in custom scripts
0.4.62 - more clear recipe instructions
0.4.61 - bugfixes
0.4.6 - improve speed of flow sensor timeout; add grounds counter
0.4.5 - secret menu dynamically loads from MQTT!
0.4.4 - mqtt secret menu; major feature update
0.4.31 - fix bug with pump active
0.4.30 - secret menu testing
0.4.29 - tank volume as percentage only if not plumbed
0.4.28 - bugfixes for calculated values
0.4.27 - custom execution id fixes? test fix of "recommend system clean" early 
0.4.26 - custom execution id ; TODO: common terminology for recipe, automation, tastk wahttever
0.4.25 - custom recipe work
0.4.24 - remove custom hardcoded recipes, MQTT only
0.4.23 - update mqtt buffer size
0.4.22 - recipe format update 
0.4.21 - bugfix re: calculating estimated hopper volume remaining
0.4.20 - json custom recipes!
0.4.15 - adjust grind/strength estimations
0.4.14 - restart in response to home assistant reboot (TODO: more elegant response??); additional buttons on webUI?
0.4.13 - fix bug with last dispense error
0.4.12 - fix error 
0.4.11 - first publication flag; maybe remove init?? no, need to know at least a single value exists 
0.4.10 - rename sensors to high pressure/low pressure pump operations
0.4.9 - status reordering; pump offtime from timout 
0.4.8 - remove tray error tracker; wifi reliablity work
0.4.7 - remove websockets stuffs; simplify
0.4.6 - fill beans soon warning
0.4.5 - reduce size of web page hosted to custom automations only; fix wait_ready to account for duration 
0.4.4 - link communications of MQTT and websockets
0.4.3 - fix flow meter MQTT reporting; fix ceramic valve error
0.4.2 - custom executino status, blocking further executions
0.4.1 - correct version name format; correct custom automations for history log
0.4.0 - timeouts; new version; crash recovery
---
0.3.06 - identify "AS:"
0.3.05 - custom automation handling; interrupt on main core, not trapped in waitloop
0.3.04 - finally find a reutrnn >= 999 counter... version up entire system;
0.3.02 - differeing update intervals; find flow meter!
0.3.01 - completed arudino convrsion; add JSON output
0.3.00 - convert to arduino from ESPHome
---
0.2.41 - try supplemental commends from other sources; non seem to work...
0.2.40 - convert all calculations to cpp; reduce lambdas
0.2.39 - delay testing for connectivity?
0.2.28 - support for machine version (needs bootloader version too?)
0.2.27 - more custom automations and experimentation with live modification of automations
0.2.26 - pump state tied to output valve state; should always be flush/drain if not pumping (during cleaning to?)
0.2.25 - remove hidden options; leave for further testing
0.2.24 - refactoring
0.2.23 - re-add buttons
0.2.22 - add more sensors
0.2.21 - re-add sensors defined from strings; only pull comes from strings (only blocking?)
0.2.20 - minimize all calculations in update loop
0.2.19 - ESP32 changeover
0.2.18 - proper extraction and naming for cs:; rename to jura bridge
0.2.17 - duty cycle to mode translation with longer averaging (UART sampling probably too slow)
0.2.16 - pump duty cycle to three modes
0.2.15 - add menu items to yaml
0.2.14 - rename sensors in yaml to be prefixed by friendly_name
0.2.13 - break out cs1 into binary bitfield for testing; change releative temperature to 180F ??; annotate hz_7 bitfield based on current research
0.2.12 - modify mod-based custom polling intervals for different UART queries
0.2.11 - comment out unknown vars; testing later
0.2.10 - refactor and relabel all "sensors" to origin-based nomenclature (from base; no more xSensor etc)
0.1.00 - PoC with ESP8622
*/

#endif
