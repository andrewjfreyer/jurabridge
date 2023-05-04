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

# Understanding the Jura Ena Micro 90

Many folks have reverse engineered other Jura models, but to the best of my research, no one had documented investigations of the Ena Micro 90. 

## Similar Machine Schematic

The Ena Micro 9 schematics are available online, at [jura-parts.com](https://www.jura-parts.com/v/vspfiles/diagrams/Jura%20ENA%209%20Micro%20Diagram.pdf). These schematics are useful to understand the layout of the Ena Micro 90 and other devices that share a number of parts and overall design language with their predecesors:

### Simplified Schematic Showing Input Board + Power Board

There is a copyright notice in each of these images, reserved by Jura. 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/schematic_micro9.png" alt="Jura Ena Micro 9"/>
</p>

### Simplified Water System Schematic
<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/fluidsystem_micro9.png" alt="Jura Ena Micro 9"/>
</p>

<hr/>

# Jura Command/Response Decoding

Jura has implemented a transfer encoding that spreads data bytes through a number of other bytes. Explained [here](https://github.com/PromyLOPh/juramote) and [here](https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/) and [here](https://github.com/oliverk71/Coffeemaker-Payment-System) and [here](http://protocol-jura.do.am).

Unfortunately, each machine has a slighly different set of commands that it responds to and encodes its sensors differently. The following are inferred from testing, review of hydraulic system topology and review of schematics of other machines. I believe that these are correct, but it's likely there are some nuances I don't appreicate or haven't figured out yet. 

*Note: spaces below are inserted for legibility, but do not occur in the response from the machine. The letters h,d,b refer to hexadecimal values, decimal values, and binary values respectively.*

### Useful Commands

| Command | Response| Presumed Acronym | Description |
| --- | - | - | --- |
| "IC:" | IC:hhhh | Input Controller (??) | Four (evidently) hexadecimal valus returned, corresponding to status of the input board|
| "RT:####" | RT:0xhhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh | Reihentest (??) | Sixteen (evidently) hexadecimal values from EEPROM address specified at input|
| "RR:hh" | RR:hhhh | Read RAM (??) | Possible ram values? Only some values change over time. **[Investigating; not used]** |
| "HZ:" | HZ:bbbbbbbbbbb,hhhh,hhhh,hhhh,hhhh,hhhh,d,bbbbb | Heißwasser-Zubereitung (??) | Values related to hot water and steam preparation automations|
| "CS:" | CS:hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh  | Circuits und Systeme (??) | Sixteen (evidently) hexadecimal valus returned, corresponding digital system values and variables|
| "FA:hh" | OK| Funktion auswählen (??) | Select a programmed operaiton of the machine to initiate (e.g., predetermined sequences of FN commands, like button presses or "brewgorup move to position") |
| "FN:hh" | OK| Funktion (??) | Initate a mechanical function of the machine (be careful here) |  
| "AS:" | AS:hhhh hhhh,hhhh hhhh| Analoger Sensor (??) | Analog values of sensors (e.g., resistance of thermistors) **[Investigating; not used]** |  
| "DI:" | OK  | Drainage Init (??) |  Initialize the drainage/output valve (linear, servo-controlled, not ceramic valve) |
| "GB:" | OK  | ?? | Disable power board, shut down machine without rinsing |
| "MI:" | OK  | ?? | Drainage valve move to 1 position |
| "MC:" | OK  | ?? | Drainage valve move to 2 position |
| "MV:" | OK  | ?? | Drainage valve validate position; (e.g., cycle through throw of server to reposition) |

### Command Response Interpretation **IC:**

Received via UART as a string, so references are from left to right as indexes of the corresponding `char` array:

| Index | Intepretation |
| --- | --- |
| 0 | four bits correspond to error flags for reed switches connected to the input board |
| 1 | flow meter |
| 2 | always zero?? |
| 3 | Some machine state representation; when 2, machine is can be ready; when 3 machine is moving water |


- **IC [0] Interpretation**

  - Binary representation of hex value == four binary flags. From left to right (again, referenced as char array here; zero index on left; *h* = *bbbb* = 0123

     - [0] = bean hopper lid state; 0 = open; 1 = OK (momentary switch under vent on top of machine is closed)

     - [1] = water tank error; 0 = OK, 1 = problem (reed switch is in presence of foating magnet in reservior)

     - [2] = bypass doser; 0 = OK; 1 = bypass doser door open (reed switch is in presence of magnet in door)

     - [3] = drip tray removed; 1 = seated properly; 0 = open (momentary switch behind tray front is closed)


- **IC [1] Interpretation**

  - Apparently a flow meter. Value cycles through odd values when water is moving through the system. Hexidecimal 0xF appears to be the state that represents "confirmed, no flow." The value can stop changing on other values though, and does not appear to consistenty reset to 0xF after an operation has stopped. Sometimes, it will reset to 0xF immediately after flow stops, other times it will take 180 seconds to reset. This value is captured in the Arduino sketch and wrapped in a 5 second timeout. 

- **IC [2] Interpretation**

  - Unknown, appears to be always zero. 

- **IC [3] Interpretation**

  - Some input board status flag; still investigating. When value == 2, system is ready. 


<hr/>

# Project Hardware

* ESP32 Dev Board. I used [this one.](https://www.amazon.com/gp/product/B0718T232Z/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

* Level Shifter. I used [this one.](https://www.amazon.com/gp/product/B07LG646VS/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

* Dupont connectors. I used [these](https://www.amazon.com/gp/product/B01EV70C78/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1) and [these](https://www.amazon.com/gp/product/B07DF9BJKH/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1). 

* (Optional) 2 Channel Relay Board. I used [this one](https://www.amazon.com/gp/product/B00E0NTPP4/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1). Relays are required if you'd like to turn the machine on. 

# Machine Modifications

* Plumb reservoir. I [used this.](https://www.amazon.com/gp/product/B076HJZQMY/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1)

* Create aperture through input board coverpiece to feed cable to ESP32. 

* 3d printed bridge housing

* Splice into momentary swich leads, close splices with 2-channel relay.

![Splice Annotation](https://github.com/andrewjfreyer/jurabridge/raw/main/images/splices_annotated.jpg)

![Switch Splice Detail](https://github.com/andrewjfreyer/jurabridge/raw/main/images/lv_splice.jpg)

![Power Splice Detail](https://github.com/andrewjfreyer/jurabridge/raw/main/images/hv_splice.jpg)

# Connection Diagram 

The ESP32 can be powered directly from the 5V rail of the Jura Ena Micro 90, but of course this means that when the machine is off so is the bridge. Separate power is also possible, but know that in this case, the ESP32 will continue to power the input board; some sensors will be readable even after the machine is "off" such as temperature sensors. Running the machine in this manner does not appear to cause damage, but whether or not this operational mode causes damage is unknown. Recommended to power the `jurabridge` from the machine itself. 

Connect hardware UART pins to LV pins of the converter board. Corresponding HV pins should be connected to appropriate Tx and Rx pins of the jura debug port:

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/wiring.png" alt="Wiring Diagram"/>
</p>

@AussieMakerGeek has an annotated pinout [here](https://www.instructables.com/IoT-Enabled-Coffee-Machine/), reproduced (cropped & rotated) below:
<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/debug_port.png" alt="Debug Port"/>
</p>

<hr/>

# Project Software

* [Arduino IDE](https://www.arduino.cc/en/software/). Note that you may have to install serial drivers. 

* Board: ESP32 Dev Module

Open sketch in Arduino IDE, create the `secrets.h` file with the defines set below, verify, and upload. Sketch defaults to UART 2, which on this board is Pin 16, 17. These pins couple to the Jura debug port. 

`secrets.h`

```
#define WIFINAME    "ssid"
#define WIFIPASS    "ssid password"
#define MQTTBROKER  "mqtt.broker.ip.address"
#define MQTT_PASS   "pw"
#define MQTT_USER   "user"
```

<hr/>


# Custom Preparations & Actions

Once accurate machine status information is pulled from the machine, any number of custom recipes or custom instruction sequences can be excuted, without needing to modify EEPROM or to orchestrate a valid sequence of `FN:` commands. This ensures that the machine excutes its own in-built sequences, and there's no risk of incidentally damaging the machine with custom instructions or custom brew sequences. 

Any number of different sequence customizations can be made to allow the machine to produce a wide variety of other drinks. As a trivial example, different settings may be appropriate for non-dairy cappuccino than dairy cappuccino, yet the machine only has one setting. 

### Double Ristretto Custom Recipe

Here is a brew sequence I use for stronger, more traditionally extracted espresso. It  pulls two ristretto shots back to back, each at roughly 30ml. This results in a much more flavorful and properly extracted whole shot. A technique like this is viable and does not waste coffee, as each Jura grind operation only uses 7g of coffee, compared against the ~15g of a traditional espresso pull.

```
Topic:    jurabridge/command
Payload:    
          [
            ["msg", " MORNING!"],
            ["delay", 1000],
            ["msg", "  STEP 1"],
            ["delay", 2000],
            ["ready"],
            ["espresso"],
            ["pump"],
            ["dispense", 30],
            ["interrupt"],
            ["msg", "  STEP 2"],
            ["delay", 2000],
            ["ready"],
            ["espresso"],
            ["pump"],
            ["dispense", 30],
            ["interrupt"],
            ["msg", "    :)"],
            ["delay", 5000]
          ]

```

### Double Ristretto with Cup Pre-warming 

```
Topic:    jurabridge/command
Payload:    
          [
            ["msg", " MORNING!"],
            ["ready"],
            ["delay", 1000],
            ["msg", " PRE-WARM "],
            ["delay", 1000],
            ["water"],
            ["delay", 4000],
            ["pump"],
            ["dispense", 100],
            ["msg", "   WAIT"],
            ["interrupt"],
            ["ready"],
            ["delay", 4000],
            ["msg", " EMPTY CUP"],
            ["delay", 3000],
            ["msg", " EMPTY 5"],
            ["delay", 1000],
            ["msg", " EMPTY 4"],
            ["delay", 1000],
            ["msg", " EMPTY 3"],
            ["delay", 1000],
            ["msg", " EMPTY 2"],
            ["delay", 1000],
            ["msg", " EMPTY 1"],
            ["delay", 3000],
            ["msg", " STEP 1/2"],
            ["delay", 3000],
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

```

### Cup Pre-Warming 

```
Topic:    jurabridge/command
Payload:    
          [
            ["msg", " MORNING!"],
            ["ready"],
            ["delay", 1000],
            ["msg", " PRE-WARM "],
            ["delay", 1000],
            ["water"],
            ["delay", 4000],
            ["pump"],
            ["dispense", 100],
            ["msg", "   WAIT"],
            ["interrupt"],
            ["ready"],
            ["delay", 4000],
            ["msg", " EMPTY CUP"],
            ["delay", 5000]
          ]
```


### Short Cappuccino

```
Topic:    jurabridge/command
Payload:    
          [
            ["delay", 1000],
            ["msg", " STEP 1/3"],
            ["delay", 2000],
            ["ready"],
            ["milk"],
            ["pump"],
            ["dispense", 60],
            ["interrupt"],
            ["ready"],
            ["msg", " STEP 2/3"],
            ["delay", 2000],
            ["ready"],
            ["espresso"],
            ["pump"],
            ["dispense", 30],
            ["interrupt"],
            ["msg"," STEP 3/3"],
            ["delay", 2000],
            ["ready"],
            ["espresso"],
            ["pump"],
            ["dispense", 30],
            ["interrupt"],
            ["msg", "    :)"],
            ["delay", 5000]
          ]

```


### Short Americano 

```
Topic:    jurabridge/command
Payload:    
          [
            ["delay", 1000],
            ["msg", " STEP 1/3"],
            ["delay", 2000],
            ["ready"],
            ["water"],
            ["pump"],
            ["dispense", 60],
            ["interrupt"],
            ["ready"],
            ["msg", " STEP 2/3"],
            ["delay", 2000],
            ["ready"],
            ["espresso"],
            ["pump"],
            ["dispense", 30],
            ["interrupt"],
            ["msg"," STEP 3/3"],
            ["delay", 2000],
            ["ready"],
            ["espresso"],
            ["pump"],
            ["dispense", 30],
            ["interrupt"],
            ["msg", "    :)"],
            ["delay", 5000]
          ]

```

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
