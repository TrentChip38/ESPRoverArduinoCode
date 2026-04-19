//Originally using the LoRa Deplus comm callback example code by Tom Igoe and Chris Drake
//Libraries: NimBLE-Arduino, LoRa, Adafruit BME, TinyGPSPlusPlus
/*Rover Wiring for all peripherals to the ESP32-WROOM-32E
From peripheral to pin on ESP32
Lora:
Vcc -> 3.3v
Miso -> 12
Mosi -> 13
SLCK -> 14
NSS -> 15
DIOD -> 26
Rest -> 27
GND -> GND
BME:
VIN -> 3.3v
GND -> GND
SCK -> 22
SDI -> 21
NEO GPS:
Vcc -> 3.3v
RX -> 17
TX -> 16
GND -> GND
Motors:
2,18
32,33
Sense Pin: 25 (Unused)
*/
#include <NimBLEDevice.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <LoRa.h>
#include "FS.h"
#include "LittleFS.h"
// Set to true to format the filesystem the first time it is used
#define FORMAT_LITTLEFS_IF_FAILED true
//File name to store data logged
String file_name = "VegasLaunch.txt";//"/dataTest3.txt";

//Motor pins
// int E1 = 18;
// int M1 = 2;
// int E2 = 32;
// int M2 = 33;
int E1 = 33;
int M1 = 32;
int E2 = 2;
int M2 = 18;

void initMotors(){
  //Set up motor pins
  pinMode(M1, OUTPUT);
  pinMode(M2, OUTPUT);
  pinMode(E1, OUTPUT);
  pinMode(E2, OUTPUT);
}
void moveMotors(){
  int value;
  int speed = 254;
  for(value = 0 ; value <= 255; value+=5)
  {
    digitalWrite(M1,HIGH);//M1 is direction HIGH for clockwise low for counter clockwise   
    digitalWrite(M2,LOW);
    analogWrite(E1, speed);   //PWM Speed Control
    analogWrite(E2, speed);   //PWM Speed Control
    delay(30);
  }
  digitalWrite(M1,LOW);
}

//Sens pin
//int sense = 25;
int speedValue = 200;
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
volatile bool packetReady = false;
void onReceive(int packetSize) {
  if (packetSize > 0) {
    packetReady = true;
  }
}
//Sensor Payload data
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
unsigned long lastSend = 0;
const unsigned long sendInterval = 5000; // 5 seconds

//Functions from IREC_send original settup and loop
void setupRover() {
  //Wait untill "DEPLOY"
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
  //pinMode(sense, INPUT);
}
void loopRover() {
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
    //Creat LoRa packet payload
    String payload = generatePayload();
    //Log payload data
    File file = LittleFS.open(file_name, FILE_APPEND);
    if(file) {
      file.println(payload);
      file.close();
    }

    //Send LoRa packet payload
    Serial.println("\nSending LoRa packet:");
    Serial.println(payload);

    String fullpayload = "CALL:KM7CUO;" + payload;

    LoRa.beginPacket();
    LoRa.write(0xFF);                 // broadcast
    LoRa.write(0xBB);                 // sender ID
    LoRa.write((byte)random(0, 255)); // message ID

    LoRa.write(fullpayload.length());
    LoRa.print(fullpayload);
    LoRa.endPacket();

    LoRa.receive(); // back to RX
  }
  // if (digitalRead(sense) == HIGH){
  //   delay(10000);
  //   digitalWrite(M1,HIGH);//M1 is direction HIGH for clockwise low for counter clockwise
  //   digitalWrite(M2,LOW);
  //   analogWrite(E1, speedValue);   //PWM Speed Control
  //   analogWrite(E2, speedValue);
  // }
}

bool runRoverSettup = true;
bool roverMoved = false;

static NimBLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E"); // UART Service
static NimBLEUUID charUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");    // RX Notify

NimBLEClient* client = nullptr;
NimBLERemoteCharacteristic* rxChar = nullptr;

volatile bool deployReceived = false;

// Called automatically when XIAO sends BLEUart.print(...)
void notifyCB(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool notify) {
  String msg = "";
  for (int i = 0; i < len; i++) msg += (char)data[i];

  Serial.print("BLE Received: ");
  Serial.println(msg);

  if (msg == "DEPLOY") {
    deployReceived = true;
  }
}

bool connectToRocketDoor() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);

  Serial.println("Scanning for UART UUID...");

  scan->start(0, false);   // CONTINUOUS SCAN like OG code

  unsigned long startTime = millis();

  while (millis() - startTime < 10000) {   // scan up to 10 seconds
    NimBLEScanResults results = scan->getResults();

    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice* dev = results.getDevice(i);

      if (dev->isAdvertisingService(serviceUUID)) {
        Serial.println("Found RocketDoor!");

        scan->stop();   // stop scanning once found

        client = NimBLEDevice::createClient();
        if (!client->connect(dev)) {
          Serial.println("Connection failed");
          return false;
        }

        NimBLERemoteService* svc = client->getService(serviceUUID);
        rxChar = svc->getCharacteristic(charUUID);
        rxChar->subscribe(true, notifyCB);

        Serial.println("Connected and subscribed!");
        delay(2000);
        return true;
      }
    }

    delay(100);  // let scan buffer fill
  }

  scan->stop();
  Serial.println("UUID not found, retrying...");
  return false;
}

void setupFS() {
  //Start Little FS
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
    Serial.println("LittleFS Mount Failed");
    return;
  }
  //Create/open file if not already existing
  if (!LittleFS.exists(file_name)) {
    Serial.println("File does not exist. Creating it now...");
    File file = LittleFS.open(file_name, FILE_WRITE);
    if(file) {
      file.println("IREC Rover Weather Data");
      file.close();
    }
  }else{
    Serial.println("File already exists");
  }
}

void setup() {
  delay(2000);
  Serial.begin(115200);
  delay(1000);
  NimBLEDevice::init("");
  // Keep trying until connected (on the launch pad)
  while (!connectToRocketDoor()) {
    delay(2000);
  }
  Serial.println("Idle until DEPLOY is received...");
}

void loop() {
  //This is the basic state machine where first we are just checking for "DEPLOY"
  //Once received it will just call the other code
  if (!deployReceived) {
    delay(100);   // let NimBLE run, very low power
    return;
  }else if (!roverMoved){
    //Rover Move
    // initMotors();
    // delay(1000);
    // moveMotors();
    // delay(1000);
    Serial.println("Rover Moves");
    roverMoved = true;
    // start LoRa, GPS, BME once here
    if (runRoverSettup){
      runRoverSettup = false;
      //Call Rover settup function to init everything
      setupRover();
      setupFS();
      Serial.println("Setup Rover Data");
    }
  }else{
    //Loop all the normal stuff for Rover and Lora with modules
    loopRover();
    //Serial.println("All data on Lora");
  }
}