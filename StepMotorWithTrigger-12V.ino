// This Arduino example demonstrates bidirectional operation of a
// 28BYJ-48, using a ULN2003 interface board to drive the stepper.
// The 28BYJ-48 motor is a 4-phase, 8-beat motor, geared down by
// a factor of 68. One bipolar winding is on motor pins 1 & 3 and
// the other on motor pins 2 & 4. The step angle is 5.625/64 and the
// operating Frequency is 100pps. Current draw is 92mA.
//
// The final use is to drive a rotary plate forto achieve 360 stepped pictures.
// At each step the plate stops for x ms to stabise internal movment, and take the picture.
// The system is controlled using an IR remote control.
// Information and setup is shown on a 16x2 LCD display.
// The final system should be able to drive 3 kind of camera :
//  - any cam that can be controlled via a switch (like Canon 70D using 2.5" jack) - Switch is isolated.
//  - an iPhone, using BT remote control.
//  - a Canon via IR remote. --> Abandonned, due to 2s Delay because of timer use in Canon Remote control.
//
// The following values are adjustables :
//  - number of picture per tour
//  - rotating speed
//  - active output (all can be set indivually) => new idea.
//
// The setup values are stored in EEprom. Each value is saved in the position corresponding to the Setup mode.
//  - 1 for
//  - 2 for
//  - 3 for Output
//
////////////////////////////////////////////////
#include <IRremote.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Stepper.h>

#define STORSTEPSEL  0    // Number of pics Storage adress
#define STORDELAY    1    // Delay factor
#define STOROUTPUT   2    // Output Num

#define GO 0xFF02FD      // OK - > Go or Valid (Alias Save in setup mode)
#define PLUS 0xFF629D    // * UP -> Increment
#define MOINS 0xFFA857   // Down -> Decrement
#define RIGHT 0xFFC23D   // Right -> Enter Setup -> Enter 
#define LEFT 0xFF22DD    // Left
#define EXIT 0xFF42BD    // * -> exit setup.

/*
 Unused :
 1 : FF6897
 2 : FF9867
 3 : FFB04F
 4 : FF30CF
 5 : FF18E7
 6 : FF7A85
 7 : FF10EF
 8 : FF38C7
 9 : FF5AA5
 0 : FF4AB5
 * : FF42BD
 # : FF52AD
*/
int storAdress[4] = {10, STORSTEPSEL, STORDELAY, STOROUTPUT};    // Storage adress for each setup mode. First given in array not used (index O in modes)
boolean saved = false;

int stepsPerRevolution = 200;    // 200 steps motor

//declare variables for the motor pins
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);
int relayPin1 = 12;    // Red    - 28BYJ48 pin 5 (VCC)

// IR Wireless used values


// Menus vars
String setupModes[4] = {"Stepper Ready", "Setup steps #", "Setup delay", "Setup output" };
int maxModes = 3;

String triggerModes[3] = { "Switch", "BT", "Canon" };
int triggerPins[3] = {12, 12, 12}; // Sorties BT et IR a etablir
int triggerMode = 0;      // Valeur sera a stocker en EEPROM (ecrasera celle-ci predef initial)
int maxTriggModes = 2;    // @todo a calculer !

// Steps params
int stepsArraySize = 9;          // To clear the sizeof not working in function...
int numSteps[9] = {2, 4, 8, 10, 20, 25, 40, 50, 100}; //
int stepModeSel = 4;              // Default to 20 steps

// Delay at each step
int stepDelay = 100;
int xDelayMax = 20;            // Facteur Max (20 x 100 = 1s )
int stepVal = 10;              // Valeur courante

// int bouton1 = 2; // Bouton start sur la broche 2
int curPic = 0;

String msg = "";

// @Todo : add all LCD Pinout here.

// Liquid crystal pinout   Order : RS, E, D4, D3, D2, D1
LiquidCrystal lcd(13, 6, 5, 4, 3, 2);

// IR part
int RECV_PIN = 7;
IRrecv irrecv(RECV_PIN);
decode_results results;
long IRtempVal;

String curval;          // variable temporaire
int tmpVal;             // Idem for an int

int motorSpeed = 1200;  // variable to set stepper speed - Was 1200
int count = 0;          // count of substeps made
int count1 = 0;         // Count of steps made
int numberOfPics;       // Nombre de photos par tour (4 = 32 pics)
int countStep;          // Step approx (was 16)
int mode = 0;
int reglageDelay = 100;
//////////////////////////////////////////////////////////////////////////////
void setup() {
  lcd.begin(16, 2);
  lcd.print("Mode Stepper");
  lcd.setCursor(0, 1);
  lcd.print("Pic #");

  // Initialise Relay Pin
  relayPin1 = triggerPins[triggerMode];

  // Override default values with EEProms ones if exists !
  tmpVal = EEPROM.read(STORSTEPSEL);
  if (!(tmpVal == 0xFF)) stepModeSel = tmpVal;
  tmpVal = EEPROM.read(STORDELAY);    // Delay factor
  if (!(tmpVal == 0xFF)) stepVal = tmpVal;
  tmpVal = EEPROM.read(STOROUTPUT);    // Sel output... 0, 1, 2
  if (!(tmpVal == 0xFF)) triggerMode = tmpVal;

  // Compute initial values
  numberOfPics =  numSteps[stepModeSel];
  countStep = stepsPerRevolution / numberOfPics;
  
  // Set motor speed 
  myStepper.setSpeed(10);

  pinMode(relayPin1, OUTPUT);
  irrecv.enableIRIn();     // IR Start the receiver
  Serial.begin(9600);
}

