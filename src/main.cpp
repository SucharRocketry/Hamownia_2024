#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>
#include <FS.h>
#include <LittleFS.h>

int mydata;

void readFile(const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = LittleFS.open(path, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) { Serial.write(file.read()); }
  file.close();
}

void writeFile(const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  delay(2000);  // Make sure the CREATE and LASTWRITE times are different
  file.close();
}

void appendFile(const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = LittleFS.open(path, "a");
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {

  } else {
    Serial.println("Append failed");
  }
  file.close();
}

NAU7802 myScale; //Create instance of the NAU7802 class

void setup() {
  Wire.begin();

  Serial.begin(115200);
  
  Serial.println("Hamownia max 1kN");
  
  Serial.println("Mount LittleFS");
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
  else{
    Serial.println("Little FS Mounted Successfully");
  }
    
    if (myScale.begin() == false)
  {
    Serial.println("Problem with wiring");
    while (1);
  }

myScale.setGain(NAU7802_GAIN_128); //Gain can be set to 1, 2, 4, 8, 16, 32, 64, or 128.

  myScale.setSampleRate(NAU7802_SPS_320); //Sample rate can be set to 10, 20, 40, 80, or 320Hz

  myScale.calibrateAFE(); //Does an internal calibration. Recommended after power up, gain changes, sample rate changes, or channel changes.

   // Check if the file already exists to prevent overwritting existing data
   bool fileexists = LittleFS.exists("/data.txt");
   Serial.print(fileexists);
   if(!fileexists) {
       Serial.println("File doesn’t exist");
       Serial.println("Creating file...");
       // Create File and add header
       writeFile("/data.txt", "Hamownia \r\n");
   }
   else {
       Serial.println("File already exists");
   }
   readFile("/data.txt");
}
void loop() {
  mydata = myScale.getReading();
  appendFile("/data.txt", (String(mydata)+ "\r\n").c_str()); //Append data to the file
  
}