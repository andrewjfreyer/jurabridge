# jurabridge ☕

# Description

This is an ESP32 Arduino project for bridging a [Jura Ena Micro 90](https://us.jura.com/en/customer-care/products-support/ENA-Micro-90-MicroSilver-UL-15116) to home automation platforms via MQTT. Main controller is an ESP32. A 3.3v to 5v level shifter is required between an available hardware UART of the ESP32 to the debug/service port of the Jura. Don't use `softwareserial`, as it's painfully slow. 

See here for [hardware](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware) and [software](https://github.com/andrewjfreyer/jurabridge/wiki/software) requirements. [Connections are straightforward.](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware#connection-diagram)

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/ena90.png" alt="Jura Ena Micro 90"/>
</p>

# Table of Contents

* [Jura Ena Micro 90 Command/Response Investigations & Interpretations](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Commands)

* [MQTT Topics & Payloads](https://github.com/andrewjfreyer/jurabridge/wiki/MQTT-Topics)

* [Relevant Schematics](https://github.com/andrewjfreyer/jurabridge/wiki/Schematic(s))

* [Ena Micro 90 Mods](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Modifications)

* [Required & Optional Hardware](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware)

* [Arduino Setup & Upload](https://github.com/andrewjfreyer/jurabridge/wiki/Software)

* [Custom Preparation Scripts](https://github.com/andrewjfreyer/jurabridge/wiki/Custom-Recipe-Scripts)

* [Home Assistant Configuration Example](https://github.com/andrewjfreyer/jurabridge/wiki/Home-Assistant-Configuration)

* [References](https://github.com/andrewjfreyer/jurabridge/wiki/References)


## Home Assistant

Here's my Home Assistant [configuration (YAML package)](https://github.com/andrewjfreyer/jurabridge/wiki/Home-Assistant-Configuration). Here is what the bridge looks like, attached to the machine after a few destructive [modifications](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Modifications) to feed a ribbon cable to the debug port. 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_housed.png" alt="Jura Ena Micro 90"/>
</p>

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_unhoused.png" alt="Jura Ena Micro 90"/>
</p>

Here's a quick video demo showing the responsiveness of the polling:

<div align="center">
      <a href="https://youtu.be/6NN9Xv9Lhq4">
         <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/youtube.png" style="width:80%;">
      </a>
</div>

The data output from the machine is received and presented by [Home Assistant.](https://www.home-assistant.io) I have created this status UI based on a modified fork of [button-card](https://github.com/custom-cards/button-card). 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_on.png" alt="BridgeOn"/>
</p>

<hr/>

# Custom Preparations & Actions

I've found out that the output of the machine is actually quite misleading about the machine's capabilities. The Jura produces good enough shots as is. I never thought to question the dosing volume, as the output was good enough for what I expected from a superautomatic. Since I bought the machine, my expectation was set that "this is the best the Jura can do, and that's just fine." However, somewhat surprising to me was that the Ena Micro 90 only uses ***7-10g of coffee per perparation*** - about half as much as I presumed. So, what if we just pull two shots back to back, at lower volume? Improved flavor, in my opinion, even if the shots do not look significantly different.

It's of course easy to pull two shots back to back, and to interrupt brewing (or to save the preparation settings) to lower volumes. Instead, we can use `jurabridge` to automate this sequence for us. Specifically, because `jurabridge` obtains and/or infers accurate machine status information, any number of custom recipes or custom instruction sequences can be excuted, without needing to modify EEPROM or to orchestrate a valid sequence of `FN:` commands, or without waiting for unnecessary long delays and presuming machine states. This command+interrupt technique ensures that the machine excutes its own in-built sequences, and there's no risk of incidentally damaging the brewgroup with custom instructions or custom brew sequences. 

Custom sequences/scripts can be made to allow the machine to produce a wide variety of other drinks, at higher quality. As a trivial example, different settings may be appropriate for non-dairy cappuccino than dairy cappuccino, yet the machine only has one setting. 

[Custom recipe examples.](https://github.com/andrewjfreyer/jurabridge/wiki/Custom-Recipe-Scripts)

<hr/>

## Disclaimer

*This repository is only provided as information documenting a project I worked on. No warranty or claim that this will work for you is made. I have described some actions that involve modifying a Jura Ena Micro 90, which if performed will void any warranty you may have. Some of the modifications described below involve mains electricity; all appropriate cautions are expected to be, and were, followed. Some of the modifications are permanent and irreversible without purchasing replacement parts. Do not duplicate any of the following unless you know exactly what you are doing. I do not take anyresponsibility for any frustration, spousal irritation, injuries, or damage that may occur from any accident, lack of common sense or experience, or poor planning. Responsibility for any damages or distress resulting from reliance on any information made available here is not the responsibility of the author or other contributors. All rights are reserved.*

*Note: Although loosely compatible, this code is far too large to work with ESPHome; attempts to get it to work usefully with all sensors updating at reasonable intervals caused watchdog crashes.*

