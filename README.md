# jurabridge ☕

# Description

This is an ESP32 Arduino project for bridging a [Jura Ena Micro 90](https://us.jura.com/en/customer-care/products-support/ENA-Micro-90-MicroSilver-UL-15116) to home automation platforms via MQTT. Main controller is an ESP32. A 3v to 5v level shifter is required between an available hardware UART of the ESP32 to the debug/service port of the Jura. Don't use `softwareserial`, as it's painfully slow. 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/ena90.png" alt="Jura Ena Micro 90"/>
</p>

<div align="left">
      <a href="https://youtu.be/6NN9Xv9Lhq4">
         <img src="https://img.youtube.com/vi/6NN9Xv9Lhq4/0.jpg" style="width:100%;">
      </a>
</div>

<video src="https://youtu.be/6NN9Xv9Lhq4"></video>

The data output from the machine can be received and presented by [Home Assistant.](https://www.home-assistant.io) I have created this status UI in a heavily modified [button-card](https://github.com/custom-cards/button-card). 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_on.png" alt="BridgeOn"/>
</p>

Here's my Home Assistant [configuration (YAML package)](https://github.com/andrewjfreyer/jurabridge/wiki/Home-Assistant-Configuration). Here is what the bridge looks like, attached to the machine after a [minor modification](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Modifications) to feed a ribbon cable to the debug port. 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_housed.png" alt="Jura Ena Micro 90"/>
</p>

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_unhoused.png" alt="Jura Ena Micro 90"/>
</p>

<hr/>

# `jurabridge` MQTT Topics of Interest

### Machine & Bridge Status

| Topic | Description |
| --- | --- |
| jurabridge/power | Last will message of "off" to indicate bridge is off (not machine) |
| jurabridge/ready | Boolean for whether system is ready or not  |

### Command Structure

| Topic | Description |
| --- | --- |
| jurabridge/command | Post a custom automation or recipe formatted as an array of command arrays |
| jurabridge/menu | Post a message to manipulate buttons and menus |

### Counters
| Topic | Description |
| --- | --- |
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
| jurabridge/counts/water tank/volume | Estimated fill percentage in reservoir |

### Errors
| Topic | Description |
| --- | --- |
| jurabridge/errors/beans | Beans hopper lid removed  |
| jurabridge/errors/grounds | Grounds needs emptying |
| jurabridge/errors/powder | Powder door / bypass doser is open |
| jurabridge/errors/reservoir low | Water fill required soon |
| jurabridge/errors/tray overfill | Tray likely needs emptying  |
| jurabridge/errors/tray removed | Tray is removed |
| jurabridge/errors/water | Water fill required; replace reservoir |

### Command History
| Topic | Description |
| --- | --- |
| jurabridge/history | Last automation/task completed |

### Machine Part Information & Status
| Topic | Description |
| --- | --- |
| jurabridge/machine/brewgroup | Status position of brewgroup |
| jurabridge/machine/custom execution | Currently executing a custom program |
| jurabridge/machine/input board | Input board ready |
| jurabridge/machine/input board/state | Input board state |
| jurabridge/machine/last dispense | Volume of last dispense |
| jurabridge/machine/last grind duration | Approximate duration of last grind operation |

### Machine Part Information & Status
| Topic | Description |
| --- | --- |
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

### Current Recommendations
| Topic | Description |
| --- | --- |
| jurabridge/recommendation | Current highest priority recommendation |
| jurabridge/recommendations/milk clean | Whether m-clean is recommended |
| jurabridge/recommendations/milk rinse | Whether m-rinse is reecommended |
| jurabridge/recommendations/rinse | Whether water rinse is recommended |
| jurabridge/system | Narrative description of system status |

<hr/>

# Useful Wiki Links

* [Schematics](https://github.com/andrewjfreyer/jurabridge/wiki/Schematic(s))

* [Jura Ena Micro 90 Command/Response Investigations & Interpretations](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Commands)

* [Required & Optional Hardware](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware)

* [Arduino Setup & Upload](https://github.com/andrewjfreyer/jurabridge/wiki/Software)

* [References](https://github.com/andrewjfreyer/jurabridge/wiki/References)


# Custom Preparations & Actions

I've found out that the output of the machine is actually quite misleading about the machine's capabilities. The Jura produces good enough shots as is. I never thought to question the dosing volume, as the output was good enough for what I expected from a superautomatic. Since I bought the machine, my expectation was set that "this is the best the Jura can do, and that's just fine." However, somewhat surprising to me was that the Ena Micro 90 only uses ***7-10g of coffee per perparation*** - about half as much as I presumed. So, what if we just pull two shots back to back, at lower volume? Improved flavor, in my opinion, even if the shots do not look significantly different.

It's of course easy to pull two shots back to back, and to interrupt brewing (or to save the preparation settings) to lower volumes. Instead, we can use `jurabridge` to automate this sequence for us. Specifically, because `jurabridge` obtains and/or infers accurate machine status information, any number of custom recipes or custom instruction sequences can be excuted, without needing to modify EEPROM or to orchestrate a valid sequence of `FN:` commands, or without waiting for unnecessary long delays and presuming machine states. This command+interrupt technique ensures that the machine excutes its own in-built sequences, and there's no risk of incidentally damaging the brewgroup with custom instructions or custom brew sequences. 

Custom sequences/scripts can be made to allow the machine to produce a wide variety of other drinks, at higher quality. As a trivial example, different settings may be appropriate for non-dairy cappuccino than dairy cappuccino, yet the machine only has one setting. 

[Custom recipe examples.](https://github.com/andrewjfreyer/jurabridge/wiki/Custom-Recipe-Scripts)

<hr/>

## Disclaimer

*The following is only provided as information documenting a project I worked on. No warranty or claim that this will work for you is made. Below is described some actions that involve modifying a Jura Ena Micro 90, which if performed will void any warranty you may have. Some of the modifications described below involve mains electricity; all appropriate cautions are expected to be and were followed. Some of the modifications are permanent and irreversible. Do not duplicate any of the following. I do not take anyresponsibility for any injuries or damage that may occur from any accident, lack of common sense or experience, or the like. Responsibility for any damages or distress resulting from reliance on any information made available here is not the responsibility of the author or other contributors. All rights are reserved.*

*Note: Although loosely compatible, this code is far too large to work with ESPHome; attempts to get it to work usefully with all sensors updating at reasonable intervals caused watchdog crashes.*

