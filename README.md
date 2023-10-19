# IoT Smart Parking System

Early prototype of the IoT enabled smart parking system.

<div>
    <img alt="Smart Parking Schematic" src="hardware/Fritzing/SmartParking_WiringDiagram.png" />
</div>

## Features :-

- One Mater login to manage all the user operations.
- Check for availability of the parking slot in real time.
- Dynamically deduct the charge from the user card based on the usage.
- Real time data view that let's other apps pull this information to do something usefull :)

## Hardware :-

- Servo Motor
- [MFRC522 Card Reader](https://robu.in/product/rc522-rfid-card-reader-module-13-56mhz/)
- [NodeMCU ESP-12E Dev Board](https://robu.in/product/nodemcu-cp2102-board/)
- RGB LED's and bunch of wires

## Getting Started

Jumping to the project is super easy, just do the wiring connections and build the project with platform io addition for arduino esp8266 framework. You can find the information below.

## Connections :-

Please refer the wiring diagram inside the hardware directory. Also there is [fritzing workspace](./hardware/Fritzing/SmartParking_WiringDiagram.fzz) as well for editing. If you wish to change the pin mapping consider changing in [pins_config.h](./include/pins_config.h) file too. 

## Dependencies :-
- [MFRC522](https://github.com/miguelbalboa/rfid/tree/1.4.10)
- [NTPClient](https://github.com/arduino-libraries/NTPClient/tree/3.2.1)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson/tree/v5.13.4)
- [FirebaseArduino](https://github.com/FirebaseExtended/firebase-arduino/tree/v0.3) 
- [ESP8266 Core SDK](https://github.com/esp8266/Arduino/tree/2.7.4)

We strongly recommend to install the specied tagged versions of the libraries and Core SDK. Just hit the link above to see the versions.

## Build & Flash

You can build, flash and monitor the app via platform io as usual :)