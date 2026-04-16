#include <Servo.h>

Servo doorServo;

const int servoPin = 4;   // XIAO pin D4

void setup() {
  doorServo.attach(servoPin);
}

void loop() {
  // Closed position
  doorServo.write(0);
  delay(2000);

  // Open position
  doorServo.write(90);
  delay(2000);

  // Fully open
  doorServo.write(160);
  delay(2000);
}