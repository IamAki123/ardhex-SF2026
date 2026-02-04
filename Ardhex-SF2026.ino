#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// pins
const int hallSensorPin = 2; // interrupt pin for Hall Effect sensor
const int buttonPin = 4;     // start test button
const int servoPin = 9;      // servo for rubber brake/magnetic arm
const int motorRelayPin = 7; // to turn the motor on/off

// variables
volatile int pulseCount = 0;
unsigned long startTime, stopTime;
bool isBraking = false;
float rpm = 0;
const int targetRPM = 1000; // set your desired testing speed here

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo brakeServo;

// interrupt function to count rotations
void countPulse() {
  pulseCount++;
}

void setup() {
  pinMode(hallSensorPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(motorRelayPin, OUTPUT);
  
  attachInterrupt(digitalPinToInterrupt(hallSensorPin), countPulse, RISING);
  
  brakeServo.attach(servoPin);
  brakeServo.write(0); // initial position (Brake OFF)
  
  lcd.init();
  lcd.backlight();
  lcd.print("Ready to Test");
}

void loop() {
  // 1. wait for button press
  if (digitalRead(buttonPin) == LOW && !isBraking) {
    runTest();
  }
}

void runTest() {
  lcd.clear();
  lcd.print("Spinning Up...");
  digitalWrite(motorRelayPin, HIGH); // start motor

  // 2. wait for target RPM
  while (calculateRPM() < targetRPM) {
    lcd.setCursor(0, 1);
    lcd.print("RPM: ");
    lcd.print(rpm);
    delay(200);
  }

  // 3. Trigger Brake
  lcd.clear();
  lcd.print("BRAKING!");
  digitalWrite(motorRelayPin, LOW); // cut power to motor
  
  // apply mechanical force via servo
  // adjust '90' to the angle that applies your desired pressure
  brakeServo.write(90); 
  
  startTime = millis();
  isBraking = true;

  // 4. wait for stop
  while (isBraking) {
    float currentRPM = calculateRPM();
    if (currentRPM == 0) {
      stopTime = millis();
      isBraking = false;
    }
  }

  // 5. display result
  float duration = (stopTime - startTime) / 1000.0;
  lcd.clear();
  lcd.print("Stop Time:");
  lcd.setCursor(0, 1);
  lcd.print(duration);
  lcd.print(" seconds");

  // reset servo for next trial
  delay(5000);
  brakeServo.write(0);
}

float calculateRPM() {
  pulseCount = 0;
  interrupts();
  delay(100); // sample for 100ms
  noInterrupts();
  
  // RPM = (pulses per 100ms * 10 * 60) / pulses per revolution
  // assuming 1 magnet on the rotor:
  rpm = pulseCount * 600; 
  return rpm;
}
