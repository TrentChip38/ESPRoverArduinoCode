// STATE = WAITING_FOR_LAUNCH

// loop:
//     accel = readIMU()

//     if STATE == WAITING_FOR_LAUNCH:
//         if accel_z > LAUNCH_THRESHOLD:
//             STATE = IN_FLIGHT

//     if STATE == IN_FLIGHT:
//         if steady_low_accel over T seconds:
//             STATE = LANDED
//             start_timer()

//     if STATE == LANDED:
//         if timer_expired():
//             open_door()
//             send_rover_signal()
//             break

#include <LSM6DS3.h>
#include <Wire.h>

LSM6DS3 imu(I2C_MODE, 0x6A);

// ===== Settings =====
const float alpha = 0.05;          // smoothing for rolling average
const int sampleRateHz = 50;
const int windowSeconds = 20;
const int windowSize = sampleRateHz * windowSeconds;

// ===== Storage for 20s window =====
float magBuffer[windowSize];
int bufferIndex = 0;

float avgMag = 0.0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (imu.begin() != 0) {
    Serial.println("IMU error");
    while (1);
  }

  Serial.println("IMU ready");
  
  // Initialize buffer
  for (int i = 0; i < windowSize; i++) {
    magBuffer[i] = 0;
  }
}

void loop() {
  float ax = imu.readFloatAccelX();
  float ay = imu.readFloatAccelY();
  float az = imu.readFloatAccelZ();

  // Acceleration magnitude in g
  float mag = sqrt(ax * ax + ay * ay + az * az);

  // Rolling average (low-pass filter)
  avgMag = alpha * mag + (1 - alpha) * avgMag;

  // Store in circular buffer
  magBuffer[bufferIndex] = mag;
  bufferIndex = (bufferIndex + 1) % windowSize;

  // Find max in last 20 seconds
  float maxMag = 0;
  for (int i = 0; i < windowSize; i++) {
    if (magBuffer[i] > maxMag) {
      maxMag = magBuffer[i];
    }
  }

  // Serial output
  Serial.print("Current: ");
  Serial.print(mag, 3);
  Serial.print(" g | Avg: ");
  Serial.print(avgMag, 3);
  Serial.print(" g | 20s Max: ");
  Serial.print(maxMag, 3);
  Serial.println(" g");

  delay(1000 / sampleRateHz);
}