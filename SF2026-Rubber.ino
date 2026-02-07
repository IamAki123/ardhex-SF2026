#include <Servo.h>

// --- Pins ---
const int motorRPWM   = 5;  
const int motorLPWM   = 6; 
const int motorR_EN   = 7;  
const int motorL_EN   = 8;  
const int sensorPin   = 3; 
const int servoPin    = 10;

Servo brakeServo;

volatile unsigned long pulseCount = 0;
unsigned long lastCalcTime = 0;
float filteredRPM = 0;
int currentPWM = 0;
bool motorOn = false;

int targetServoPos = 0;
int currentServoPos = 0;
unsigned long lastServoUpdate = 0;
const int servoStepDelay = 20;
const int servoStepSize  = 1;

void countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  
  pinMode(motorRPWM, OUTPUT);
  pinMode(motorLPWM, OUTPUT);
  pinMode(motorR_EN, OUTPUT);
  pinMode(motorL_EN, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(sensorPin), countPulse, FALLING);

  digitalWrite(motorLPWM, LOW);
  digitalWrite(motorR_EN, LOW);
  digitalWrite(motorL_EN, LOW);
  
  brakeServo.attach(servoPin);
  brakeServo.write(0);
  currentServoPos = 0;
  targetServoPos = 0;
  brakeServo.detach();  // Start quiet
  
  Serial.println("--- Ready ---");
  Serial.println("a = SLOW START + brake 0° (released)");
  Serial.println("b = COAST + brake 15°");
  Serial.println("r = RESET servo to 0° (original spot)");
}

void loop() {
  unsigned long now = millis();

  if (Serial.available() > 0) {
    char command = Serial.read();
    
    if (command == 'a') {
      brakeServo.attach(servoPin);
      motorOn = true;
      digitalWrite(motorR_EN, HIGH);
      digitalWrite(motorL_EN, HIGH);
      currentPWM = 40;
      targetServoPos = 0;
      Serial.println(">>> SLOW ON | Brake RELEASE (0°)");
    } 
    else if (command == 'b') {
      brakeServo.attach(servoPin);
      motorOn = false;
      currentPWM = 0;
      digitalWrite(motorR_EN, LOW);
      digitalWrite(motorL_EN, LOW);
      targetServoPos = 15;
      Serial.println(">>> COAST + Brake 15°");
    }
    else if (command == 'r') {
      brakeServo.attach(servoPin);
      targetServoPos = 0;
      Serial.println(">>> RESETTING servo to original 0° spot...");
    }
  }

  // Servo movement + smart detach
  if (now - lastServoUpdate >= servoStepDelay) {
    if (abs(currentServoPos - targetServoPos) > 0) {
      if (currentServoPos < targetServoPos) {
        currentServoPos += servoStepSize;
        if (currentServoPos > targetServoPos) currentServoPos = targetServoPos;
      } else if (currentServoPos > targetServoPos) {
        currentServoPos -= servoStepSize;
        if (currentServoPos < targetServoPos) currentServoPos = targetServoPos;
      }
      brakeServo.write(currentServoPos);
    } else {
      brakeServo.detach();  // Quiet when arrived
    }
    lastServoUpdate = now;
  }

  analogWrite(motorRPWM, currentPWM);

  if (now - lastCalcTime >= 100) {
    float currentRPM = (pulseCount / 3.0) * 600.0;
    filteredRPM = (filteredRPM * 0.7) + (currentRPM * 0.3);
    Serial.print("RPM_Graph:");
    Serial.println(filteredRPM);
    pulseCount = 0;
    lastCalcTime = now;
  }
}
