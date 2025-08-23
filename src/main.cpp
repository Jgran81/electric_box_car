#include <Arduino.h>
#include <ResponsiveAnalogRead.h>
//#include "super_mario.h"
//#include "doom.h"
//#include "ice_ice_baby.h"
//#include "imagine_dragons_enemy.h"
//#include "maroon5_memories.h"
//#include "tetris.h"
//#include "star_wars.h"

// Uncomment the line below to enable logging
#define DEBUG

// Throttle Limiter
constexpr int THROTTLE_LIMIT_PIN{26};
constexpr int throttleMaxInput{4096};
constexpr int thottleMinInput{0};
constexpr int throttleMaxOutPut{99};
constexpr int throttleMinOutPut{10};
constexpr int reverseThrottleMax{30};  // Max revers speed
int throttleLimit{10};

// Horn
constexpr int HORN_PIN{16};
bool hornState{false};

// Gear switch
constexpr int GEAR_SWITCH_PIN{15}; // Gear selector switch
bool isItInRevers{false};  // temp variable for testing if we are reversing
constexpr int gearSwitchTransition{5}; // how fast will motor slow down in gear change?

enum class gears {forward, revers};
gears GearState{gears::forward};
gears LastGearState{gears::forward};

// Buzzer
constexpr int BUZZER_PIN{25};  // Your buzzer pin
constexpr int BUZZER_PWM_CHANNEL{0};  // Buzzer PWM channel
constexpr int beepInterval{500}; // 500ms on/off like a truck
bool reverseActive{true}; // Set to true when in reverse
bool buzzerOn{false};
unsigned long lastBeepTime{0};


// Throttle
constexpr int GAS_PEDAL{34};
constexpr int softStartStep{5};  // Hur snabbt PWM ökar, öka för snabbare förändring
constexpr int softStartDelay{10}; // Fördröjning mellan varje steg i ms
int currentPWM{0};  // Nuvarande PWM-värde
int targetPWM{0};
/// Throttle cables
/// Red: +
/// Yellow: -
/// Green: signal
constexpr int throttleMax{100};	// Hur mycket gas i % kan Nils ge, minsta för att sänka hastigheten på bilen.
constexpr int minInputThrottle{850};
constexpr int maxInputThrottle{2930};
constexpr int throttleDeadSpace{3};
constexpr int L_EN{4}; 
constexpr int R_EN{5}; 
constexpr int L_PWM{18}; 
constexpr int R_PWM{19}; 

// Throttle filter
constexpr float snapMultiplier{0.01f};
constexpr int resolution{4096};

// Constructs
ResponsiveAnalogRead ThrottleAnalogRead(GAS_PEDAL, true, snapMultiplier);

void softAccelerateOrDecelerate(int& currentPWM, int& targetPWM);
int readAnalogThrottleValue(int throttleLevel);
void updateReversingBeep();
void updateHornBeep();
void playGearChangeWarning();

void setup() {
	// Motor controller
  pinMode(L_EN, OUTPUT);
	pinMode(R_EN, OUTPUT);
	pinMode(L_PWM, OUTPUT);
	pinMode(R_PWM, OUTPUT);

	pinMode(GAS_PEDAL, INPUT);

  pinMode(GEAR_SWITCH_PIN, INPUT_PULLUP); // GEAR: Enable internal pull-up resistor
  pinMode(HORN_PIN, INPUT_PULLUP); // HORN: Enable internal pull-up resistor

  //pinMode(BUZZER_PIN, OUTPUT);
  ledcAttachPin(BUZZER_PIN, BUZZER_PWM_CHANNEL);

  pinMode(THROTTLE_LIMIT_PIN, INPUT);
  
	digitalWrite(L_EN, HIGH);
	digitalWrite(R_EN, HIGH);

  // Setting the resolution for input pins
  ThrottleAnalogRead.setAnalogResolution(resolution);

	Serial.begin(9600);
}

