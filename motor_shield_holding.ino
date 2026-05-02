// =============================================================
// Nema 17 (17HS19-2004S1) + FUYU FPB30 Belt Drive Linear Stage
// Arduino Motor Shield (L298) · Stepper.h
// Homing + Move by mm · 2000mm stroke · Position Hold
// Memory optimized · Silent PWM (31 kHz)
// =============================================================

#include <Stepper.h>

// ── Motor parameters ───────────────────────────────────────
const int   STEPS_PER_REV  = 200;
const float STEP_ANGLE_DEG = 1.8;

// ── Linear stage conversion ────────────────────────────────
const float MM_PER_REV  = 60.0;
const float MM_PER_STEP = MM_PER_REV / STEPS_PER_REV;

// ── Travel limits ──────────────────────────────────────────
const float MAX_TRAVEL_MM = 2000.0;
const float MIN_TRAVEL_MM = 0.0;

// ── Homing batch size ──────────────────────────────────────
const int SEEK_BATCH = 10;
const int FINE_BATCH = 5;

// ── Shield pins ────────────────────────────────────────────
const int CHA_DIR = 12;
const int CHB_DIR = 13;
const int CHA_PWM =  3;
const int CHB_PWM = 11;
const int CHA_BRK =  9;
const int CHB_BRK =  8;

// ── End switch pin ─────────────────────────────────────────
const int HOME_SWITCH_PIN = 2;

// ── Stepper object ─────────────────────────────────────────
Stepper myStepper(STEPS_PER_REV, CHA_DIR, CHB_DIR);

// ── User settings ──────────────────────────────────────────
float targetRpm      = 60.0;
float targetDistance  = 10.0;
bool  clockwise      = true;
float homeRpm        = 60.0;
float currentPosMm   = 0.0;
bool  isHomed        = false;
bool  holdEnabled    = false;
int   holdPwm        = 255;

// ── Phase tracking ─────────────────────────────────────────
const int PHASE_TABLE[4][2] = {
  {HIGH, HIGH},
  {LOW,  HIGH},
  {LOW,  LOW},
  {HIGH, LOW}
};
int currentPhase = 0;

// ── Helpers ────────────────────────────────────────────────

int mmToSteps(float mm) {
  return (int)((mm / MM_PER_STEP) + 0.5);
}

void shieldEnable() {
  digitalWrite(CHA_BRK, LOW);
  digitalWrite(CHB_BRK, LOW);
  analogWrite(CHA_PWM, 180);
  analogWrite(CHB_PWM, 180);
}

void shieldDisable() {
  analogWrite(CHA_PWM, 0);
  analogWrite(CHB_PWM, 0);
  digitalWrite(CHA_BRK, HIGH);
  digitalWrite(CHB_BRK, HIGH);
}

void holdPosition() {
  digitalWrite(CHA_BRK, LOW);
  digitalWrite(CHB_BRK, LOW);
  digitalWrite(CHA_DIR, PHASE_TABLE[currentPhase][0]);
  digitalWrite(CHB_DIR, PHASE_TABLE[currentPhase][1]);
  analogWrite(CHA_PWM, holdPwm);
  analogWrite(CHB_PWM, holdPwm);
}

void trackPhase(int steps) {
  int phaseChange = steps % 4;
  if (phaseChange < 0) phaseChange += 4;
  currentPhase = (currentPhase + phaseChange) & 0x03;
}

bool switchPressed() {
  return digitalRead(HOME_SWITCH_PIN) == LOW;
}

bool withinLimits(float targetMm) {
  if (targetMm < MIN_TRAVEL_MM || targetMm > MAX_TRAVEL_MM) {
    Serial.print(F("ERR: "));
    Serial.print(targetMm, 1);
    Serial.print(F(" mm out of range 0-"));
    Serial.println(MAX_TRAVEL_MM, 0);
    return false;
  }
  return true;
}

// ── Homing routine ─────────────────────────────────────────

bool homeMotor() {
  Serial.println(F("Homing..."));
  holdEnabled = false;

  myStepper.setSpeed(homeRpm);
  shieldEnable();

  if (switchPressed()) {
    int backoffSteps = 0;
    while (switchPressed() && backoffSteps < STEPS_PER_REV) {
      myStepper.step(SEEK_BATCH);
      backoffSteps += SEEK_BATCH;
    }
    if (switchPressed()) {
      Serial.println(F("ERR: Backoff failed"));
      shieldDisable();
      return false;
    }
    delay(200);
  }

  int seekSteps = 0;
  int maxSeek = mmToSteps(MAX_TRAVEL_MM + 100);

  while (!switchPressed() && seekSteps < maxSeek) {
    myStepper.step(-SEEK_BATCH);
    seekSteps += SEEK_BATCH;
  }

  if (!switchPressed()) {
    Serial.println(F("ERR: Switch not found"));
    shieldDisable();
    return false;
  }

  myStepper.setSpeed(homeRpm / 2);
  myStepper.step(50);
  delay(100);

  while (!switchPressed()) {
    myStepper.step(-FINE_BATCH);
  }

  currentPosMm = 0.0;
  currentPhase = 0;
  isHomed = true;
  Serial.println(F("Homed: 0.00 mm"));

  delay(200);
  shieldDisable();
  return true;
}

// ── Move to absolute position ──────────────────────────────