//////////////////////////////////////////////////////////////////////////////
void loop() {

  if (irrecv.decode(&results)) {

    if (results.value != 0xFFFFFFFF) IRtempVal = results.value;
    //Serial.println(results.value, HEX);
    // In mode 0 the action are :
    // - start
    // - + or - adjust motor position
    // - enter setup mode
    if ( mode == 0) {
      switch (IRtempVal) {
        case GO :       //  (OK)
          showModeValue();    // To clear display (avoid leading remaining chars)
          processMotor();
          break;

        case MOINS :   // Ajuster Moins
          myStepper.step(-1);
          delay(reglageDelay);
          break;

        case PLUS :  // Ajuster Plus
          myStepper.step(1);
          delay(reglageDelay);
          break;

        case RIGHT :    // Entering setup mode.
          mode = 1;
          setMode();
          lcd.setCursor(0, 1);
          lcd.print(numberOfPics);
          break;

        default:
          // For debug : display IR val
          lcd.setCursor(5, 1);
          // lcd.println(results.value, HEX);
          break;
      }


      // if mode is > 1 then IR actions are on setup mode list (# steps / delay / etc...
      //
    } else if (mode > 0) {
      saved = false;
      // Setup modes
      switch (IRtempVal) {

        case RIGHT :  // Next mode
          setupModeNext('U');
          break;

        case LEFT :  // Previous mode
          setupModeNext('D');
          break;

        case PLUS :    // +  : increment la valeur de setup du mode courant.
          incrementModeValue();
          break;

        case MOINS :   // -
          decrementModeValue();
          break;

        // Save
        case GO :
          saveValue(); // Save current value in EEPROM
          break;

        case EXIT : // CH- = Exit setup
          resetA();
          break;

        default:
          lcd.setCursor(5, 1);
          // lcd.println(IRtempVal, HEX);
          break;
      }
      showModeValue();    // Affiche la valeur du mode choisi
    }
    irrecv.resume(); // Receive the next value
  }
  delay(100);

}
//////////////////////////////////////////////////////////////////////////////

// Set default Mode
void resetA () {
  mode = 0;
  lcd.clear();
  lcd.print("Mode Stepper");
}

void setMode() {
  lcd.clear();
  lcd.print(setupModes[mode]);
}

// Loop modes up or down
void setupModeNext(char upOrDown) {
  if (upOrDown == 'U') {
    if (mode == maxModes) {
      mode = 0;
    } else {
      mode++;
    }
  } else {
    if (mode == 0 ) {
      mode = maxModes;
    } else {
      mode--;
    }
  }
  setMode();
  // dbgDsplayMode();
}


/* Affichage des valeurs selectionnes pour le mode */
void showModeValue() {
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  switch (mode) {

    case 0 :
      curval = "Pic #";
      break;

    case 1 :
      curval = (String) numSteps[stepModeSel];
      break;

    case 2:
      curval = (String) (stepVal * stepDelay) + "ms";
      break;

    // Output mode setup
    case 3 :
      curval = triggerModes[triggerMode];
      break;

    default :
      curval = "No value yet";
      break;
  }
  lcd.print(curval);
  if (saved == true) lcd.print('*');
}

// Save current value in EEProm
void saveValue() {
  switch (mode) {
    // Setup step numbers
    case 1 :
      tmpVal = stepModeSel;
      break;
    case 2:
      tmpVal = stepVal;
      break;
    case 3:
      tmpVal = triggerMode;
      break;
    default :
      tmpVal = 99;
      break;
  }
  if (tmpVal != 99) {
    EEPROM.update(storAdress[mode], tmpVal);
    saved = true;
  }
}

void incrementModeValue() {
  // Incmente les valeurs du mode ajuste
  switch (mode) {
    // Setup step numbers
    case 1 :
      if (stepModeSel == stepsArraySize-1) { 
        stepModeSel = 0;
      } else {
        stepModeSel++;
      }
      break;

    case 2:
      if (stepVal < xDelayMax) stepVal++ ;
      break;

    // Reglage du mode de sortie (Switch / BT / IR Canon)
    case 3 :
      if (triggerMode == 2) {
        triggerMode = 0;
      } else {
        triggerMode++;
      }
      break;

    default :

      break;
  }

}

void decrementModeValue() {
  switch (mode) {
    // Setup step numbers
    case 1 :
      if (stepModeSel == 0) {
        stepModeSel = stepsArraySize-1;
      } else {
        stepModeSel--;
      }
      break;

    case 2:
      if (stepVal >= 1) stepVal-- ;
      break;

    case 3 :
      if (triggerMode == 0) {
        triggerMode = 2;
      } else {
        triggerMode--;
      }
      break;

    default :

      break;
  }

}

// Picture fn - Pauses half pic time defined - pic - then do the remaining half.
void takePic() {
  int picTime = stepDelay * stepVal;
  delay((picTime-200)/2);                    // waits for the time defined for the picture
  digitalWrite(relayPin1, HIGH);       // sets the LED on
  delay(200);
  digitalWrite(relayPin1, LOW);
  delay((picTime-200)/2);                    // waits for the time defined for the picture
}

// Debug mode only
void dbgDsplayMode() {
  lcd.setCursor(0, 1);
  lcd.print("Mode : " + mode  );
}

void processMotor() {
  numberOfPics =  numSteps[stepModeSel];
  countStep = stepsPerRevolution / numberOfPics;
  count1 = 0;
  while (count1 < numberOfPics) {
    takePic();
    myStepper.step(countStep);
    lcd.setCursor(5, 1);
    curPic = count1 + 1;
    lcd.print(curPic);
    lcd.print("/");
    lcd.print(numberOfPics);
    count1++;
  }
}  // end Process Motor

