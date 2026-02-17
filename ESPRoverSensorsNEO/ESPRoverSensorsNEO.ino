#include <TinyGPS++.h>
#include <HardwareSerial.h>

// GPS Module RX/TX pins
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600 // NEO-6M default baud rate [7]

TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // Use UART1

void setup() {
  Serial.begin(115200);
  // Begin Serial2 for GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("GPS Simulation Start");
}

void loop() {
  // Read data from GPS
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      displayInfo();
    }
  }

  // If no data comes for 5 seconds, check wiring
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("No GPS detected: check wiring.");
  }
}

void displayInfo() {
  if (gps.location.isValid()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());
  } else {
    Serial.println("Location: Not Available (Waiting for Lock)");
  }
}
