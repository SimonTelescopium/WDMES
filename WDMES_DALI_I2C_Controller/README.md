### WDMES_DALI_I2C_Controller

DALI I2C Lighting controller using an Arduino nano

## Purpose


Control an overhead DALI LED light using an Arduino set-up as an I2C slave so that the overhead light acts as a model Sun, dimming at dusk until off and brightening at dawn until fully on.

## STATUS:

This is a work in progress - this code currently doesn't doesn't work or is very unstable


## Definitions

DALI is a lighting control protocol/interface.


## Approach

There will be a physical circuit as described here https://projecthub.arduino.cc/NabiyevTR/simple-dali-controller-c652b7 interfacing the DALA light to the Arduino. the Arduino will receive commands on the I2C interface with a target brightness and duration to get to that brightness.

The idea is to take a DALI controlled LED light and use it as an artificial Sun in the Welling & District Model Engineering Society (WDMES) OO gauge railway model.

This specific project is to create an I2C interface using an Arduino Nano which then communicates using a simple circuit with the DALI interface on the light, in this way we can use I2C to control the DALI light, altering it's brightness to simulate twilight and full sun.

The Arduino is dumb in that it simple does what it is told via the I2C interface.
The interface is stateless, it will change the lighting levels to whatever the last command requested over the duration teh command stated.

This project is based heavily on the project here; https://projecthub.arduino.cc/NabiyevTR/simple-dali-controller-c652b7 Uber thanks go out to the author of this project and the DALI Arduino library

## Core Concepts

1) OHDLCON will be an I2C slave
2) OHDLCON is stateless - i.e. it doesn't report it's state, it just receives commands and acts on them
3) OHDLCON is Fire and forget - it doesn't return status, errors or anything else on the I2C bus, but will send information on the serial port of the Arduino for troubleshooting and debugging
4) The controller that instructs OHDLCON is expected to assume success and shouldn't hang if the OHDLCON is not connected to the I2C bus
5) OHDLCOM will not require an I2C controller to work - It will have manual override controls.
6) OHDLCON will monitor the I2C port, if it receives an I2C command whilst still executing the previous command it will abort the previous command and start executing the new command. This is to ensure it always acts on the latest user requirements.

## I2C Command Structure

_There will only be one I2C command, this will consist of 2 parameters, BRIGHTNESS and TIME_

BRIGHTNESS 0-254 0 = Off,  254 = maximum brightness.

Time = number of milli-seconds to achieve that brightness.

The OHDLCON will determine the smoothest way to achieve the desired brightness in the desired time.
