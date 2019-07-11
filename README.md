# 360-Deg-Stepper
360 Degrees Arduino based stepper


 This Arduino example demonstrates bidirectional operation of a
 28BYJ-48, using a ULN2003 interface board to drive the stepper.
 The 28BYJ-48 motor is a 4-phase, 8-beat motor, geared down by
 a factor of 68. One bipolar winding is on motor pins 1 & 3 and
 the other on motor pins 2 & 4. The step angle is 5.625/64 and the
 operating Frequency is 100pps. Current draw is 92mA.

 The final use is to drive a rotary plate forto achieve 360 stepped pictures.
 At each step the plate stops for x ms to stabise internal movment, and take the picture.
 The system is controlled using an IR remote control.
 Information and setup is shown on a 16x2 LCD display.
 The final system should be able to drive 3 kind of camera :
  - any cam that can be controlled via a switch (like Canon 70D using 2.5" jack) - Switch is isolated.
  - an iPhone, using BT remote control.
  - a Canon via IR remote. --> Abandoned, due to 2s Delay because of timer use in Canon Remote control.

 The following values are adjustables :
  - number of picture per tour
  - rotating speed
  - active output (all can be set indivually) => new idea.

 The setup values are stored in EEprom. Each value is saved in the position corresponding to the Setup mode.
  - 1 for
  - 2 for
  - 3 for Output
