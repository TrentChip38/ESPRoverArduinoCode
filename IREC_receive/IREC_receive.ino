//*****************************************************************BASE
#include <SPI.h>
#include <LoRa.h>
/*
Lora:
Vcc -> 3.3v
Miso -> 12
Mosi -> 13
SLCK -> 14
NSS -> 15
DIOO -> 26
Rest -> 27
GND -> GND
*/
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
// Node addressing
// -----------------------------
byte localAddress = 0xBB;   // this node's address (only used for filtering)

// -----------------------------
// Receive callback
// -----------------------------
void onReceive(int packetSize) {
  if (packetSize == 0) return;

  int recipient      = LoRa.read();  // destination
  byte sender        = LoRa.read();  // source
  byte incomingMsgId = LoRa.read();  // message ID
  byte incomingLen   = LoRa.read();  // length byte

  String incoming = "";
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incoming.length() != incomingLen) {
    Serial.println("error: length mismatch");
    return;
  }

  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("not for me");
    return;
  }

  Serial.print("\nRec from:0x" + String(sender, HEX));
  Serial.print(" to:0x" + String(recipient, HEX));
  Serial.print(" mID:" + String(incomingMsgId));
  Serial.print(" len:" + String(incomingLen));
  Serial.print(" msg:" + incoming);
  Serial.print(" RSSI:" + String(LoRa.packetRssi()));
  Serial.print(" SNR:" + String(LoRa.packetSnr()));
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== ESP32 LoRa BASE (RX ONLY) ===");

  // HSPI init
  hspi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setSPI(hspi);

  // LoRa pins
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  Serial.println("LoRa init OK. Listening...");

  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void loop() {
  // nothing: all work is done in onReceive()
}