void loop() {
  // Throttle limit input potentiometer, from 10 to 100%
  throttleLimit = map(analogRead(THROTTLE_LIMIT_PIN), thottleMinInput, throttleMaxInput, throttleMinOutPut, throttleMaxOutPut);
  
  // Read gear switch, LOW == Revers, HIGH == Forward
  //GearIsInReverse = digitalRead(GEAR_SWITCH_PIN) == LOW;
  //GearIsInDrive = !GearIsInReverse;   // ==TRUE if going forward else its FALSE
  //digitalRead(GEAR_SWITCH_PIN) == LOW ? gearstate::Reverse : gearstate::Forward;

  GearState = digitalRead(GEAR_SWITCH_PIN) == HIGH ? gears::forward : gears::revers;

  // Read current gear state
  //bool currentGearState = digitalRead(GEAR_SWITCH_PIN) == LOW;  // true = reverse

  //if (currentGearState != lastGearState) {
  if (GearState != LastGearState) {
    // Gear changed! Slow down before switching direction
    while (currentPWM > 15) {
      currentPWM -= 10;
      if (currentPWM < 0) currentPWM = 0;

      if (GearState == gears::revers) { 
        // Switching into reverse
        analogWrite(L_PWM, currentPWM);
        analogWrite(R_PWM, 0);
      } else {
        // Switching into drive
        analogWrite(R_PWM, currentPWM);
        analogWrite(L_PWM, 0);
      }

      delay(100);
    }

    // Full stop
    analogWrite(R_PWM, 0);
    analogWrite(L_PWM, 0);
    currentPWM = 0;

    // Save gear state for next loop
    LastGearState = GearState;
  }

  // Read and map servo values
  if (GearState == gears::revers) {
    targetPWM = readAnalogThrottleValue(reverseThrottleMax);
  } else {
    targetPWM = readAnalogThrottleValue(throttleLimit);
  }

  // Mjukstart och broms, översätter inkommande gas signal och översätter till ett mjukare gaspådrag
  softAccelerateOrDecelerate(currentPWM, targetPWM);

  // Deadspace uträkning
  if (currentPWM < throttleDeadSpace) {
    currentPWM = 0;
  }

  // Read horn switch, LOW == no sound, HIGH == Sound
  hornState = digitalRead(HORN_PIN) == LOW;

  if (GearState == gears::revers) { 
    analogWrite(R_PWM, currentPWM);  
    analogWrite(L_PWM, 0);  // Only driver forward
    isItInRevers = true;
  } else {
    analogWrite(L_PWM, currentPWM);  
    analogWrite(R_PWM, 0);  // Only revers
    isItInRevers = false;
  }

  if (GearState == gears::revers) { 
    updateReversingBeep();
  } else if (hornState) {
    updateHornBeep();
  } else {
    ledcWrite(BUZZER_PWM_CHANNEL, 0); 
  }


  #ifdef DEBUG
    Serial.print(" | PwmValue: ");
    Serial.print(currentPWM);

    Serial.print(" | RawValue: ");
    Serial.print(analogRead(GAS_PEDAL));

    Serial.print(" | Reverse Active: ");
    Serial.print(GearState == gears::revers ? "YES" : "NO");

    Serial.print(" | HORN Active: ");
    Serial.print(hornState ? "YES" : "NO");

    //Serial.print(" | Revers?: ");
    //Serial.print(isItInRevers ? "YES" : "NO");

    Serial.print("      \r");
  #endif

  //delay(softStartDelay);  // Vänta lite mellan varje steg för mjuk effektökning
}
/// --<<<END OF LOOP>>--


/**
 * Smoothly adjusts currentPWM toward targetPWM.
 * Modifies currentPWM directly (passed by reference).
 *
 * @param currentPWM - reference to the current PWM value
 */
void softAccelerateOrDecelerate(int& currentPWM, int& targetPWM) {
  // Mjukstart: Öka PWM gradvis mot det önskade värdet
  if (currentPWM < targetPWM) {
    currentPWM += softStartStep;
    if (currentPWM > targetPWM) {
      currentPWM = targetPWM;  // Se till att vi inte går över
    }
  } else if (currentPWM > targetPWM) {
    currentPWM -= softStartStep;
    if (currentPWM < targetPWM) {
      currentPWM = targetPWM;  // Se till att vi inte går under
    }
  }
}


int readAnalogThrottleValue(int throttleLevel) {

  ThrottleAnalogRead.update();

  return map(ThrottleAnalogRead.getValue(), minInputThrottle, maxInputThrottle, 0, ((255*throttleLevel)/100)); // Justera PWM
}


void updateReversingBeep() {

  unsigned long currentMillis = millis();
  if (currentMillis - lastBeepTime >= beepInterval) {
    lastBeepTime = currentMillis;  // Reset timer

    if (buzzerOn) {
      ledcWrite(BUZZER_PWM_CHANNEL, 0);  // Ingen ton
      //noTone(BUZZER_PIN);  // Turn OFF
    } else {
      ledcWriteNote(BUZZER_PWM_CHANNEL, NOTE_E, 5);
      //tone(BUZZER_PIN, NOTE_E5);  // Revers Tone
    }

    buzzerOn = !buzzerOn;  // Toggle state
  }
}


void updateHornBeep() {

  ledcWriteNote(BUZZER_PWM_CHANNEL, NOTE_B, 4);
}


// Play music from included files, switch in header.
/* void playGearChangeWarning() {
  int size = sizeof(durations) / sizeof(int);

  for (int note = 0; note < size; note++) {
    //to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int duration = 1000 / durations[note];
    tone(BUZZER_PIN, melody[note], duration);

    //to distinguish the notes, set a minimum time between them.
    //the note's duration + 30% seems to work well:
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    
    //stop the tone playing:
    noTone(BUZZER_PIN);
  }
} */