void moveToMm(float targetMm) {
  if (!isHomed) { Serial.println(F("ERR: Home first (H)")); return; }
  if (!withinLimits(targetMm)) return;

  float distanceMm = targetMm - currentPosMm;
  int stepsToMove = mmToSteps(abs(distanceMm));
  if (stepsToMove == 0) { Serial.println(F("At target")); return; }
  if (distanceMm < 0) stepsToMove = -stepsToMove;

  myStepper.setSpeed(targetRpm);
  shieldEnable();

  Serial.print(currentPosMm, 1);
  Serial.print(F(" -> "));
  Serial.print(targetMm, 1);
  Serial.println(F(" mm"));

  myStepper.step(stepsToMove);
  trackPhase(stepsToMove);
  currentPosMm = targetMm;

  delay(200);

  if (holdEnabled) {
    holdPosition();
    Serial.println(F("Done. Holding."));
  } else {
    shieldDisable();
    Serial.println(F("Done."));
  }
}

// ── Move relative distance ─────────────────────────────────

void moveByMm(float distanceMm, bool forward) {
  if (!isHomed) { Serial.println(F("ERR: Home first (H)")); return; }
  if (!forward) distanceMm = -distanceMm;

  float newPos = currentPosMm + distanceMm;
  if (!withinLimits(newPos)) return;

  int stepsToMove = mmToSteps(abs(distanceMm));
  if (distanceMm < 0) stepsToMove = -stepsToMove;

  myStepper.setSpeed(targetRpm);
  shieldEnable();

  myStepper.step(stepsToMove);
  trackPhase(stepsToMove);
  currentPosMm = newPos;

  delay(200);

  if (holdEnabled) {
    holdPosition();
  } else {
    shieldDisable();
  }

  Serial.print(F("At "));
  Serial.print(currentPosMm, 1);
  Serial.println(holdEnabled ? F(" mm (hold)") : F(" mm"));
}

// ── Setup ──────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  // Set Timer2 PWM frequency to ~31 kHz (pins 3 and 11)
  // Eliminates audible buzzing at reduced hold power
  TCCR2B = (TCCR2B & 0b11111000) | 0x01;

  pinMode(CHA_PWM, OUTPUT);
  pinMode(CHB_PWM, OUTPUT);
  pinMode(CHA_BRK, OUTPUT);
  pinMode(CHB_BRK, OUTPUT);
  pinMode(HOME_SWITCH_PIN, INPUT_PULLUP);

  shieldDisable();

  Serial.println(F("=== FPB30 Stage ==="));
  Serial.println(F("H=Home G=Go M<mm>=MoveTo"));
  Serial.println(F("L<mm>=Dist R<rpm>=Speed"));
  Serial.println(F("D<0|1>=Dir K=Hold W<0-255>=Str"));
  Serial.println(F("C=Cont S=Stop ?=Status"));
  Serial.println(F("Home first! Send H"));
}

// ── Loop ───────────────────────────────────────────────────

bool continuousMode = false;

void loop() {
  if (continuousMode) {
    float nextPos = currentPosMm + (clockwise ? MM_PER_REV : -MM_PER_REV);

    if (nextPos > MAX_TRAVEL_MM || nextPos < MIN_TRAVEL_MM) {
      continuousMode = false;
      shieldDisable();
      Serial.println(F("Limit reached"));
    } else {
      myStepper.setSpeed(targetRpm);
      shieldEnable();
      int steps = clockwise ? STEPS_PER_REV : -STEPS_PER_REV;
      myStepper.step(steps);
      trackPhase(steps);
      currentPosMm = nextPos;
    }

    if (switchPressed()) {
      continuousMode = false;
      shieldDisable();
      Serial.println(F("Switch hit"));
    }
  }

  if (Serial.available()) {
    char cmd = toupper(Serial.read());

    switch (cmd) {
      case 'H':
        continuousMode = false;
        homeMotor();
        break;

      case 'G':
        continuousMode = false;
        moveByMm(targetDistance, clockwise);
        break;

      case 'M': {
        float m = Serial.parseFloat();
        continuousMode = false;
        moveToMm(m);
        break;
      }

      case 'L': {
        float d = Serial.parseFloat();
        if (d > 0) targetDistance = d;
        Serial.print(F("Dist: "));
        Serial.print(targetDistance, 1);
        Serial.println(F(" mm"));
        break;
      }

      case 'R': {
        float r = Serial.parseFloat();
        if (r > 0) targetRpm = r;
        Serial.print(F("RPM: "));
        Serial.println(targetRpm);
        break;
      }

      case 'D': {
        int d = Serial.parseInt();
        clockwise = (d == 1);
        Serial.println(clockwise ? F("Dir: Fwd") : F("Dir: Back"));
        break;
      }

      case 'K':
        holdEnabled = !holdEnabled;
        if (holdEnabled) {
          holdPosition();
          Serial.print(F("Hold ON ("));
          Serial.print(holdPwm);
          Serial.println(F("/255)"));
        } else {
          shieldDisable();
          Serial.println(F("Hold OFF"));
        }
        break;

      case 'W': {
        int w = Serial.parseInt();
        if (w >= 0 && w <= 255) holdPwm = w;
        Serial.print(F("Hold str: "));
        Serial.println(holdPwm);
        if (holdEnabled) holdPosition();
        break;
      }

      case 'C':
        if (!isHomed) {
          Serial.println(F("ERR: Home first (H)"));
        } else {
          continuousMode = true;
          holdEnabled = false;
          Serial.println(F("Cont ON"));
        }
        break;

      case 'S':
        continuousMode = false;
        holdEnabled = false;
        shieldDisable();
        Serial.println(F("Stopped"));
        break;

      case '?':
        Serial.print(F("Pos: "));
        Serial.print(currentPosMm, 1);
        Serial.print(F("/"));
        Serial.print(MAX_TRAVEL_MM, 0);
        Serial.print(F("mm H:"));
        Serial.print(isHomed ? F("Y") : F("N"));
        Serial.print(F(" K:"));
        Serial.println(holdEnabled ? F("Y") : F("N"));
        break;
    }
  }
}