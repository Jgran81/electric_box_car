#include <Arduino.h>
#include <atomic>
#include <ResponsiveAnalogRead.h>
#include "mario.h"  // Include the Mario theme notes
//#include "super_mario.h"
//#include "doom.h"
//#include "ice_ice_baby.h"
//#include "imagine_dragons_enemy.h"
//#include "maroon5_memories.h"
//#include "tetris.h"
//#include "star_wars.h"

// Uncomment the line below to enable logging
//#define DEBUG

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
std::atomic<gears> GearState{gears::forward}; 
gears LastGearState{gears::forward};

// Buzzer
constexpr int BUZZER_PIN{25};  // Your buzzer pin
constexpr int BUZZER_PWM_CHANNEL{0};  // Buzzer PWM channel
constexpr int beepInterval{500}; // 500ms on/off like a truck
bool reverseActive{true}; // Set to true when in reverse
bool buzzerOn{false};
unsigned long lastBeepTime{0};
const size_t MARIO_LEN = sizeof(marioSong) / sizeof(marioSong[0]);


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

// Task 0: Play music on core 0
void MusicCore(void *pvParameters);
// Task 1: Normal tasks on core 1
void NormalTasks(void *pvParameters);

void setup() {
	Serial.begin(9600);

  xTaskCreatePinnedToCore(MusicCore, "MusicCore", 4096, NULL, 1, NULL, 0); // Core 0
  xTaskCreatePinnedToCore(NormalTasks, "NormalTasks", 4096, NULL, 1, NULL, 1); // Core 1
}

void loop() {
  // Nothing here, all tasks are handled in their respective functions
}
/// --<<<END OF LOOP>>--

// Core 0 task to play music
void MusicCore(void *pvParameters) {
  // This task runs on core 0
  ledcAttachPin(BUZZER_PIN, BUZZER_PWM_CHANNEL);
  pinMode(HORN_PIN, INPUT_PULLUP); // HORN: Enable internal pull-up resistor
  for(;;) {
    // Read horn switch, LOW == no sound, HIGH == Sound
    hornState = digitalRead(HORN_PIN) == LOW;

    if (GearState.load() == gears::revers) {
      updateReversingBeep();
    } else if (hornState) {
      playMusic();
    } else {
      ledcWrite(BUZZER_PWM_CHANNEL, 0); 
    }
    #ifdef DEBUG
      Serial.print(" | HORN Active: ");
      Serial.print(hornState ? "YES" : "NO");
    #endif
    // Optionally add a small delay to yield to other tasks
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// Core 1 task for normal operations
void NormalTasks(void *pvParameters) {
  // Motor controller
  pinMode(L_EN, OUTPUT);
	pinMode(R_EN, OUTPUT);
	pinMode(L_PWM, OUTPUT);
	pinMode(R_PWM, OUTPUT);

	pinMode(GAS_PEDAL, INPUT);

  pinMode(THROTTLE_LIMIT_PIN, INPUT);
  
	digitalWrite(L_EN, HIGH);
	digitalWrite(R_EN, HIGH);

  // Setting the resolution for input pins
  ThrottleAnalogRead.setAnalogResolution(resolution);
  
  for(;;) {
    // This task runs on core 1

    // Throttle limit input potentiometer, from 10 to 100%
    throttleLimit = map(analogRead(THROTTLE_LIMIT_PIN), thottleMinInput, throttleMaxInput, throttleMinOutPut, throttleMaxOutPut);

    // This needs to be atomic to avoid race conditions with MusicCore
    GearState.store(digitalRead(GEAR_SWITCH_PIN) == HIGH ? gears::forward : gears::revers);

    if (GearState.load() != LastGearState) {
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

        vTaskDelay(100);
      }

    // Full stop
    analogWrite(R_PWM, 0);
    analogWrite(L_PWM, 0);
    currentPWM = 0;

    // Save gear state for next loop
    LastGearState = GearState.load();
  }

  // Read and map servo values
  if (GearState.load() == gears::revers) {
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

  if (GearState.load() == gears::revers) { 
    analogWrite(R_PWM, currentPWM);  
    analogWrite(L_PWM, 0);  // Only driver forward
    isItInRevers = true;
  } else {
    analogWrite(L_PWM, currentPWM);  
    analogWrite(R_PWM, 0);  // Only revers
    isItInRevers = false;
  }

  #ifdef DEBUG
    Serial.print(" | PwmValue: ");
    Serial.print(currentPWM);

    Serial.print(" | RawValue: ");
    Serial.print(analogRead(GAS_PEDAL));

    Serial.print(" | Reverse Active: ");
    Serial.print(GearState.load() == gears::revers ? "YES" : "NO");

    Serial.print("      \r");
  #endif

    // Optionally add a small delay to yield to other tasks
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

}


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
    } else {
      ledcWriteNote(BUZZER_PWM_CHANNEL, NOTE_E, 5);
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

void playMusic() {
  for (size_t i = 0; i < MARIO_LEN; ++i) {
    const Note &n = marioSong[i];
      if (GearState.load() == gears::revers) {
        break;
      }
      if (n.note == REST) {
        // silence for duration
        ledcWrite(BUZZER_PWM_CHANNEL, 0);
      } else {
        ledcWriteNote(BUZZER_PWM_CHANNEL, static_cast<note_t>(n.note), n.octave);
      }
      const uint32_t dur_ms = 1100 / n.duration;
      vTaskDelay(dur_ms / portTICK_PERIOD_MS);
      // ensure silence between notes without adding extra time
      ledcWrite(BUZZER_PWM_CHANNEL, 0);
  }
}