//Libraries: NimBLE-Arduino
#include <NimBLEDevice.h>

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

void setup() {
  delay(2000);
  Serial.begin(115200);
  delay(1000);

  NimBLEDevice::init("");

  // Keep trying until connected (on the launch pad)
  while (!connectToRocketDoor()) {
    delay(2000);
  }

  Serial.println("Sleeping until DEPLOY is received...");
}

void loop() {
  //This is the basic state machine where first we are just checking for "DEPLOY"
  //Once received it will just call the other code
  if (!deployReceived) {
    delay(100);   // let NimBLE run, very low power
    return;
  }else if (!roverMoved){
    //Rover Move
    Serial.println("Rover Moves");
    roverMoved = true;
    // start LoRa, GPS, BME once here
    if (runRoverSettup){
      runRoverSettup = false;
      //Call Rover settup function to init everything
      Serial.println("Settup Rover Data");
    }
  }else{
    //Loop all the normal stuff for Rover and Lora with modules
    Serial.println("All data on Lora");
  }
}