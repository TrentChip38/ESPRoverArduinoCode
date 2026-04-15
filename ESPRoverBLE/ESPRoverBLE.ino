//Libraries: NimBLE-Arduino
#include <NimBLEDevice.h>

static NimBLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID charUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

NimBLEClient* client = nullptr;
NimBLERemoteCharacteristic* rxChar = nullptr;

bool connected = false;
bool scanning = false;

void notifyCB(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool notify) {
  Serial.print("Received: ");
  for (int i = 0; i < len; i++) Serial.print((char)data[i]);
  Serial.println();
}

void startScan() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->start(0, false);   // continuous scan
  scanning = true;
  Serial.println("Scanning...");
}

void setup() {
  Serial.begin(115200);

  NimBLEDevice::init("");
  startScan();
}

void loop() {
  if (!connected) {
    NimBLEScanResults results = NimBLEDevice::getScan()->getResults();

    for (int i = 0; i < results.getCount(); i++) {
      const NimBLEAdvertisedDevice* device = results.getDevice(i);

      if (device->isAdvertisingService(serviceUUID)) {
        Serial.println("Found RocketDoor!");

        NimBLEDevice::getScan()->stop();
        scanning = false;

        client = NimBLEDevice::createClient();
        if (!client->connect(device)) {
          Serial.println("Connect failed, rescanning...");
          startScan();
          return;
        }

        NimBLERemoteService* svc = client->getService(serviceUUID);
        rxChar = svc->getCharacteristic(charUUID);

        if (rxChar->canNotify()) {
          rxChar->subscribe(true, notifyCB);
          Serial.println("Connected and subscribed!");
          connected = true;
        }
        return;
      }
    }
  }

  // If connection drops, restart scan
  if (connected && !client->isConnected()) {
    Serial.println("Disconnected! Restarting scan...");
    connected = false;
    startScan();
  }

  delay(500);
}