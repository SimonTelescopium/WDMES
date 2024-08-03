Purpose:
========
This is the documentation for the Welling and District Model Engineering Society OO Gauge lighting model.

The lighting model is executed by an Arduino nano, with a number of peripherals

The lights are grouped into types which have different behaviour.

Platform Lights:
---------------
These are controlled by a single PCF8574 GPIO extender using I2C up to 6 platforms can be controlled.

Behaviour: 
these lights illuminate one platform at a time to simulate the station master walking from platform to platform turning on/off the lights. Lights turn on at dusk and off at dawn.

Street Lights:
--------------
These are controlled by a single PCF8574 GPIO extender using I2C up to 8 streetlights can be controlled.
Behaviour:
these lights turn on and off in a random order and with a random period between each light, to simulate how streetlights turn on/off independently to lighting conditions. Lights turn on at dusk and off at dawn.

Building lights:
---------------
These are controlled by a single PCF8574 GPIO extender using I2C up to 8 building lights can be controlled.
Behaviour: 
these lights turn on and off in a random order and with a random period between each light, to simulate how people turn on/off their lights. Lights turn on at dusk and off at bedtime (midnight).

Special lights:
--------------
We have a model of our  clubhouse in our model railway, the lights and welder (in the workshop)in the model clubhouse turn on and off according to our club opening hours. :-)


Controlling the lights:
-----------------------
Lights are controlled by a rotary switch which enables the controller to select between Day, Evening, Night and Auto, the switch is connected to an I2C GPIO extender (PCF8574) and feedback to the user is via an I2C 16x2 LCD display.
The model also incorporates days of the week, so that the special lights only  turn on on the correct days of the week according to the model.

- **Auto:**
In auto-mode lights cycle through at a rate of 48s = 1hr this is approximately the same as OO gauge scale. 
- **Day:**
Platform Lights - off
Building Lights off
Special Lights on
Street Lights off
- **Evening:**
Platform Lights - on
Building Lights on
Special Lights on
Street Lights on
- **Night:**
Platform Lights - on
Building Lights off
Special Lights off
Street Lights on



Controller feedback:
-------------------
An I2C 16x2 LCD display shows the models day of week, time, Period of day and status.
e.g.

    Wednesday  19:00
    Evening  Running

    Monday     03:00
    Night     Paused

The periods of the day are split into the following...
Night, Twilight, Day, Twilight, Evening


