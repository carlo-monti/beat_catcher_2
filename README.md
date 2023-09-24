# Beat Catcher 2.0
A portable beat-tracking system.

## About
**Beat Catcher** is a beat-tracking device for synchronizing electronic systems (drum machines, sequencer, arpeggiator, ...) with a human performer playing an acoustic drum. Using one piezoelectric sensor on the kick and another on the snare, Beat Catcher detects the tempo currently held by the musician and sends out a Midi Clock signal that keeps the electronic instrument in sync with it. 

## The algorithm
Beat Catcher 2.0 is an implementation of the B-Keeper beat-tracking algorithm (*Robertson, Andrew, and Mark D. Plumbley. “Synchronizing Sequencing Software to a Live Drummer.” Computer
Music Journal, vol. 37, no. 2, 2013*). An updated version of B-Keeper algorithm is now included in Ableton Live 11, a de facto standard for electro/acoustic performances. Beat Catcher 2.0 represents an alternative method that is cheap and portable: a DAWless way of doing syncronization.

## Hardware
The whole project is based on an ESP32 MCU and uses other peripherals such as an OLED Display SSD1306, an Encoder KY-040 and various electronic components (piezo sensors, leds...). This is the schematic of the circuit:
![Circuit image](img/circuito.png?raw=true "Circuit")

## Code

| Supported Targets | ESP32 | ESP32-S3 |
| ----------------- | ----- | -------- |

To compile the code just do what suggested in the ESP-IDF docs:

1. Install the ESP-IDF framework with plugin in VSC or Eclipse (v5.1.1)
2. Create a new bare project with template sample_project
3. Copy the `main` folder content to the `main` folder of the project
4. Add the `components` folder to your project
5. If you are using VSC, set include path as specified [here](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/C_CPP_CONFIGURATION.md). You likely have to set `.vscode/c_cpp_properties.json` to:
   
   ```json
   {
    "configurations": [
      {
        "name": "ESP-IDF",
        "cStandard": "c11",
        "cppStandard": "c++17",
        "compileCommands": "${workspaceFolder}/build/compile_commands.json"
      }
    ],
    "version": 4
   }
     ```
7. Run SDK Configuration Editor for your needs (be sure to check for FreeRTOS tick frequency of 1000Hz)
8. Build

## Documentation

Doxygen docs can be found at: https://carlo-monti.github.io/beat_catcher_2/

## How to use it in practice

The user interface of the system looks like this:
![User interface](img/user_interface.png?raw=true "User interface")
Once the system is powered on, the user can start the sequence by tapping on the Tap button four times at the correct tempo. The MIDI Clock sequence will start and the system will keep up with the drummer playing. To stop the sequence simply press the Tap button once. Pressing the Menu button will enter the SETTINGS mode in which the user can set various parameters.

Here is an usage example:
- Connect the two piezos to kick and drum.
- Power up the system (once started it will show TAP on screen).
- Press MENU button. You are now ready to set the gain of the piezos.
  - Hit the kick as hard as you can
  - If the led blinks, lower the gain. Otherwise increase it.
  - Set the gain just below the clipping (the led should not blink).
  - Repeat the same for snare.
- Now press the encoder repeatedly and select the parameter you want to set. At the beginning of the menu you will find global parameters:
  - SYNC responsiveness.
  - TEMPO responsiveness.
- If you go ahead clicking the encoder, you will find parameters for tweaking hit detection for kick and snare (you can monitor the settings result by looking at the corresponding led: if it blinks, it means that a hit has been detected):
  - THRESHOLD indicates how hard you have to hit the kick (or snare) to detect an onset.
  - RETRIGGER GATE indicates the time that the system will wait to retrig a new onset.
  - FILTER indicates the smoothin of the incoming signal (you better leave it as it is).
  - LENGTH indicates the duration of the attach of the note (again, you better leave it as it is).
- Click the encoder until you see "SAVE VALUES". If you choose Yes, the system will store your settings for the next time. Otherwise, the settings will be resetted to default every time you boot up the system.
- Press MENU button again. You are now in TAP mode.
- Hit the Tap pad four times at the correct tempo: the system will start transmitting the MIDI_CLOCK. The red leds will blink depending on the position in the 4/4 bar and you can hear an audio click through the headphones.
- Now you can start playing and the system will hopefully follow you (you have to start in sync with the system). If you press the MENU button, the system will stop. If you press the TAP button four times again, the system will start a new sequence with the new tempo.
- Be careful: if you stop tapping until the fourth tap, the system will reset (after a timeout period). So, if you want to tap again a new tempo, you have to wait until TAP is displayed on the screen.
- 
