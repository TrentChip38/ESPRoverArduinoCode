//Board: Seeed XIAO nRF52840 Sense
//Board manager: Seeed nRF52 Boards
//Libraries: Seeed_Arduino_LSM6DS3 and Seeeduino or nrf52
#include <bluefruit.h>
// #include <Servo.h>
#include <LSM6DS3.h>
#include <Wire.h>
//Instantiate bluetooth
BLEUart bleuart;
//Instantiate the IMU
LSM6DS3 imu(I2C_MODE, 0x6A);
//Sample rate and thresholds
const float LAUNCH_THRESHOLD = 8.0; // g
const float LANDING_THRESHOLD = 1.05; // g
const int sampleRateHz = 50;
//ENUM for STATE
enum SeedState {
  WAITING_FOR_LAUNCH,
  IN_FLIGHT,
  LANDING,
  LANDED
};
SeedState STATE = WAITING_FOR_LAUNCH;

// Servo doorServo;

const int servoPin = 5;
bool deployed = false;
bool landed = false;

void setup() {
  Serial.begin(115200);
  if (imu.begin() != 0) {
    Serial.println("IMU error");
    while (1);
  }
  //Servo setup and latch door on boot?
  // doorServo.attach(servoPin);
  // doorServo.write(0); // door closed
  //Set up BLE advertised data
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("RocketDoor");

  bleuart.begin();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.start();
}

void openDoorAndSignal() {
  // doorServo.write(90);   // open door
  delay(2000);

  bleuart.println("DEPLOY");
  deployed = true;
}
float readIMU() {
  float ax = imu.readFloatAccelX();
  float ay = imu.readFloatAccelY();
  float az = imu.readFloatAccelZ();
  return sqrt(ax * ax + ay * ay + az * az);
}

void loop() {
  bleuart.println("Hello from rocket!");
  delay(1000);
  // Read IMU accelaration magnitude
  float accel = readIMU();

  // Serial output
  Serial.print("Current: ");
  Serial.print(accel, 3);
  Serial.print(" g, State: ");
  Serial.println(STATE);

  //State machine (Wait, in flight, landing, landed)
  if (STATE == WAITING_FOR_LAUNCH) {
      if (accel > LAUNCH_THRESHOLD) {
          STATE = IN_FLIGHT;
          //Start timer?
      }
  }else if (STATE == IN_FLIGHT) {
      delay(1000 * 10); // wait for min duration of flight
      STATE = LANDING;
      // if (steady_low_accel over T seconds) {
      //     STATE = LANDED;
      //     start_timer();
      // }
  }else if (STATE == LANDING) {
      if (accel < LANDING_THRESHOLD) {
          //If not moving for like 10 seconds, consider it landed
          STATE = LANDED;
      }
  }else if (STATE == LANDED) {
      // if (timer_expired()) {
      //     open_door();
      //     send_rover_signal();
      //     break;
      // }
      delay(6 * 1000);   // 60 s post-landing delay
      landed = true;
  }

  delay(1000 / sampleRateHz);

  //After landing and delay, open door and signal rover
  if (landed && !deployed) {
    delay(6 * 1000);   // 60 s post-landing delay
    openDoorAndSignal();
  }
}