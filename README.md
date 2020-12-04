# Bike Speedometer and Odometer
A completed Arduino project to create a speedometer that doubles as an odometer for a bicycle using a hall effect sensor and an Arduino Nano ([ATmega328p](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf)) for a university course. The nano's inbuilt timers and interrupts are used directly here.

## Required Functionality
* 4 digit display (2 decimal places) to display current speed in km/h and distance in km
* Ability to toggle in between displays
* Button to reset odometer distance
* Ability to recognise idleness
* Ability to set the circumference of the wheel

## Electronic Components
* Arduino Nano ([ATmega328p](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf))
* Hall-effect sensor
* Rotary Encoder Switch
* 4 LEDs
* 2 4-digit 7-segment displays with decimal point
* 7-segment decoder
* 8 Transistors
* 1:8 Demultiplexer
* Resistors and capacitors (debouncing) where necessary

## Circuit
Full circuit not to be included here. Transistors and demultiplexer are set up to allow for the multiplexing of the 7-segment displays. 

## Arduino Pin Mapping (More in detail in [code file](https://github.com/jiekelove7/bikeproj/blob/master/project_attempt.ino))
Consult [datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf) if unsure about pin mapping.
| Functionality  | Pin No. (0 - 19) | Port & No.       |
| -------------  | ---------------- | ----------       |
| Rotary Encoder | 2, 4, 6          | D2, D4, D6       |
| Hall-effect    | 8                | B0               |
| BCD Decoder    | 14 - 15, 18 - 19 | C0 - C1, C4 - C5 |
| Display - DP   | 17               | C3               |
| LEDs           | 0 - 1, 5, 7      | D0 - D1, D5, D7  |
| Demultiplexer  | 11 - 13          | B3 - B5          |

## Functionality
In normal operation, twisting the rotary encoder will change the settings (of what it displays) from speed to max speed to average speed. The LED's will light up accordingly. The distance (odometer) is always visible on the second display.

Tapping the button will reset the odometer (and consequently the average speed readings). Holding the button will change to 'circumference mode' where twisting the knob will now change the circumference of the wheel, displayed on the first display. Tapping the button after will revert to normal mode.
