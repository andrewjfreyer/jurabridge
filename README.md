# jurabridge ☕

# Description

This is an ESP32 Arduino project for bridging a [Jura ENA Micro 90](https://us.jura.com/en/customer-care/products-support/ENA-Micro-90-MicroSilver-UL-15116) to home automation platforms via MQTT. Main controller is an ESP32. A 3.3v to 5v level shifter is required between an available hardware UART of the ESP32 to the debug/service port of the Jura. Don't use `softwareserial`, as it's painfully slow. The ESP continually polls the Jura for status information, and reports changes in status information via MQTT. 

See here for [hardware](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware) and [software](https://github.com/andrewjfreyer/jurabridge/wiki/software) requirements. [Connections are straightforward.](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware#connection-diagram)

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/ena90.png" alt="Jura ENA Micro 90"/>
</p>


# Why? Why do you do this? It's a discontinued machine...

For fun, that's all. I saw [this](https://github.com/ryanalden/esphome-jura-component/) and other similar projects and thought I'd begin investigating whether some or all of the functionality would extend to the Jura ENA Micro 90. Many did, many did not. I needed to investigate and characterize a lot of the output from the machine. One thing led to another, and now we're here.


# Table of Contents [Under Construction!]

### Reverse Engineering 

* [Jura ENA Micro 90 Command/Response Investigations & Interpretations](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Commands)

* [Relevant Schematics](https://github.com/andrewjfreyer/jurabridge/wiki/Schematic(s))

### Building & Programming the `jurabridge`

* [Required & Optional Hardware](https://github.com/andrewjfreyer/jurabridge/wiki/Hardware)

* [Arduino Setup & Upload](https://github.com/andrewjfreyer/jurabridge/wiki/Software)

### Using `jurabridge` for Stuff

* [MQTT Topics & Payloads](https://github.com/andrewjfreyer/jurabridge/wiki/MQTT-Topics)

* [Jura ENA Micro 90 Mods](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Modifications)

### Acknowledgements & Further Reading

* [References](https://github.com/andrewjfreyer/jurabridge/wiki/References)


# Home Assistant

Here's my Home Assistant [configuration (YAML package)](https://github.com/andrewjfreyer/jurabridge/wiki/Home-Assistant-Configuration). Here is what the bridge looks like, attached to the machine after a few destructive [modifications](https://github.com/andrewjfreyer/jurabridge/wiki/Jura-Ena-Micro-90-Modifications) to feed a ribbon cable through the housing to the debug port. 

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_housed.png" alt="Jura Ena Micro 90"/>
</p>

<p align="center">
  <img src="https://github.com/andrewjfreyer/jurabridge/raw/main/images/bridge_unhoused.png" alt="Jura Ena Micro 90"/>
</p>

The data output from the machine is received and presented by [Home Assistant presented via MQTT Device Discovery.](https://www.home-assistant.io) 

<hr/>

# Custom Preparations & Actions

I've found out that the default programming of the machine undersells/underutilizes machine's capabilities. The Jura produces good enough shots as is. I never thought to question the dosing volume, as the output was good enough for what I expected from a super-automatic. 

All that to say, since I bought the machine, my expectation was set that "this is the best the Jura can do, and that's just fine." It is, after all, a superauto. We're sacrificing some quality for pushbutton convenience. However, somewhat surprising to me was that the ENA Micro 90 only uses ***7-10g of coffee per perparation*** - about half as much as I presumed. 

It's of course easy to pull two shots back to back to get to a more traditional 15 - 20g per pull. But that requires that I sit and wait, read words on a display, clear any errors, and press buttons twice. Who has time for that?

Lets just automate it. 

Here, because `jurabridge` obtains (and/or infers) accurate machine status information, any number of custom recipes or custom instruction sequences can be excuted, without needing to modify EEPROM or to orchestrate a valid sequence of `FN:` commands, or without waiting for unnecessary long delays to presuming machine state. This command+interrupt+statuswait technique ensures that the machine excutes its own in-built sequences, and there's no risk of incidentally damaging the brewgroup with custom instructions or custom brew sequences. 

<hr/>

## Disclaimer

*This repository is only provided as information documenting a project I worked on. No warranty or claim that this will work for you is made. I have described some actions that involve modifying a Jura ENA Micro 90, which if performed will void any warranty you may have. Some of the modifications described below involve mains electricity; all appropriate cautions are expected to be, and were, followed. Some of these modifications are permanent and irreversible without purchasing replacement parts. Do not duplicate any of this project unless you know exactly what you are doing. I do not take anyresponsibility for any frustration, spousal irritation, bad coffee, injuries, or damage to your machine, your home plumbing, your home, or anyone or anything that may occur from any accident, lack of common sense or experience, poor planning, or following this project word for word. Responsibility for any effect, including damages or distress, resulting from reliance on any information made available here is not the responsibility of the author or other contributors. All rights are reserved.*
