# Temperature_and_brightness_controller
Lab work for data acquisition system course with the objective to open and close blindfolds depending on temperature and brightness detected through an Explorer 16 with a PIC24FJ128GA010.

#
INTRODUCTION

The implementation of a simple climate control system in this laboratory uses the Explorer16 microcontrolled platform, which uses a PIC24FJ128GA010. This system consists of controlling a system of blinds, which go up or down depending on the values read by sensors.
The conversion of real-world measurements to digital data from a data acquisition system is done using sensors and transducers, of which a thermistor and an LDR were used for this work. The digital data acquired by these sensors is then sent to the Explorer16 microcontroller platform.

#
DESCRIPTION OF THE SYSTEM

As mentioned above, the system developed consists of a thermistor and an LDR both connected to a resistance of 5.1 KOhms each, and an Explorer16 platform microcontrolled by a PIC24FJ128GA010. Based on the data collected by the sensors, the microcontroller decides whether to raise or lower hypothetical blinds.
These blinds are represented by two LEDs from the Explorer16 platform (A0 and A1) and two buttons (RD6 and RD13). If the A0 LED is on, the blinds are going up, if the A1 LED is on, the blinds are going down and if both LEDs are off, the blinds are stopped. As for the RD6 and RD13 buttons, these indicate whether the blinds have closed or opened completely when pressed.

#
Last time the code was tested -> 15/05/2022
