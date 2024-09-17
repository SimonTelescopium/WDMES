### WDMES SERVO POWER CONTROLLER

STATUS: Working and in use

## Problem

Our OO gauge railway model has a lot of points controlled by half a dozen MegaPoints MQTT Controllers. Unfortunatly these controllers when powered on all 'fight' for a Wifi connection at the same time, and generally one or two will fail to gain a connection before timing out. This project is designed to solve this problem.

## Solution

We are using an Arduino nano to power-up the MegePoints servo controllers one at a time with a 4s delay between each, this is enouth of a gap to ensure the individule controllers can establish an IP address with the router sucessfully.

This project uses an 8x relay board which the arduino controls, so is suitable for any use case that requires powering up low voltage equipment in sequence.

