# Juniper Source Code

## blink.jun

The hello_world of arduino programming. 

Just blinks the onboard LED every second.

## fade.jun

Utilizes an RGB LED. With the following pin connections:

* Cathode attached to ground
* Red to pin 6
* Green to pin 5
* Blue to pin 3

> With a 220 Ohm resistor between the pin and the led leads.

This program just cycles through the color spectrum of the RGB LED.

## toggleLed.jun

Utilizes a LED and a basic button. Anytime the button is pressed/engaged the LED is turned on. When the button is released/disengaged the LED turns off.

* LED utilizes pin 5 with a 220 Ohm resistor
* Button utilizes pin 9

## digitalInputs.jun

Utilizes a LED and two buttons. When button A is pressed/engaged, the LED is turned on. When button B is pressed/engaged the LED is turned off.

* LED utilizes pin 5 with a 220 Ohm resistor
* ButtonA utilizes pin 9
* ButtonB utilizes pin 8

> This is the same circuit as `toggleLed.jun` except with one more button added to pin 8 for explicit on/off support
