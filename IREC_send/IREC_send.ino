/* LoRa Duplex communication wth callback

  Sends a message every half second, and uses callback
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFF as the broadcast address.

  Note: while sending, LoRa radio is not listening for incoming messages.
  Note2: when using the callback method, you can't use any of the Stream
  functions that rely on the timeout, such as readString, parseInt(), etc.

  created 28 April 2017 by Tom Igoe
  Modified 14 Nov 2018 by Chris Drake

======
WIRING
======
esp32      <==> LoRa

GND        ---- GND
3V3        ---- VCC
D6* (io19) ---- MISO
D7* (io23) ---- MOSI
D5* (io18) ---- SLCK
D8  (io15) ---- NSS
D12*(io26) ---- DIO0
D4  (io27)  ---- REST (optional - NB: D4 is wired to the blue LED)  
*/

//***************************************************************************field

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <LoRa.h>

// -----------------------------
// BME280 (I2C)
// SDA = 21, SCL = 22
// -----------------------------
Adafruit_BME280 bme;

// -----------------------------
// GPS (NEO-6M) on UART1
// RXD2 = 16 (ESP32 RX), TXD2 = 17 (ESP32 TX)
// -----------------------------
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

// -----------------------------
// LoRa Pin Mapping (ESP32 HSPI)
// -----------------------------
#define LORA_SCK   14
#define LORA_MISO  12
#define LORA_MOSI  13
#define LORA_CS    15
#define LORA_RST   27
#define LORA_DIO0  26

SPIClass hspi(HSPI);

// -----------------------------
// LoRa Receive Handling
// -----------------------------
volatile bool packetReady = false;

void onReceive(int packetSize) {
  if (packetSize > 0) {
    packetReady = true;
  }
}

// -----------------------------
// Generate Sensor Payload
// -----------------------------
String generatePayload() {
  // BME280 readings
  float temp     = bme.readTemperature();          // °C
  float hum      = bme.readHumidity();             // %
  float pressure = bme.readPressure();             // Pa

  // GPS readings
  float lat = 0.0, lon = 0.0, alt = 0.0;
  if (gps.location.isValid()) {
    lat = gps.location.lat();
    lon = gps.location.lng();
  }
  if (gps.altitude.isValid()) {
    alt = gps.altitude.meters();
  }

  // Dummy windspeed
  float wind = random(0, 200) / 10.0;              // 0–20 m/s

  String payload = "GPS(Lat:" + String(lat, 5) +
                   ",Lon:" + String(lon, 5) +
                   ",Alt:" + String(alt, 1) + ") ";

  payload += "T:" + String(temp, 1);
  payload += " H:" + String(hum, 1);
  payload += " P:" + String(pressure / 100.0, 1); // hPa
  payload += " W:" + String(wind, 1);

  return payload;
}

// -----------------------------
// Setup
// -----------------------------
unsigned long lastSend = 0;
const unsigned long sendInterval = 5000; // 5 seconds

//Motor pins
int E1 = 33;
int M1 = 32;
int E2 = 34;
int M2 = 35;
//Sens pin
int sense = 25;
int speedValue = 200;
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32 BME280 + GPS + LoRa Node Booting ===");

  // I2C for BME280
  Wire.begin();
  if (!bme.begin(0x77)) {
    Serial.println("Could not find a valid BME280 sensor at 0x77, check wiring!");
    while (1);
  }
  Serial.println("BME280 init OK.");

  // GPS UART
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("GPS UART init OK.");

  // LoRa HSPI
  hspi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setSPI(hspi);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa init OK.");

  LoRa.onReceive(onReceive);
  LoRa.receive();

  //Set up for pin sensing
  pinMode(sense, INPUT);
  //Set up motor pins
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);
  //pinMode(E1, OUTPUT);
  //pinMode(E2, OUTPUT);
}

// -----------------------------
// Main Loop
// -----------------------------
void loop() {
  // Continuously feed GPS parser
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Warn if GPS is not talking at all
  static bool gpsWarned = false;
  if (!gpsWarned && millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("No GPS detected: check wiring.");
    gpsWarned = true;
  }

  // Handle incoming LoRa packets
  if (packetReady) {
    packetReady = false;

    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      int recipient = LoRa.read();
      int sender    = LoRa.read();
      int msgID     = LoRa.read();
      int length    = LoRa.read();

      String incoming = "";
      while (LoRa.available()) {
        incoming += (char)LoRa.read();
      }

      Serial.println("\n--- LoRa Packet Received ---");
      Serial.println("From: 0x" + String(sender, HEX));
      Serial.println("To:   0x" + String(recipient, HEX));
      Serial.println("ID:   " + String(msgID));
      Serial.println("Len:  " + String(length));
      Serial.println("Data: " + incoming);
      Serial.println("RSSI: " + String(LoRa.packetRssi()));
      Serial.println("SNR:  " + String(LoRa.packetSnr()));
      Serial.println("-----------------------------");
    }

    LoRa.receive(); // back to RX mode
  }

  // Send packet every 5 seconds
  if (millis() - lastSend > sendInterval) {
    lastSend = millis();

    String payload = generatePayload();

    Serial.println("\nSending LoRa packet:");
    Serial.println(payload);

    LoRa.beginPacket();
    LoRa.write(0xFF);                 // broadcast
    LoRa.write(0xBB);                 // sender ID
    LoRa.write((byte)random(0, 255)); // message ID
    LoRa.write(payload.length());
    LoRa.print(payload);
    LoRa.endPacket();

    LoRa.receive(); // back to RX
  }
  if (digitalRead(sense) == HIGH){
    delay(10000);
    digitalWrite(M1,HIGH);//M1 is direction HIGH for clockwise low for counter clockwise
    digitalWrite(M2,LOW);
    analogWrite(E1, speedValue);   //PWM Speed Control
    analogWrite(E2, speedValue);
  }
}