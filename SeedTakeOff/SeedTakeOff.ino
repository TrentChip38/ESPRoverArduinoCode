#include <LSM6DS3.h>
#include <Wire.h>

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

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (imu.begin() != 0) {
    Serial.println("IMU error");
    while (1);
  }

  Serial.println("IMU ready");
}

float readIMU() {
  float ax = imu.readFloatAccelX();
  float ay = imu.readFloatAccelY();
  float az = imu.readFloatAccelZ();
  return sqrt(ax * ax + ay * ay + az * az);
}

void loop() {
  // Read IMU accelaration magnitude
  float accel = readIMU();

  // Serial output
  Serial.print("Current: ");
  Serial.print(accel, 3);
  Serial.print(" g, State: ");
  Serial.println(STATE);

  //State machine (Wait, in flight, landed)
  if (STATE == WAITING_FOR_LAUNCH) {
      if (accel > LAUNCH_THRESHOLD) {
          STATE = IN_FLIGHT;
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
    int i = 0;//Nothing
      // if (timer_expired()) {
      //     open_door();
      //     send_rover_signal();
      //     break;
      // }
  }

  delay(1000 / sampleRateHz);
}