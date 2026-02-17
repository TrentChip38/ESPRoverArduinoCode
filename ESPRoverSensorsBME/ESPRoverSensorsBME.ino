#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Use I2C: SDI-21, SCL-22
Adafruit_BME280 bme; 

void setup() {
  Serial.begin(115200);
  
  // Initialize BME280, check address 0x76 or 0x77
  if (!bme.begin(0x77)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void loop() {
  Serial.print("Temperature: ");
  Serial.print(bme.readTemperature());
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.print("Pressure: ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.println();
  delay(2000); // Wait 2 seconds
}

