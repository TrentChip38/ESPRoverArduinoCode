#include "FS.h"
#include "LittleFS.h"

// Set to true to format the filesystem the first time it is used
#define FORMAT_LITTLEFS_IF_FAILED true
String file_name = "/data2.txt";
void setup() {
  Serial.begin(115200);
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
      file.print("IREC Rover Weather Data");
      file.close();
    }
  }else{
    Serial.println("File already exists");
  }
}

void loop(){
  File file = LittleFS.open(file_name, FILE_APPEND);
  if(file) {
    file.println("New log entry2");
    file.close();
  }
  delay(2000);
}