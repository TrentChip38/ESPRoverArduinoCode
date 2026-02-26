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
/*
#include <SPI.h>              // include libraries
#include <LoRa.h>

SPIClass hspi(HSPI);

// WeMos D1 - use these pins for the WeMos D1 board . (note that you need to solder
const int csPin = 15;   // LoRa radio chip select NSS
const int resetPin = 27; // LoRa radio reset (optional) - NB - this is also the blue LED on the ESP8266 board
const int irqPin = 26;  // LoRa radio hardware interrupt pin DIO0
 
char outbuff[]="abcdefghijklmnopqrstuvwxyz0123456789\n\000";
int counter=0;

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
long sinterval = 2000;          // interval between sends

void setup() {
  Serial.begin(115200);
  while((!Serial)&&(micros()<2000000));

  hspi.begin(14, 12, 13, 15);
  LoRa.setSPI(hspi);

  Serial.print("#\n# ");
  Serial.print(__FILE__);
  Serial.print(" ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  
//  Serial.begin(115200);                   // initialize serial
  //while (!Serial);

  Serial.println("LoRa Duplex with callback");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin


  if (!LoRa.begin(433E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing (this causes the WeMos D1 to spew stack errors; probably because it's needing to reach the loop to stay alive)
  }

  LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (millis() - lastSendTime > sinterval) {
    sprintf(outbuff,"LeLoRa World %s %d",__TIME__,counter++);
//    String message = "HeLoRa World!";   // send a message
//    sendMessage(message);
    sendMessage(outbuff);
//    Serial.print("Sending ");Serial.println(outbuff);
    Serial.print("s");
    lastSendTime = millis();            // timestamp the message
    sinterval = random(2000) + 1000;     // 2-3 seconds
    LoRa.receive();                     // go back into receive mode
  }
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available()) {            // can't use readString() in callback, so
    incoming += (char)LoRa.read();      // add bytes one by one
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.print("\tRec from:0x" + String(sender, HEX));
  Serial.print(" to:0x" + String(recipient, HEX));
  Serial.print(" mID:" + String(incomingMsgId));
  Serial.print(" l:" + String(incomingLength));
  Serial.print(" Msg:" + incoming);
  Serial.print(" RSSI:" + String(LoRa.packetRssi()));
  Serial.print(" Snr:" + String(LoRa.packetSnr()));
  Serial.print(" freqErr:" + String(LoRa.packetFrequencyError()));
  Serial.print(" rnd:" + String(LoRa.random()));

  Serial.println();
}
*/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <LoRa.h>

//BME280
// Use I2C: SDI-21, SCL-22

// GPS Module RX/TX pins
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600 // NEO-6M default baud rate [7]
// -----------------------------
// LoRa Pin Mapping (ESP32 HSPI)
// -----------------------------

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_CS    15
#define LORA_RST   27
#define LORA_DIO0  26

Adafruit_BME280 bme;//BME280
TinyGPSPlus gps;//NEO-6M
HardwareSerial gpsSerial(1); // Use UART1
SPIClass hspi(HSPI);//LoRa
// -----------------------------
// Receive Handling
// -----------------------------
volatile bool packetReady = false;

// ISR-safe callback (no heavy work here)
void onReceive(int packetSize) {
  if (packetSize > 0) {
    packetReady = true;
  }
}

// -----------------------------
// Generate Sensor Data
// -----------------------------
String generatePayload() {
  float temp = bme.readTemperature();
  float hum  = bme.readHumidity();
  float pressure = bme.readPressure();

  if (gps.location.isValid()) {
  
  float lat  = gps.location.lat();   // around Utah
  float lon  = gps.location.lng();
  float alt  = gps.altitude.meters();
  } else {
    Serial.println("Location: Not Available (Waiting for Lock)");
  }
  
  float wind = random(0, 200) / 10.0;                  // 0–20 m/s

  String payload = "GPS(Lat: " + String(lat, 5) + ", Lon: " + String(lon, 5) + 
  ", Alt: " + String(alt, 1) + ") ";
  payload += "T:" + String(temp, 1);
  payload += " H:" + String(hum, 1);
  payload += " P:" + String(pressure / 100.0, 1);
  payload += " W:" + String(wind, 1);

  return payload;
}
// -----------------------------
// Setup
// -----------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

   // Initialize BME280, check address 0x76 or 0x77
  if (!bme.begin(0x77)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  // Begin Serial2 for GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("GPS Simulation Start");

  Serial.println("\n=== ESP32 LoRa Node Booting ===");

  // Initialize HSPI
  hspi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setSPI(hspi);

  // Set LoRa pins
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  // Start LoRa
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  Serial.println("LoRa init OK.");

  // Register callback
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

// -----------------------------
// Main Loop
// -----------------------------
unsigned long lastSend = 0;
const unsigned long sendInterval = 10000; // 10 seconds

void loop() {
  // -----------------------------
  // Handle incoming packets
  // -----------------------------
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

      Serial.println("\n--- Packet Received ---");
      Serial.println("From: 0x" + String(sender, HEX));
      Serial.println("To:   0x" + String(recipient, HEX));
      Serial.println("ID:   " + String(msgID));
      Serial.println("Data: " + incoming);
      Serial.println("RSSI: " + String(LoRa.packetRssi()));
      Serial.println("SNR:  " + String(LoRa.packetSnr()));
      Serial.println("------------------------");
    }

    LoRa.receive();  // return to RX mode
  }

  // -----------------------------
  // Send packet every 10 seconds
  // -----------------------------
  if (millis() - lastSend > sendInterval) {
    lastSend = millis();

     // Read data from GPS
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      String payload = generatePayload();
    }
  }
  // If no data comes for 5 seconds, check wiring
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("No GPS detected: check wiring.");
  }

    Serial.println("\nSending packet:");
    Serial.println(payload);

    LoRa.beginPacket();
    LoRa.write(0xFF);       // broadcast
    LoRa.write(0xBB);       // sender ID
    LoRa.write((byte)random(0,255)); // message ID
    LoRa.write(payload.length());
    LoRa.print(payload);
    LoRa.endPacket();

    LoRa.receive(); // back to RX
  }
}
