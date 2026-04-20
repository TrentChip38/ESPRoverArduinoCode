// # Editor     : Lauren from DFRobot
// # Date       : 17.02.2012

// # Product name: L298N motor driver module DF-MD v1.3
// # Product SKU : DRI0002
// # Version     : 1.0

// # Description:
// # The sketch for using the motor driver L298N
// # Run with the PWM mode

// # Connection:
// #        M1 pin  -> Digital pin 4
// #        E1 pin  -> Digital pin 5
// #        M2 pin  -> Digital pin 7
// #        E2 pin  -> Digital pin 6
// #        Motor Power Supply -> Centor blue screw connector(5.08mm 3p connector)
// #        Motor A  ->  Screw terminal close to E1 driver pin
// #        Motor B  ->  Screw terminal close to E2 driver pin
// #
// # Note: You should connect the GND pin from the DF-MD v1.3 to your MCU controller. They should share the GND pins.
// #

int E1 = 18;
int M1 = 2;
int E2 = 32;
int M2 = 33;

void setup()
{
    pinMode(M1, OUTPUT);
    pinMode(M2, OUTPUT);
    pinMode(E1, OUTPUT);
    pinMode(E2, OUTPUT);
}
int speedValue = 255;
void loop()
{
  // int value;
  // for(value = 0 ; value <= 255; value+=5)
  // {
  //   digitalWrite(M1,HIGH);
  //   digitalWrite(M2,LOW);
  //   analogWrite(E1, value);   //PWM Speed Control
  //   analogWrite(E2, value);   //PWM Speed Control
  //   delay(30);
  // }


  digitalWrite(M1,HIGH);
  digitalWrite(M2,LOW);
  digitalWrite(E1,HIGH);
  digitalWrite(E2,HIGH);
  // analogWrite(E1, speedValue);   //PWM Speed Control
  // analogWrite(E2, speedValue);   //PWM Speed Control
  delay(30);
}
