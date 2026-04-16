#include "FS.h"
#include "LittleFS.h"
String file_name = "/dataTest.txt";  //"/data2.txt";
void setup() {
  Serial.begin(115200);
  if(!LittleFS.begin()){ return; }

  File file = LittleFS.open(file_name, "r");
  if(!file){ return; }

  while(file.available()){
    Serial.write(file.read()); // Dumps file content to Serial Monitor
  }
  file.close();
}
void loop(){
  delay(1000);
}
