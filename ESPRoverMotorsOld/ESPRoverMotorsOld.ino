// # Editor     : Lauren from DFRobot
// # Date       : 17.02.2012

// # Product name: L298N motor driver module DF-MD v1.3
// # Product SKU : DRI0002
// # Version     : 1.0

// # Description:
// # The sketch for using the motor driver L298N
// # Run with the PWM mode

// # Connection:
// #        M1 pin  -> Digital pin 27
// #        E1 pin  -> Digital pin 14
// #        M2 pin  -> Digital pin 25
// #        E2 pin  -> Digital pin 26
// #        Motor Power Supply -> Centor blue screw connector(5.08mm 3p connector)
// #        Motor A  ->  Screw terminal close to E1 driver pin
// #        Motor B  ->  Screw terminal close to E2 driver pin
// #
// # Note: You should connect the GND pin from the DF-MD v1.3 to your MCU controller. They should share the GND pins.
// #

int E1 = 33;
int M1 = 32;
int E2 = 34;
int M2 = 35;

void setup()
{
    pinMode(M1, OUTPUT);
    pinMode(M2, OUTPUT);
}

void loop()
{
  int value;
  for(value = 0 ; value <= 255; value+=5)
  {
    digitalWrite(M1,HIGH);//M1 is direction HIGH for clockwise low for counter clockwise   
    digitalWrite(M2,LOW);
    analogWrite(E1, value);   //PWM Speed Control
    analogWrite(E2, value);   //PWM Speed Control
    delay(30);
  }

   for(value = 0 ; value <= 255; value+=5)
  {
    digitalWrite(M1,LOW);//M1 is direction HIGH for clockwise low for counter clockwise
    digitalWrite(M2,HIGH);
    analogWrite(E1, value);   //PWM Speed Control
    analogWrite(E2, value);   //PWM Speed Control
    delay(30);
  }

}
