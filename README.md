# jurabridge ☕

# Description

This is an ESP32 Arduino project for bridging a [Jura Ena Micro 90](https://us.jura.com/en/customer-care/products-support/ENA-Micro-90-MicroSilver-UL-15116) to home automation platforms via MQTT. Main controller is an ESP32. A 3v to 5v level shifter is required between an available hardware UART of the ESP32 to the debug/service port of the Jura. Don't use softwareserial, as it's painfully slow. 

*Note: Although loosely compatible, this code is far too large to work with ESPHome; attempts to get it to work usefully with all sensors updating at reasonable intervals caused watchdog crashes.*

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/ena90.png" alt="Jura Ena Micro 90"/>
</p>

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_housed.png" alt="Jura Ena Micro 90"/>
</p>

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_unhoused.png" alt="Jura Ena Micro 90"/>
</p>

Optionally, a two channel relay board can be used to simulate the dual throw momentary switches that power on the input board and the power control boards, respectively. I control this with a second ESP32, but the same ESP could be used too. 

The data output from the machine can be received and presented by [Home Assistant.](https://www.home-assistant.io) I have created this status UI in a heavily modified [button-card](https://github.com/custom-cards/button-card). 

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


# Custom Preparations & Actions

I've found out that the output of the machine is actually quite misleading about the machine's capabilities. The Jura produces good enough shots as is. I never thought to question the dosing volume, as the output was good enough for what I expected from a superautomatic. Since I bought the machine, my expectation was set that "this is the best the Jura can do, and that's just fine." 

However, somewhat surprising to me was that the Ena Micro 90 only uses 7-10g of coffee per perparation - about half as much as I presumed. So, what if we just pull two shots back to back, at lower volume? Improved flavor, in my opinion, even if the shots do not look significantly different. See here: 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/compare.png" alt="Compare Pulls"/>
</p>

It's of course easy to pull two shots back to back, and to interrupt brewing (or to save the preparation settings at) to lower volumes. Instead, we can use `jurabridge` to automate this sequence for us. Specifically, once accurate machine status information is pulled from the machine, any number of custom recipes or custom instruction sequences can be excuted, without needing to modify EEPROM or to orchestrate a valid sequence of `FN:` commands. This ensures that the machine excutes its own in-built sequences, and there's no risk of incidentally damaging the machine with custom instructions or custom brew sequences. 

Custom sequences/scripts can be made to allow the machine to produce a wide variety of other drinks, at higher quality. As a trivial example, different settings may be appropriate for non-dairy cappuccino than dairy cappuccino, yet the machine only has one setting. 

[Custom recipe examples.](https://github.com/andrewjfreyer/jurabridge/wiki/Custom-Recipe-Scripts)

<hr/>

# Home Assistant Integration

Set up an MQTT broker, appropriately. Then subscribe to the topics of interest. Here are screenshots of my UI in a few Lovelace cards:

### Example UI: Bridge Off

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_off.png" alt="BridgeOff"/>
</p>


### Example UI: Bridge On

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_on.png" alt="BridgeOn"/>
</p>


Here's a dump from a Home Assistant package yaml defining all sensors and an nubmer of buttons:

```
#TURN ON IN THE MORNING ON FIRST MOTION FROM LIVING ROOM OR KITCHEN
automation:
  - alias: "appliance: ENA90 Morning Motion"
    initial_state: true
    mode: single
    trigger:
      - platform: state
        entity_id:
          - binary_sensor.living_room_motion
          - binary_sensor.kitchen_motion
        from: "off"
        to: "on"
    condition:
      - condition: state
        entity_id: binary_sensor.ena90_bridge_power
        state: "off"
      - condition: time
        after: "05:30:00"
        before: "10:00:00"
    action:
      - service: button.press
        target:
          entity_id: button.jura_power_controller_ena90_relay_power_button
      - delay: "05:00"

mqtt:
  button:
    ################################################################################
    #
    # menu items
    #
    ################################################################################
    - name: "ENA90 Menu Rinse"
      command_topic: "jurabridge/menu"
      payload_press: "rinse"
      icon: mdi:menu
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Menu Milk Clean"
      command_topic: "jurabridge/menu"
      payload_press: "mclean"
      icon: mdi:menu
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Menu Milk Rinse"
      command_topic: "jurabridge/menu"
      payload_press: "mrinse"
      icon: mdi:menu
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Menu Clean"
      command_topic: "jurabridge/menu"
      payload_press: "clean"
      icon: mdi:menu
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Menu Filter"
      command_topic: "jurabridge/menu"
      payload_press: "filter"
      icon: mdi:menu
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    ################################################################################
    #
    # recipes and automations
    #
    ################################################################################

    - name: "ENA90 Make Short Cappuccino"
      command_topic: "jurabridge/command"
      payload_press: "short cappuccino"
      icon: mdi:coffee-maker-outline
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Make Americano"
      command_topic: "jurabridge/command"
      payload_press: "americano"
      icon: mdi:coffee-maker-outline
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Make Double Ristretto"
      command_topic: "jurabridge/command"
      payload_press: >
        [
            ["id", "DOUBLE RISTRETTO"],
            ["msg", " MORNING!"],
            ["delay", 1000],
            ["msg", " STEP 1/2"],
            ["delay", 2000],
            ["ready"],
            ["espresso"],
            ["pump"],
            ["dispense", 30],
            ["interrupt"],
            ["msg", " STEP 2/2"],
            ["delay", 2000],
            ["ready"],
            ["espresso"],
            ["pump"],
            ["dispense", 30],
            ["interrupt"],
            ["msg", "    :)"],
            ["delay", 5000]
          ]
      icon: mdi:coffee-maker-outline
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Make Espresso"
      command_topic: "jurabridge/command"
      payload_press: >
        [
          ["ready"],
          ["espresso"],
          ["pump"],
          ["dispense",40],
          ["interrupt"],
          ["msg","    :)"]
        ]
      icon: mdi:coffee-maker-outline
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Make Cappuccino"
      command_topic: "jurabridge/command"
      payload_press: >
        [
          ["ready"],
          ["cappuccino"],
          ["msg","    :)"]
        ]
      icon: mdi:coffee-maker-outline
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Make Coffee"
      command_topic: "jurabridge/command"
      payload_press: >
        [
          ["ready"],
          ["coffee"],
          ["msg","    :)"]
        ]
      icon: mdi:coffee-maker-outline
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Make Macchiato"
      command_topic: "jurabridge/command"
      payload_press: >
        [
          ["ready"],
          ["macchiato"],
          ["msg","    :)"]
        ]
      icon: mdi:coffee-maker-outline
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Make Water"
      command_topic: "jurabridge/command"
      payload_press: >
        [
          ["ready"],
          ["water"],
          ["msg","    :)"]
        ]
      icon: mdi:coffee-maker-outline
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Make Milk Foam"
      command_topic: "jurabridge/command"
      payload_press: >
        [
          ["ready"],
          ["milk"],
          ["msg","    :)"]
        ]
      icon: mdi:coffee-maker-outline
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    ################################################################################
    #
    # button presses
    #
    ################################################################################

    - name: "ENA90 Power Off"
      command_topic: "jurabridge/command"
      payload_press: "off"
      icon: mdi:power
      entity_category: "config"
      availability:
        topic: "jurabridge/ready"
        payload_available: "TRUE"
        payload_not_available: "FALSE"

    - name: "ENA90 Settings"
      command_topic: "jurabridge/command"
      payload_press: "settings"
      icon: mdi:cog-outline
      entity_category: "config"

    - name: "ENA90 Confirm"
      command_topic: "jurabridge/command"
      payload_press: "confirm"
      icon: mdi:cog-outline
      entity_category: "config"

  ########################### JURA BRIDGE MQTT BINARIES ##############################

  binary_sensor:
    - name: "ENA90 Bridge Power"
      state_topic: "jurabridge/power"
      payload_on: "TRUE"
      payload_off: "FALSE"
      icon: mdi:power

    - name: "ENA90 Machine Ready"
      state_topic: "jurabridge/ready"
      payload_on: "TRUE"
      payload_off: "FALSE"
      icon: mdi:alert

    - name: "ENA90 Input Board State"
      state_topic: "jurabridge/machine/input board"
      payload_on: "TRUE"
      payload_off: "FALSE"
      icon: mdi:alert

    - name: "ENA90 Brewgroup Ready"
      state_topic: "jurabridge/machine/brewgroup"
      payload_on: "TRUE"
      payload_off: "FALSE"
      icon: mdi:check

    ########################### ERRORS ##############################
    - name: "ENA90 Drip Tray Removed"
      state_topic: "jurabridge/errors/tray removed"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    - name: "ENA90 Drip Tray Overfill"
      state_topic: "jurabridge/errors/tray overfill"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    - name: "ENA90 Water Reservior Volume"
      state_topic: "jurabridge/errors/reservoir low"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    - name: "ENA90 Grounds"
      state_topic: "jurabridge/errors/grounds"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    - name: "ENA90 Beans Hopper Cover"
      state_topic: "jurabridge/errors/beans"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    - name: "ENA90 Water Reservior"
      state_topic: "jurabridge/errors/water"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    - name: "ENA90 Bypass Doser"
      state_topic: "jurabridge/errors/powder"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    ########################### BINARY RECOMMENDATIONS ##############################

    - name: "ENA90 Recommendation"
      state_topic: "jurabridge/recommendations"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    - name: "ENA90 Rinse Recommended"
      state_topic: "jurabridge/recommendations/rinse"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    - name: "ENA90 Milk Rinse Recommended"
      state_topic: "jurabridge/recommendations/milk rinse"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

    - name: "ENA90 Milk Clean Recommended"
      state_topic: "jurabridge/recommendations/milk clean"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

      ########################### OTHER ##############################

    - name: "ENA90 Custom Automation"
      state_topic: "jurabridge/machine/custom execution"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: problem
      icon: mdi:alert

      ########################### PARTS ##############################

    - name: "ENA90 Thermoblock Preheated"
      state_topic: "jurabridge/parts/thermoblock/preheated"
      payload_on: "TRUE"
      payload_off: "FALSE"
      icon: mdi:thermometer

    - name: "ENA90 Thermoblock Heating"
      state_topic: "jurabridge/parts/thermoblock/active"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: heat
      icon: mdi:fire

    - name: "ENA90 Pump Active"
      state_topic: "jurabridge/parts/pump/active"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: running
      icon: mdi:water

    - name: "ENA90 Pump Circulating"
      state_topic: "jurabridge/parts/pump/flowing"
      payload_on: "TRUE"
      payload_off: "FALSE"
      device_class: running
      icon: mdi:water

    - name: "ENA90 Brewgroup Active"
      state_topic: "jurabridge/parts/grinder/active"
      device_class: running
      payload_on: "TRUE"
      payload_off: "FALSE"
      icon: mdi:cog-outline

  ########################### PREPARATIONS ##############################

  sensor:
    - name: "ENA90 Machine Ready State"
      state_topic: "jurabridge/system"
      icon: mdi:state-machine

    - name: "ENA90 Machine Recommendation State"
      state_topic: "jurabridge/recommendation"
      icon: mdi:information

    - name: "ENA90 History"
      state_topic: "jurabridge/history"
      icon: mdi:history

    - name: "ENA90 Total Automations"
      state_topic: "jurabridge/counts/total automations"
      unit_of_measurement: "preparations"
      icon: mdi:counter

    - name: "ENA90 Input Board State"
      state_topic: "jurabridge/machine/input board/state"
      icon: mdi:history

    - name: "ENA90 Water Tank Volume"
      state_topic: "jurabridge/counts/water tank/volume"
      unit_of_measurement: "%"
      icon: mdi:water

    - name: "ENA90 Last Grind Duration"
      state_topic: "jurabridge/machine/last grind duration"
      unit_of_measurement: "ms"
      icon: mdi:timer

    - name: "ENA90 Pump Status"
      state_topic: "jurabridge/parts/pump/status"
      unit_of_measurement: "state"
      icon: mdi:pump

    - name: "ENA90 Tray Volume"
      state_topic: "jurabridge/counts/tray volume"
      unit_of_measurement: "ml"
      icon: mdi:water

    - name: "ENA90 Estimated Hopper Volume"
      state_topic: "jurabridge/counts/beans"
      unit_of_measurement: "%"
      icon: mdi:counter

    - name: "ENA90 High Pressure Operations"
      state_topic: "jurabridge/counts/high pressure operations"
      unit_of_measurement: "operations"
      icon: mdi:counter

    - name: "ENA90 Espresso Preparations"
      state_topic: "jurabridge/counts/espresso"
      unit_of_measurement: "preparations"
      icon: mdi:coffee

    - name: "ENA90 Coffee Preparations"
      state_topic: "jurabridge/counts/coffee"
      unit_of_measurement: "preparations"
      icon: mdi:coffee

    - name: "ENA90 Cappuccino Preparations"
      state_topic: "jurabridge/counts/cappuccino"
      unit_of_measurement: "preparations"
      icon: mdi:coffee

    - name: "ENA90 Macchiato Preparations"
      state_topic: "jurabridge/counts/macchiato"
      unit_of_measurement: "preparations"
      icon: mdi:coffee

    - name: "ENA90 Milk Foam"
      state_topic: "jurabridge/counts/milk foam"
      unit_of_measurement: "preparations"
      icon: mdi:beer

    - name: "ENA90 Hot Water"
      state_topic: "jurabridge/counts/hot water"
      unit_of_measurement: "preparations"
      icon: mdi:cup-water

      ########################### PARTS ##############################

    - name: "ENA90 Thermoblock Temperature"
      state_topic: "jurabridge/parts/thermoblock/temp"
      unit_of_measurement: "C"
      icon: mdi:thermometer

    - name: "ENA90 Thermoblock Duty Cycle"
      state_topic: "jurabridge/parts/thermoblock/duty"
      unit_of_measurement: "%"
      icon: mdi:percent-box

    - name: "ENA90 Pump Duty Cycle"
      state_topic: "jurabridge/parts/pump/duty"
      unit_of_measurement: "%"
      icon: mdi:percent-box

    - name: "ENA90 Ceramic Valve Temperature"
      state_topic: "jurabridge/parts/ceramic valve/temp"
      unit_of_measurement: "C"
      icon: mdi:thermometer

    - name: "ENA90 Output Valve Position"
      state_topic: "jurabridge/parts/output valve/position"
      icon: mdi:valve

    - name: "ENA90 Ceramic Valve Position"
      state_topic: "jurabridge/parts/ceramic valve/position"
      icon: mdi:valve

    - name: "ENA90 Last Dispense"
      state_topic: "jurabridge/machine/last dispense"
      unit_of_measurement: "mL"
      icon: mdi:water

      ########################### ADMIN ##############################

    - name: "ENA90 Spent Grounds"
      state_topic: "jurabridge/counts/grounds"
      unit_of_measurement: "pucks"
      icon: mdi:hockey-puck

    - name: "ENA90 Since Cleaned"
      state_topic: "jurabridge/counts/since clean"
      unit_of_measurement: "preparations"
      icon: mdi:counter

      ########################### SANITARY ##############################

    - name: "ENA90 Low Pressure Operations"
      state_topic: "jurabridge/counts/low pressure operations"
      unit_of_measurement: "operations"
      icon: mdi:water

    - name: "ENA90 Descale Cycles"
      state_topic: "jurabridge/counts/descales"
      unit_of_measurement: "descale"
      icon: mdi:water

    - name: "ENA90 Milk Clean Cycles"
      state_topic: "jurabridge/counts/milk clean"
      unit_of_measurement: "cleans"
      icon: mdi:spray-bottle

    - name: "ENA90 System Clean Cycles"
      state_topic: "jurabridge/counts/cleans"
      unit_of_measurement: "cleans"
      icon: mdi:spray-bottle

    - name: "ENA90 Grinder"
      state_topic: "jurabridge/counts/grinder"
      unit_of_measurement: "grinds"
      icon: mdi:counter

```

# References & Thanks

Lots of folks have provided a great amount of information that allowed this project to be possible. Some links and general research notes follow: 

https://www.youtube.com/watch?v=sNedGBSFs04 (disassembly)

https://github.com/MayaPosch/BMaC/blob/master/esp8266/app/juraterm_module.cpp 

https://forum.fhem.de/index.php?topic=45331.0

https://us.jura.com/-/media/global/pdf/manuals-na/Home/ENA-Micro-90/download_manual_ena_micro90_ul.pdf

https://github.com/PromyLOPh/juramote <--  great foundational reference for Jura protocol reversing

https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/  <-- great inspiration for completing the project

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


## Disclaimer

*The following is only provided as information documenting a project I worked on. No warranty or claim that this will work for you is made. Below is described some actions that involve modifying a Jura Ena Micro 90, which if performed will void any warranty you may have. Some of the modifications described below involve mains electricity; all appropriate cautions are expected to be and were followed. Some of the modifications are permanent and irreversible. Do not duplicate any of the following. I do not take anyresponsibility for any injuries or damage that may occur from any accident, lack of common sense or experience, or the like. Responsibility for any damages or distress resulting from reliance on any information made available here is not the responsibility of the author or other contributors. All rights are reserved.*
