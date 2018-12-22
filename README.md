# Solar Tracking System
Tiernan Cuesta - Ian Nielsen - Kevin Purcell //
A solar tracking application to increase output power over an extended period of time. This system requires two micro servo motors, and 4 photoresistors as the main devices. An MSP430F5529 launchpad kit is used to run the control interface. 
## Functionality
The four photoresistors that are arranged in a North, East, South, and West configuration are used to balance the system. In other words, difference in resistance of opposing light dependant resistors tell the servos to correct the error so that the photovoltaic cell maintains orthogonality with the sun. The maximum output power of the cell is achieved over longer periods of time, given satisfactory environmental conditions. Typically, the range of motion of the system is 0-180 azimuth and altitude angle given no mechanical restriction.

## Valid Inputs/Outputs
The system features 6 inputs and 3 outputs. Five of the six inputs are ADC ports 6.0 - 6.4 where four of these converters record the voltage across the photoresistors which is dependant it's resistance. These values are then compared to the coorsponding values and a P-control algorithm handles the data to tell the motors what to do. The 5th ADC is used to record the voltage output photovoltaic cell for data acquisition. The last input is a button, button P1.1, that callibrates the system by taking the current error and biasing the error with the mentioned callibration value.
## Description of code
To achieve desired functionality, first of all, a sequence of ADC input channels was implemented along with a timer setup for a 50Hz PWM signal. The ADC's task was for system error logging to impose algorithms on to correct these errors in the system. Simple P-control was sufficient enough in order to reduce overall power consumption of the tracking system. A button interrupt was implemented to support the calibration protocol. A UART buffer interrupt enables data aqcuisition of photovoltaic cell status.
