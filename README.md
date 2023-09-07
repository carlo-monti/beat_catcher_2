# Beat Catcher 2.0
A portable beat-tracking system.

## About
**Beat Catcher 2.0** is a beat-tracking device for synchronizing electronic systems (drum machines, sequencer, arpeggiator, ...) with an acoustic drum played by a human performer. Using one piezoelectric sensor on the Kick and another on the Snare, Beat Catcher 2.0 detects the tempo currently held by the musician and sends out a Midi Clock signal that is in sync with it. 

## The algorithm
Beat Catcher 2.0 is an implementation of the B-Keeper beat-tracking algorithm (*Robertson, Andrew, and Mark D. Plumbley. “Synchronizing Sequencing Software to a Live Drummer.” Computer
Music Journal, vol. 37, no. 2, 2013*). An updated version of B-Keeper algorithm is now included in Ableton Live 11, a de facto standard for electro/acoustic performances. Beat Catcher 2.0 represents an alternative method that is cheap and portable: a DAWless way of doing syncronization.

## Hardware
The whole project is based on an ESP32 MCU and uses other peripherals such as an OLED Display SSD1306, an Encoder KY-040 and various electronic components (piezo sensors, leds...). This is the schematic of the circuit:
![Circuit image](img/circuito.png?raw=true "Circuit")

## How to use
The user interface of the system looks like this:
![User interface](img/user_interface.png?raw=true "User interface")
Once the system is powered on, the user can start the sequence by tapping on the Tap button four times at the correct tempo. The MIDI Clock sequence will start and the system will keep up with the drummer playing. To stop the sequence simply press the Tap button once. Pressing the Menu button will enter the SETTINGS mode in which the user can set various parameters.

## Code

The code has been developed with the ESP-IDF framework. After the framework is installed (in VSC or Eclipse), just clone the whole code into a new project and set the build target.

## Documentation
Doxygen docs can be found at: https://carlo-monti.github.io/beat_catcher_2/
