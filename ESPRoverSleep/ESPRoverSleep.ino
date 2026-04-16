//Libraries: NimBLE-Arduino
#include <NimBLEDevice.h>

static NimBLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID charUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

NimBLEClient* client;
NimBLERemoteCharacteristic* rxChar;

volatile bool deployReceived = false;

void notifyCB(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool notify) {
  String msg = "";
  for (int i = 0; i < len; i++) msg += (char)data[i];

  if (msg == "DEPLOY") {
    deployReceived = true;
  }
}

bool connectToRocketDoor() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);

  Serial.println("Scanning once to connect...");

  scan->start(5, false);   // scan for 5 seconds
  NimBLEScanResults results = scan->getResults();

  for (int i = 0; i < results.getCount(); i++) {
    const NimBLEAdvertisedDevice* dev = results.getDevice(i);

    if (dev->haveName() && dev->getName() == "RocketDoor") {
      Serial.println("Found RocketDoor!");

      client = NimBLEDevice::createClient();
      if (!client->connect(dev)) {
        Serial.println("Connect failed");
        return false;
      }

      NimBLERemoteService* svc = client->getService(serviceUUID);
      if (!svc) return false;

      rxChar = svc->getCharacteristic(charUUID);
      if (!rxChar) return false;

      rxChar->subscribe(true, notifyCB);

      Serial.println("Connected and subscribed!");
      return true;
    }
  }

  Serial.println("Not found, retrying...");
  return false;
}

void setup() {
  Serial.begin(115200);
  NimBLEDevice::init("");

  while (!connectToRocketDoor()) {
    delay(2000);
  }

  Serial.println("Entering light sleep, waiting for DEPLOY...");
}

void loop() {
  if (!deployReceived) {
    esp_light_sleep_start();  // BLE notify wakes this
  }

  Serial.println("DEPLOY RECEIVED — STARTING SYSTEMS");

  // Start rover, LoRa, GPS, BME here

  while (1); // never sleep again
}