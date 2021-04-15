# Sample Home Assistant Files

### These are sample .yaml files that you can use or adapt to integrate the clock into Home Assistant

All communications between the clock (ESP8266) and Home Assistant are via MQTT.  Currently, the clock does not support MQTT discovery (anyone willing to take a stab at this is welcome to submit a PR), therefore all entites must be manually created in Home Assistant.  Obviously, you must have an MQTT broker running (either the integrated HA Mosquitto or a standalone broker).

While MQTT is not technically required to build and use the clock, many features can only be set and controlled via MQTT because the built-in web application does not support these additional items. For example, text mode and effects are only available via MQTT commands.  However, MQTT can be disabled within the Arudino sketch by setting MQTTMODE = 0 in the Settings.h file. See the [wiki](https://github.com/Resinchem/LED-Matrix_Clock_WS2812b/wiki) for additional details.

The samples include every possible sensor value and commands available for the clock.  You may not wish to include them all.

The entities also have long descriptive names in an attempt to clarify how they are used.

You can safely edit most of the yaml to fit your needs.  However, you cannot changes any of the MQTT topics or payloads without also modifying and uploading a new Arduino .ino file to the ESP8266 board.

For the Lovelace card, two custom cards (available via HACS) are necessary:

[Custom Button Card](https://github.com/custom-cards/button-card)

[Text-Divider-Row](https://github.com/iantrich/text-divider-row)

The files here are just starting examples.  Many, many more automations are possible and if you'd like to share any you develop, I'll post the interesting ones here.  But some examples might include:
- Change the clock color and/or brightness based on sun (or any other event)
- Sound the buzzer based on any other HA event or state change.
- Use the countdown feature to trigger other automations.

With over 40 MQTT sensors and over 30 MQTT commands, the possibilites are endless. See a full list of MQTT topcis in the MQTT folder.
