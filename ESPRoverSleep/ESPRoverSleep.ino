//Libraries: NimBLE-Arduino
#include <NimBLEDevice.h>

static NimBLEUUID serviceUUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static NimBLEUUID charUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

NimBLEClient* client = nullptr;
NimBLERemoteCharacteristic* rxChar = nullptr;

bool connected = false;
bool deployReceived = false;

void notifyCB(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool notify) {
  String msg = "";
  for (int i = 0; i < len; i++) msg += (char)data[i];

  Serial.println("Received: " + msg);

  if (msg.indexOf("DEPLOY") >= 0) {
    deployReceived = true;
  }
}

bool scanForDevice() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);

  Serial.println("Scan window...");
  scan->start(1, false);   // 1 second scan

  NimBLEScanResults results = scan->getResults();

  for (int i = 0; i < results.getCount(); i++) {
    const NimBLEAdvertisedDevice* device = results.getDevice(i);

    if (device->isAdvertisingService(serviceUUID)) {
      Serial.println("Found RocketDoor!");
      scan->stop();

      client = NimBLEDevice::createClient();
      if (!client->connect(device)) {
        Serial.println("Connect failed");
        return false;
      }

      NimBLERemoteService* svc = client->getService(serviceUUID);
      rxChar = svc->getCharacteristic(charUUID);

      if (rxChar->canNotify()) {
        rxChar->subscribe(true, notifyCB);
        Serial.println("Connected and subscribed!");
        connected = true;
        return true;
      }
    }
  }

  return false;
}

void lightSleepSeconds(int seconds) {
  esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
  esp_light_sleep_start();
}

void setup() {
  Serial.begin(115200);
  NimBLEDevice::init("");
}

void loop() {
  if (!connected) {
    if (!scanForDevice()) {
      Serial.println("Sleeping...");
      lightSleepSeconds(5);   // << LOW POWER PART
      return;
    }
  }

  if (deployReceived) {
    Serial.println("DEPLOY received!");
    // Turn on LoRa, GPS, BME here
    while (true);
  }

  if (connected && !client->isConnected()) {
    Serial.println("Disconnected, retrying...");
    connected = false;
  }

  delay(200);
}