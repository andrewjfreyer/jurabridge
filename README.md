# jurabridge ☕

# TL;DR

(v2) ESP32 Arduino project for bridging a [Jura ENA Micro 90](https://us.jura.com/en/customer-care/products-support/ENA-Micro-90-MicroSilver-UL-15116) to home automation platforms via MQTT. A 3.3v to 5v level shifter is required between hardware UART of the ESP32 to the service port of the Jura. The ESP32 polls the Jura for status information via reverse-engineered Jura Service Port calls, calculates/determines device state and meta statuses and reports changes to an MQTT broker. Added custom preparation functionality via a separate input button.

With exposed sensors, a UI can be built for Home Assistant that provides tons of information about the machine and its preparations. For example, with [button-card](https://github.com/custom-cards/button-card) and others: 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/ui_walkthrough.gif" alt="Jura Ena Micro 90"/>
</p>


See here for [hardware](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware) and [software](https://github.com/andrewjfreyer/jurabridge/wiki/software) requirements. [Connections are straightforward.](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware#connection-diagram)

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/ena90.png" alt="Jura ENA Micro 90"/>
</p>


# Why? It's a discontinued machine...

For fun, that's all. I saw [this](https://github.com/ryanalden/esphome-jura-component/) and other projects and thought I'd begin investigating whether some or all of the functionality would extend to the Jura ENA Micro 90. Many did, many did not. Turns out, I needed to investigate and characterize a lot of the output from the machine. One thing led to another, and now we're here.


# Table of Wiki Contents

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

# Custom Preparations & Actions

I've found out that the default programming dramatically undersells the machine's capabilities. The Jura produces good enough shots as is. All that to say, since I bought the machine, my expectation was set that "this is the best it can do, and that's just fine." It is, after all, a superauto and sacrifies are made over a manual process. We're sacrificing some quality for pushbutton convenience. However, surprising to me was that the ENA Micro 90 only uses ***7-10g of coffee per perparation*** - with 30 some-odd ml of water. That's less coffee and more water than I presumed, without giving it much thought. Plus, pump pressure dropps pretty dramatically due to channeling so, the first parts of our shots are the best parts anyway. 

It's of course easy to pull two short shots back to back to get to a more traditional 15 - 20g "single" shot, but why not automate it? 
Lets just automate it. Here, because `jurabridge` obtains (and/or infers) accurate machine status information, any number of custom recipes or custom instruction sequences can be excuted, without needing to modify EEPROM or to orchestrate a valid sequence of `FN:` commands, or without waiting for unnecessary long delays to presuming machine state. This command+interrupt+statuswait technique ensures that the machine excutes its own in-built sequences, and there's no risk of incidentally damaging the brewgroup with custom instructions or custom brew sequences. 

In this project there's a header above that defines custom functions that are accessible via a pushbutton. As an example, there are three custom operations defined. This can be modified to include as many custom functions as needed:

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
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/menu_demo.mov" alt="Jura Ena Micro 90"/>
</p>

The first index prepares a double ristretto in which two espresso shots are pulled back to back, each limited to 17ml of espresso output (that's about 2x the grounds volume of 8g). 

The second index here prepares a cappuccino with 60ml of milk, 17ml of espresso, plus a second added shot of 17ml of espresso. 

The third index here prepares a "cortado" with 30ml of milk, 17ml of espresso, plus a second added shot of 17ml of espresso.

The final operation will cause the secret menu to close. The menu will time out after a short period of time as well. 15 seconds by default. 

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
|JuraMachineStateIdentifier::HasDoes| Dose in Brew Group|No|


## Disclaimer

*This repository is only provided as information documenting a project I worked on for kicks. No warranty or claim that this will work for you is made. I have described some actions that involve modifying a Jura ENA Micro 90, which if performed will absolutely void any warranty you may have (which you probably don't; this machine is old). Some of the modifications described below involve mains electricity; all appropriate cautions are expected to be, and were, followed. Some of these modifications are permanent and irreversible without purchasing replacement parts. Do not duplicate any of this project unless you know exactly what you are doing. I do not take **any** responsibility for any frustration, spousal irritation, bad coffee, injuries, or damage to your machine, your home plumbing, your home, or anyone or anything that may occur from any accident, lack of common sense or experience, poor planning, or following this project word for word. Responsibility for any effect, including damages or distress, resulting from reliance on any information made available here is not the responsibility of the author or other contributors. All rights are reserved.*
