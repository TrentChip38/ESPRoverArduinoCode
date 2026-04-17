//Board: Seeed XIAO nRF52840 Sense
//Board manager: Seeed nRF52 Boards
//Libraries: Seeed_Arduino_LSM6DS3 and Seeeduino or nrf52
#include <bluefruit.h>
// #include <Servo.h>
#include <LSM6DS3.h>
#include <Wire.h>
#include <Servo.h>
//Make servo for door
Servo doorServo;
const int servoPin = 4;   // XIAO pin D4
//Instantiate bluetooth
BLEUart bleuart;
//Instantiate the IMU
LSM6DS3 imu(I2C_MODE, 0x6A);
//Sample rate and thresholds
const float LAUNCH_THRESHOLD = 4.0; // 4g
const float LAUNCH_THRESHOLD_TIME = 1000; // 1000 ms = 1 sec
const float LANDING_THRESHOLD_HIGH = 1.1; // g
const float LANDING_THRESHOLD_LOW = 0.95; // g
const int LANDING_COUNT = 100; // a couple seconds?
int land_counter = 0;
const float alpha = 0.05;
float avgMag = 0.0;
const int sampleRateHz = 50;
unsigned long startTime = 0;
unsigned long currentTime = 0;
unsigned long elapsedTime = 0;
unsigned int total_flight_amount = 1000 * 60 * 10;//10 min

//ENUM for STATE
enum SeedState {
  WAITING_FOR_LAUNCH,
  LAUNCHING,
  IN_FLIGHT,
  LANDING,
  LANDED,
  OPEN,
  DEPLOY
};
SeedState STATE = WAITING_FOR_LAUNCH;
//4g for 1 sec
//Wait 10 min
//Check not less than 1g (free fall) or greater (moving)


// bool deployed = false;
// bool landed = false;

void setup() {
  Serial.begin(115200);
  if (imu.begin() != 0) {
    Serial.println("IMU error");
    while (1);
  }
  //Servo setup and latch door on boot?
  doorServo.attach(servoPin);
  // doorServo.write(0); // door closed
  //Set up BLE advertised data
  Bluefruit.begin();
  //Bluefruit.setTxPower(4);
  Bluefruit.setName("RocketDoor");

  bleuart.begin();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  //Bluefruit.Advertising.start();
  Bluefruit.ScanResponse.addName();   // optional but helpful

  Bluefruit.Advertising.start(0);     // advertise forever
}

// void openDoorAndSignal() {
//   // Fully open
//   doorServo.write(160);
//   delay(2000);
//   bleuart.println("DEPLOY");
//   deployed = true;
// }
float readIMU() {
  float ax = imu.readFloatAccelX();
  float ay = imu.readFloatAccelY();
  float az = imu.readFloatAccelZ();
  return sqrt(ax * ax + ay * ay + az * az);
}

void loop() {
  //Get current time
  // Serial.print("Elapsed Time (ms): ");
  // Serial.println(elapsedTime);

  // bleuart.println("Hello from rocket!");
  //delay(1000);
  // Read IMU accelaration magnitude
  float accel = readIMU();
  // Rolling average (low-pass filter)
  avgMag = alpha * accel + (1 - alpha) * avgMag;

  // Serial output
  Serial.print("Current: ");
  Serial.print(accel, 3);
  Serial.print(" : ");
  Serial.print(avgMag, 3);
  Serial.print(" g, State: ");
  Serial.println(STATE);

  //State machine (Wait, in flight, landing, landed)
  if (STATE == WAITING_FOR_LAUNCH) {
    //bleuart.println("WAITING");
      if (avgMag > LAUNCH_THRESHOLD) {
          STATE = LAUNCHING;
          startTime = millis(); // Record starting time
      }
  }else if (STATE == LAUNCHING) {
    //bleuart.println("LAUNCHING");
    STATE = IN_FLIGHT;
    currentTime = millis();
    elapsedTime = currentTime - startTime;
    //If 1 second has elapsed since first 4g
    if (elapsedTime >= LAUNCH_THRESHOLD_TIME){
      //And acceleration is still > 4 g
      if (avgMag > LAUNCH_THRESHOLD){
        //Then its launching
        STATE = IN_FLIGHT;
        //Restart timer for next timing?
      }else{
        //Its not really launching
        STATE = WAITING_FOR_LAUNCH;
      }
    }
  }else if (STATE == IN_FLIGHT) {
    //bleuart.println("FLIGHT");
      delay(total_flight_amount); // wait for min duration of flight
      //Flight should be 7 min. We will wait 10 min.
      STATE = LANDING;
      land_counter = 0;
  }else if (STATE == LANDING) {
    //bleuart.println("LANDING");
      if (accel < LANDING_THRESHOLD_HIGH && accel > LANDING_THRESHOLD_LOW) {
        land_counter += 1;
        //If not moving or in free fall for a few seconds, consider it landed
        if (land_counter >= LANDING_COUNT){
          STATE = LANDED;
        }
      }
  }else if (STATE == LANDED) {
    //bleuart.println("LANDED");
      // if (timer_expired()) {
      //     open_door();
      //     send_rover_signal();
      //     break;
      // }
      // Fully open
      doorServo.write(160);
      delay(2000);
      //delay(6 * 1000);   // 60 s post-landing delay
      STATE = DEPLOY;
  }else if (STATE == OPEN) {
    //bleuart.println("OPEN");
  }else if (STATE == DEPLOY) {
    //Only bleuart print is for DEPLOY cause any BLE event wakes ESP
    bleuart.println("DEPLOY");
      //landed = true;
  }

  delay(1000 / sampleRateHz);

  // //After landing and delay, open door and signal rover
  // if (landed && !deployed) {
  //   delay(6 * 1000);   // 60 s post-landing delay
  //   openDoorAndSignal();
  // }
}