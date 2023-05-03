# jurabridge ☕


## Disclaimer

*The following is only provided as information documenting a project I worked on. No warranty or claim that this will work for you is made. Below is described some actions that involve modifying a Jura Ena Micro 90, which if performed will void any warranty you may have. Some of the modifications described below involve mains electricity; all appropriate cautions are expected to be and were followed. Some of the modifications are permanent and irreversible. Do not duplicate any of the following. I do not take anyresponsibility for any injuries or damage that may occur from any accident, lack of common sense or experience, or the like. Responsibility for any damages or distress resulting from reliance on any information made available here is not the responsibility of the author or other contributors. All rights are reserved.*


## Description

This is an arduino project for bridging a Jura Ena Micro 90 to home automation platforms via MQTT. Main controller is an ESP32. A 3v to 5v level shifter is required between an available UART of the ESP32 to the debug/service port of the Jura. 

Optionally, a second ESP32 or other controller can be used to simulate the dual throw momentary switches that power on the input board and the power control boards, respectively. 

## Hardware

* ESP32 Dev Board. I used [this one.](https://www.amazon.com/gp/product/B0718T232Z/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

* Level Shifter. I used [this one.](https://www.amazon.com/gp/product/B07LG646VS/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

* Dupont connectors. I used [these](https://www.amazon.com/gp/product/B01EV70C78/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1) and [these](https://www.amazon.com/gp/product/B07DF9BJKH/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1). 

* (Optional) 2 Channel Relay Board. I used [this one](https://www.amazon.com/gp/product/B00E0NTPP4/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1). Relays are required if you'd like to turn the machine on. 

## Machine Modifications

* Plumb reservoir. I [used this.](https://www.amazon.com/gp/product/B076HJZQMY/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

* Create aperture through input board coverpiece to feed cable to ESP32. 

* 3d printed bridge housing

* Splice into momentary swich leads, close splices with 2-channel relay.

![Splice Annotation](https://github.com/andrewjfreyer/jurabridge/raw/main/images/splices_annotated.jpg)

![Switch Splice Detail](https://github.com/andrewjfreyer/jurabridge/raw/main/images/lv_splice.jpg)

![Power Splice Detail](https://github.com/andrewjfreyer/jurabridge/raw/main/images/hv_splice.jpg)

## Connection Diagram 

*Diagram to come...*

## Software

* [Arduino IDE](https://www.arduino.cc/en/software/). Note that you may have to install serial drivers. 

* Board: ESP32 Dev Module

## Setup 

Open sketch in Arduino IDE, modify the `secrets.h` file, verify, and upload. Sketch defaults to UART 2, which on this board is Pin 16, 17. These pins couple to the Jura debug port. 

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


## References

https://www.youtube.com/watch?v=sNedGBSFs04 (disassembly)


https://github.com/MayaPosch/BMaC/blob/master/esp8266/app/juraterm_module.cpp 

https://forum.fhem.de/index.php?topic=45331.0

https://us.jura.com/-/media/global/pdf/manuals-na/Home/ENA-Micro-90/download_manual_ena_micro90_ul.pdf

https://github.com/PromyLOPh/juramote <-- HUGE HELP & INSPIRATION

https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/

https://blog.q42.nl/hacking-the-coffee-machine-5802172b17c1/

https://github.com/psct/sharespresso

https://github.com/Q42/coffeehack

https://hackaday.com/tag/jura/

https://community.home-assistant.io/t/control-your-jura-coffee-machine/26604/65

https://github.com/Jutta-Proto/protocol-cpp

https://github.com/COM8/esp32-jura

https://github.com/hn/jura-coffee-machine

http://protocoljura.wiki-site.com/index.php/Hauptseite

https://elektrotanya.com/showresult?what=jura-impressa&kategoria=coffee-machine&kat2=all

https://www.elektro-franck.de/search?sSearch=15269&p=1

https://www.juraprofi.de/anleitungen/Jura_ENA_Micro-9-90_A-5-7-9_Wasserlaufplan.pdf

https://www.thingiverse.com/thing:5348735

https://github.com/sklas/CofFi/blob/master/sketch/coffi_0.3.ino

https://github.com/Q42/coffeehack/blob/master/reverse-engineering/commands.txt

https://tore.tuhh.de/bitstream/11420/11433/2/Antrittsvortrag.pdf

https://protocol-jura.at.ua/index/commands_for_coffeemaker/0-5 


*Under active development, README still being written...*