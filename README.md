# jurabridge ☕

# TL;DR

An [ESP32 dev board](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware) coupled to the service port of a [Jura ENA Micro 90](https://us.jura.com/en/customer-care/products-support/ENA-Micro-90-MicroSilver-UL-15116) can (1) poll machine data (e.g., sensors, memory, settings) via UART and (2) issue commands to the machine so as to act as a bridge between the machine and Home Assistant. See here for [hardware](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware) and [software](https://github.com/andrewjfreyer/jurabridge/wiki/software) requirements. [Connections are straightforward.](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware#connection-diagram)

[Home Assistant Forum Link](https://community.home-assistant.io/t/jura-ena-micro-90-to-mqtt-bridge)

# Summary

ESP32 Arduino project for bridging a [Jura ENA Micro 90](https://us.jura.com/en/customer-care/products-support/ENA-Micro-90-MicroSilver-UL-15116) to home automation platforms via MQTT. A 3.3v to 5v level shifter is required between hardware UART of the ESP32 to the service port of the Jura. The ESP32 polls the Jura for status information via reverse-engineered Jura Service Port calls, calculates/determines device state and meta statuses and reports changes to an MQTT broker. Added custom preparation functionality via a separate input button.

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/ena90.png" alt="Jura ENA Micro 90"/>
</p>

# Why? It's a discontinued machine...

For fun, that's all. The machine is *awesome* and still available as a refurb. 

I saw [this](https://github.com/ryanalden/esphome-jura-component/) and other projects for other Jura models and thought I'd begin investigating the ENA Micro 90. Some existing comands worked, most did not. I needed to investigate and characterize a lot of the output from the machine and iterate through hundreds of commands to determine which ones the machine responded to, note what the response was, and then set about decoding/understanding what the data meant. One thing led to another, and now we're here. 

# So what? 

With 'jurabridge' we can/have: 

* ✅ **Power control.** Automatically turn the machine on and off in response to whatever triggers I want (via Home Assistant). Personally, mine is triggered from a motion sensor nearby the kitchen in the mornings. 

* ✅ **Custom coffee drinks.** Automatically create custom coffee or milk drinks with an arbitrary number of shots, arbitrary milk quantity, arbitrary water quantity (e.g., cortado, americano, lungo, ristretto, short cap, and so on). 

* ✅ **Tuning.** Fine-tune shots to improve quality. By default, the machine doses 8 - 9g of coffee (it seems) and pushes through it 30ml of water, which ends up being a 3:1 grounds to water dosing ratio. That's about half-strenth compared to a traditional pull. Also, after extraction starts, channeling is almost guaranteed to take over and extraction will suffer even before we consider how much water is being forced through. To improve shot quality then, we need to reduce water and/or increase coffee. We can't increase coffee because of torque limitations of the brew group drive motor during tamping. So our best option to improve our shot is to pull 15 - 17ml through a standard 8g puck twice, back-to-back. This would give about 30ml of espresso pulled through 16g of coffee, in aggregate. The shots are *much* better to my palate using this technique.

* ✅ **Quality control.** The thermoblock does not have fine temperture control, so there is quite a large range of possible extraction temperatures for different shots. The system can warn if the thermoblock is too high (meaning overextraction is likely), or if the thermoblock is too low (meaning underextraction is likely). In either case, running water through the machine can serve three purposes: (1) pre-warm a mug, (2) normalize thermoblock temperature below or around 100°C by purging heat or forcing the thermoblock on, and (3) clean the mug. This is not necessary in all cases, but is a good idea for best shot performance. 

* ✅ **Automation.** Automate our fine-tunings and/or temperature purge so that a perfect pull (sequence) executes without supervision.

* ✅ **Convenience & sanitation.** Automate maintenance operations such as brew group rinse and milk system rinse when the machine is idle. This will save internal silicon tubing from absorbing and accumulating burned coffee flavors, and it'll improve the sanitation of the milksystem.

* ✅ **Reminders.** Create maintence reminders or alerts via Home Assistant (e.g., iOS notifications, Alexa notifications, UI alerts or badges)

* ✅ **Alerts.** iOS alerts when a coffee product is ready.
  
* ✅ **Improved safety.** Ensure the machine turns off at an appropriate time, or in response to certain conditions. *⚠️ NOTE: If a milk clean operation is not completed and the machine is waiting for "water for milk clean," the thermoblock is held around 150 degrees celcius for sanitation purposes. There *DOES NOT* appear to be a timeout apart from the machine's power off timer, which may be on the order of hours. If the machine is left in this state, it'll consume signifiant power and pose a minor fire risk.*
  
* ✅ **Ridiculousness.** Due to the connection to Home Assisant, we have connection to voice assistants too: "Alexa, make me a short espresso." 

* ✅ **Overwhelming data.** We retreive all meters from the machine, including service life of the grinder, the drive motor of the brew group, and others. Can inform when other maintenance is necessary. 

# Table of Contents

### Reverse Engineering 

* [Jura ENA Micro 90 Command/Response Investigations & Interpretations](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Commands-v2)

* [Relevant Schematics](https://github.com/andrewjfreyer/jurabridge/wiki/Schematic(s))

### Building & Programming the `jurabridge`

* [Required & Optional Hardware](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware)

* [Arduino Setup & Upload](https://github.com/andrewjfreyer/jurabridge/wiki/Software)

### Using `jurabridge` for Stuff

* [MQTT Topics & Payloads](https://github.com/andrewjfreyer/jurabridge/wiki/MQTT-Topics-v2)

* [Jura ENA Micro 90 Mods](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Modifications)

### Acknowledgements & Further Reading

* [References](https://github.com/andrewjfreyer/jurabridge/wiki/References)


# Home Assistant

The bridge automatically reports back to HA via MQTT Discovery, divided into a number of different devices for readability. Once `secrets.h` is configured with appropriate values, and device discovery finishes, size different devices will appear:

<p align="center">
<img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/mqtt_device_discovery_main.png" alt="Jura Ena Micro 90"/>
</p>


## Main Controller
<p align="center">
<img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/mqtt_device_discovery_controller.png" alt="Jura Ena Micro 90"/>
</p>

## Brew Group
<p align="center">
<img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/mqtt_device_discovery_brewgroup.png" alt="Jura Ena Micro 90"/>
</p>

## Dosing System
<p align="center">
<img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/mqtt_device_discovery_dosing.png" alt="Jura Ena Micro 90"/>
</p>

## Milk System
<p align="center">
<img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/mqtt_device_discovery_milk.png" alt="Jura Ena Micro 90"/>
</p>

## Water System
<p align="center">
<img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/mqtt_device_discovery_water.png" alt="Jura Ena Micro 90"/>
</p>

## Thermoblock
<p align="center">
<img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/mqtt_device_discovery_thermoblock.png" alt="Jura Ena Micro 90"/>
</p>

## User Interface Example

With all the automatically-exposed sensors, a UI can be built for Home Assistant that provides tons of information about the machine and its preparations. For example, with [button-card](https://github.com/custom-cards/button-card) and others: 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/ui_walkthrough.gif" alt="Jura Ena Micro 90"/>
</p>


# `jurabridge` Hardware

Here is what the `jurabridge` looks like, attached to the machine after a few destructive [modifications](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Modifications) to feed a ribbon cable through the housing to the debug port. 


<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_housed.png" alt="Jura Ena Micro 90"/>
</p>

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_unhoused.png" alt="Jura Ena Micro 90"/>
</p>

In v2, I added a momentary pushbutton to Pin 21 that allows for custom menus that leverage the machine's own display:

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_with_button.png" alt="Jura Ena Micro 90"/>
</p>

The data output from the machine is received and presented by [Home Assistant presented via MQTT Device Discovery.](https://www.home-assistant.io) 

<hr/>

# Maintenance Automation

With machine status accurately known by the `jurabridge`, we can schedule maintenance operations automatically. This will increase output quality and sanitation to an extent. All this means is that a few minutes after a milk preparation has been completed and the machine has been idle, milk rinse is automatically performed. Similarly, after sitting idle for a time period, a water rinse is automaticaly performed. 

<hr/>


# Custom Preparations & Actions

I've found out that the default programming dramatically undersells the machine's capabilities. The Jura produces good enough shots as is. All that to say, since I bought the machine, my expectation was set that "this is the best it can do, and that's just fine." It is, after all, a superauto and sacrifies are made over a manual process. We're sacrificing some quality for pushbutton convenience. However, surprising to me was that the ENA Micro 90 only uses ***7-10g of coffee per perparation*** - with 30 some-odd ml of water. That's less coffee and more water than I presumed, without giving it much thought. Plus, pump pressure drops pretty dramatically due to channeling so, the first parts of our shots are the best parts anyway. 

It's of course easy to pull two short shots back to back to get to a more traditional 15 - 20g "single" shot, but why not automate it? 
Lets just automate it. Here, because `jurabridge` obtains (and/or infers) accurate machine status information, any number of custom recipes or custom instruction sequences can be excuted, without needing to modify EEPROM or to orchestrate a valid sequence of `FN:` commands, or without waiting for unnecessary long delays to presuming machine state. This command+interrupt+statuswait technique ensures that the machine excutes its own in-built sequences, and there's no risk of incidentally damaging the brewgroup with custom instructions or custom brew sequences. 

In this project there's a header above that defines custom functions that are accessible via a pushbutton. As an example, there are three custom operations defined. 

Each of the menu array entires conform to a struct: 

```

struct JuraCustomMenuItemConfiguration {
  const char name[255];
  const char topic[255];
  const char payload[255];
};

```

The name field of this struct should be ten ASCII characters or less, as this will be displayed on the machine's display. The array itself can be modified to include as many custom functions as needed:

```
static const JuraCustomMenuItemConfiguration JuraCustomMenuItemConfigurations[] {
  {
    " RISTR x2",
    MQTT_ROOT MQTT_SUBTOPIC_FUNCTION "make_espresso",
    "{'add':1,'brew':17}",
  },
  {
    "  CAP +1",
    MQTT_ROOT MQTT_SUBTOPIC_FUNCTION "make_cappuccino",
    "{'milk':60,'brew':17,'add':1}",
  },
  {
    "CORTADO +1",
    MQTT_ROOT MQTT_SUBTOPIC_FUNCTION "make_cappuccino",
    "{'milk':30,'brew':17',add':1}",
  },
  { /* the last element will always be treated as an exit, regardless the command phrase here */
    "   EXIT",
    "",
    "",
  }
};
```

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/menu_demo.gif" alt="Jura Ena Micro 90"/>
</p>

1. The first index prepares a double ristretto in which two espresso shots are pulled back to back, each limited to 17ml of espresso output (that's about 2:1 ratio in respect of the grounds volume of 8g). 

2. The second index here prepares a cappuccino with 60ml of milk, 17ml of espresso, plus a second added shot of 17ml of espresso. 

3. The third index here prepares a "cortado" with 30ml of milk, 17ml of espresso, plus a second added shot of 17ml of espresso.

4. The final operation will cause the secret menu to close. The menu will time out after a short period of time as well. 15 seconds by default. 

To add other options, simply insert a new array item. For example, a two-shot americano: 

```
{
  " AMERI x2",
  MQTT_ROOT MQTT_SUBTOPIC_FUNCTION "make_hot_water",
  "{'add':2,'brew':17}",
}
```

Or a lungo:

```
{
  "  LUNGO",
  MQTT_ROOT MQTT_SUBTOPIC_FUNCTION "make_espresso",
  "{'brew':40}",
}
```

In other cases, you can define your own personal brew preferences that will apply to the next physical button press of the machine. In the following example, pressing any of the program buttons will cause the next button press to limit brew output volume to 17 ml.

```
{
  "ESPRESSO 1",
  MQTT_ROOT MQTT_DISPENSE_CONFIG,
  "{'brew':17}",
}
```

Or, simply add a shot to any other preparation. In this case, we can automatically add a 17ml espresso pull after any other machine button. After enabling this option, the display will update to "PRODUCT?", encouraging selection of one of the six prefab buttons (espresso, cappuccino, macchiato, water, milk foam, coffee). Once the use makes a selection and the selected program completes, a shot will be pulled:

```
{
  " ADD SHOT",
  MQTT_ROOT MQTT_DISPENSE_CONFIG,
  "{'add':1,m 'brew':17}",
}
```
<hr/>

# Function Buttons in Home Assistant

The following lists what buttons are reportable to home assistant. By default, most are disabled as each additional sensor adds reporting time and cycle time. However, use cases may vary so the options exist in [JuraConfiguraton.h](https://github.com/andrewjfreyer/jurabridge/blob/main/v2/JuraConfiguration.h)

| Function ID |  Description / Name | Enabled by Default |
| --- | -  | - |
|JuraFunctionIdentifier::PowerOff| "Power Off"|No|
|JuraFunctionIdentifier::ConfirmDisplayPrompt| "Confirm Display Prompt"|No|
|JuraFunctionIdentifier::SettingsMenu| "Settings Menu"|Yes|
|JuraFunctionIdentifier::SelectMenuItem| "Select Menu Item"|No|
|JuraFunctionIdentifier::RotaryRight| "Rotary Right"|No|
|JuraFunctionIdentifier::RotaryLeft| "Rotary Left"|No|
|JuraFunctionIdentifier::MakeEspresso| "Make Espresso"|Yes|
|JuraFunctionIdentifier::MakeCappuccino| "Make Cappuccino"|Yes|
|JuraFunctionIdentifier::MakeCoffee| "Make Coffee"|Yes|
|JuraFunctionIdentifier::MakeMacchiato| "Make Macchiato"|No|
|JuraFunctionIdentifier::MakeHotWater| "Make Hot Water"|Yes|
|JuraFunctionIdentifier::MakeMilkFoam| "Make Milk Foam"|Yes|
|JuraFunctionIdentifier::MenuSelectRinseBrewGroup| "Rinse Brew Group"|Yes|
|JuraFunctionIdentifier::MenuSelectRinseMilkSystem| "Rinse Milk System"|Yes|
|JuraFunctionIdentifier::MenuSelectCleanMilkSystem| "Clean Milk System"|Yes|
|JuraFunctionIdentifier::MenuSelectCleanBrewGroup| "Clean Brew Group"|Yes|
|JuraFunctionIdentifier::RefreshMQTTConfiguration| "Refresh MQTT Configuration"|Yes|

<hr/>

# Sensors Reportable to Home Assistant

The following lists what sensors are reportable to home assistant. By default, most are disabled as each additional sensor adds reporting time and cycle time. However, use cases may vary so the options exist in [JuraConfiguraton.h](https://github.com/andrewjfreyer/jurabridge/blob/main/v2/JuraConfiguration.h)
| State ID |  Description / Name | Enabled by Default |
| --- | -  | - |
|JuraMachineStateIdentifier::OperationalState|Operational State| Yes|
|JuraMachineStateIdentifier::ReadyStateDetail|Ready State Detail| Yes|
|JuraMachineStateIdentifier::NumEspresso|Espresso| Yes|
|JuraMachineStateIdentifier::NumCoffee| Coffee| Yes|
|JuraMachineStateIdentifier::NumCappuccino| Cappuccino| Yes|
|JuraMachineStateIdentifier::NumMacchiato| Macchiato| Yes|
|JuraMachineStateIdentifier::NumPreground| Preground| Yes|
|JuraMachineStateIdentifier::NumDriveMotorOperations| Drive Motor Operations| Yes|
|JuraMachineStateIdentifier::NumLowPressurePumpOperations| Low Pressure| Yes|
|JuraMachineStateIdentifier::NumBrewGroupCleanOperations| Cleanings of Brewgroup| Yes|
|JuraMachineStateIdentifier::NumSpentGrounds| Spent Grounds| Yes|
|JuraMachineStateIdentifier::SpentGroundsLevel| Knockbox Fill Level| Yes| 
|JuraMachineStateIdentifier::NumPreparationsSinceLastBrewGroupClean| Since Clean| Yes|
|JuraMachineStateIdentifier::NumHighPressurePumpOperations| High Pressure| Yes|
|JuraMachineStateIdentifier::NumMilkFoamPreparations| Milk Foam| Yes|
|JuraMachineStateIdentifier::NumWaterPreparations| Hot Water| Yes|
|JuraMachineStateIdentifier::NumGrinderOperations| Grindings| Yes|
|JuraMachineStateIdentifier::NumCleanMilkSystemOperations| Cleanings of Milk System| Yes|
|JuraMachineStateIdentifier::HasFilter| Water Filter| Yes|
|JuraMachineStateIdentifier::BeanHopperEmpty| Bean Hopper Empty| Yes|
|JuraMachineStateIdentifier::BeanHopperCoverOpen| Bean Hopper Open| Yes|
|JuraMachineStateIdentifier::SpentBeansByWeight| Beans Consumed| Yes|
|JuraMachineStateIdentifier::BeanHopperLevel| Bean Hopper Fill Level| Yes|
|JuraMachineStateIdentifier::WaterReservoirNeedsFill| Water Reservoir| Yes|
|JuraMachineStateIdentifier::BypassDoserCoverOpen| Bypass Doser Open| Yes|
|JuraMachineStateIdentifier::DrainageTrayRemoved| Drip Tray Removed| Yes|
|JuraMachineStateIdentifier::DrainageTrayFull| Drip Tray Full| Yes|
|JuraMachineStateIdentifier::DrainageTrayLevel| Drip Tray Level| Yes|
|JuraMachineStateIdentifier::DrainageSinceLastTrayEmpty| Drainage Volume| Yes|
|JuraMachineStateIdentifier::FlowState| Flow State|No|
|JuraMachineStateIdentifier::BrewGroupPosition| Brew Group Position|No|
|JuraMachineStateIdentifier::BrewGroupDriveShaftPosition| Brew Group Drive Shaft Position|No|
|JuraMachineStateIdentifier::BrewGroupEncoderState| Brew Group Encoder State|No|
|JuraMachineStateIdentifier::OutputValveEncoderState| Output Valve Encoder State|No|
|JuraMachineStateIdentifier::OutputValveNominalPosition| Output ValveNo|No|
|JuraMachineStateIdentifier::HasError| Error| Yes|
|JuraMachineStateIdentifier::HasMaintenanceRecommendation| Maintenance Recommendation| Yes|
|JuraMachineStateIdentifier::RinseBrewGroupRecommended| Water Rinse Recommended| Yes|
|JuraMachineStateIdentifier::ThermoblockPreheated| Thermoblock Preheated|No|
|JuraMachineStateIdentifier::ThermoblockReady| Thermoblock Ready|No|
|JuraMachineStateIdentifier::OutputValvePosition| Output Valve Position|No|
|JuraMachineStateIdentifier::OutputValveIsBrewing| Output Valve Brewing Position|No|
|JuraMachineStateIdentifier::OutputValveIsFlushing| Output Valve Flushing Position|No|
|JuraMachineStateIdentifier::OutputValveIsDraining| Output Valve Draining Position|No|
|JuraMachineStateIdentifier::OutputValveIsBlocking| Output Valve Blocking Position|No|
|JuraMachineStateIdentifier::WaterPumpFlowRate| Flow Rate| Yes|
|JuraMachineStateIdentifier::LastDispensePumpedWaterVolume| Last Dispense Pumped Water Volume| Yes|
|JuraMachineStateIdentifier::LastMilkDispenseVolume| Last Milk Dispense Volume| Yes|
|JuraMachineStateIdentifier::LastWaterDispenseVolume| Last Water Dispense Volume| Yes|
|JuraMachineStateIdentifier::LastBrewDispenseVolume| Last Brew Dispense Volume| Yes|
|JuraMachineStateIdentifier::LastDispenseMaxTemperature| Last Dispense Max Temperature| Yes|
|JuraMachineStateIdentifier::LastDispenseType| Last Dispense Type| Yes|
|JuraMachineStateIdentifier::LastDispenseGrossTemperatureTrend| Last Dispense Gross Temperature Trend| Yes|
|JuraMachineStateIdentifier::LastDispenseDuration| Last Dispense Duration| Yes|
|JuraMachineStateIdentifier::LastDispenseMinTemperature| Last Dispense Min Temperature| Yes|
|JuraMachineStateIdentifier::LastDispenseAvgTemperature| Last Dispense Average Tempreature| Yes|
|JuraMachineStateIdentifier::ThermoblockTemperature| Thermoblock Temperature| Yes|
|JuraMachineStateIdentifier::ThermoblockStatus| Thermoblock Status|No|
|JuraMachineStateIdentifier::ThermoblockColdMode| Cold Temperature|No|
|JuraMachineStateIdentifier::ThermoblockLowMode| Low Temperature|No|
|JuraMachineStateIdentifier::ThermoblockHighMode| High Temperature|No|
|JuraMachineStateIdentifier::ThermoblockOvertemperature| Overtemperture| Yes|
|JuraMachineStateIdentifier::ThermoblockSanitationTemperature| Sanitation|No|
|JuraMachineStateIdentifier::SystemWaterMode| Hot Water Mode|No|
|JuraMachineStateIdentifier::SystemSteamMode| Steam Mode|No|
|JuraMachineStateIdentifier::BrewGroupLastOperation|Last Brew Group Operation|No|
|JuraMachineStateIdentifier::CeramicValvePosition| Ceramic Valve Position|No|
|JuraMachineStateIdentifier::CeramicValveCondenserPosition| Ceramic Valve Condenser Position|No|
|JuraMachineStateIdentifier::CeramicValveHotWaterPosition| Ceramic Valve Hot Water Position|No|
|JuraMachineStateIdentifier::CeramicValvePressurizingPosition| Ceramic Valve Pressurizing Position|No|
|JuraMachineStateIdentifier::CeramicValveBrewingPosition| Ceramic Valve Brewing Position|No|
|JuraMachineStateIdentifier::CeramicValveSteamPosition| Ceramic Valve Steam Position|No|
|JuraMachineStateIdentifier::CeramicValveVenturiPosition| Ceramic Valve Venturi Position|No|
|JuraMachineStateIdentifier::CeramicValvePressureReliefPosition| Ceramic Valve Pressure Relief Position|No|
|JuraMachineStateIdentifier::ThermoblockMilkDispenseMode| Thermoblock Milk Dispense Mode|No|
|JuraMachineStateIdentifier::VenturiPumping| Venturi Pumping|No|
|JuraMachineStateIdentifier::ThermoblockDutyCycle| Thermoblock Duty Cycle|No|
|JuraMachineStateIdentifier::ThermoblockActive| Thermoblock Active| Yes|
|JuraMachineStateIdentifier::PumpDutyCycle| Pump Duty Cycle|No|
|JuraMachineStateIdentifier::PumpActive| Pump Active|No|
|JuraMachineStateIdentifier::GrinderDutyCycle| Grinder Duty Cycle|No|
|JuraMachineStateIdentifier::GrinderActive| Grinder Active|No|
|JuraMachineStateIdentifier::BrewGroupDutyCycle| Brew Group Duty Cycle|No|
|JuraMachineStateIdentifier::BrewGroupActive| Brew Group Active|No|
|JuraMachineStateIdentifier::BrewGroupIsRinsing| Brew Group Rinsing|No|
|JuraMachineStateIdentifier::BrewGroupIsReady| Brew Group Ready|No|
|JuraMachineStateIdentifier::BrewGroupIsLocked| Brew Group Locked|No|
|JuraMachineStateIdentifier::BrewGroupIsOpen| Brew Group Open|No|
|JuraMachineStateIdentifier::BrewProgramIsReady| Brew Program Ready|No|
|JuraMachineStateIdentifier::BrewProgramIsCleaning| Tablet Cleaning| Yes|
|JuraMachineStateIdentifier::GroundsNeedsEmpty| Grounds Needs Empty| Yes|
|JuraMachineStateIdentifier::SystemIsReady| System Ready| Yes|
|JuraMachineStateIdentifier::RinseMilkSystemRecommended| Milk Rinse Recommended| Yes|
|JuraMachineStateIdentifier::CleanMilkSystemRecommended| Milk Clean Recommended| Yes|
|JuraMachineStateIdentifier::CleanBrewGroupRecommendedSoon| System Clean Recommended| Yes|
|JuraMachineStateIdentifier::CleanBrewGroupRecommended| System Clean Required| Yes|
|JuraMachineStateIdentifier::HasDose| Dose in Brew Group|No|


## Disclaimer

*This repository is only provided as information documenting a project I worked on for kicks. No warranty or claim that this will work for you is made. I have described some actions that involve modifying a Jura ENA Micro 90, which if performed will absolutely void any warranty you may have (which you probably don't; this machine is old). Some of the modifications described below involve mains electricity; all appropriate cautions are expected to be, and were, followed. Some of these modifications are permanent and irreversible without purchasing replacement parts. Do not duplicate any of this project unless you know exactly what you are doing. I do not take **any** responsibility for any frustration, spousal irritation, bad coffee, injuries, or damage to your machine, your home plumbing, your home, or anyone or anything that may occur from any accident, lack of common sense or experience, poor planning, or following this project word for word. Responsibility for any effect, including damages or distress, resulting from reliance on any information made available here is not the responsibility of the author or other contributors. All rights are reserved.*
