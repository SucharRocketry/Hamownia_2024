#include <Arduino.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>
#include <Wire.h>
#include "FS.h"
#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true

bool off = false;
unsigned long aktualny_czas = 0;
int currentIndex = 0;


NAU7802 myScale; //Create instance of the NAU7802 class

  struct DataFrame {

    unsigned long zapamietanyCzas2;
    float nacisk;

    String toString() {

      char pomiar[20];
      sprintf(pomiar, "%lu, %8.2f", zapamietanyCzas2, nacisk);
      return String(pomiar);
    }
  };

 DataFrame pakiecik2[12500];
 uint8_t tempBuffer[sizeof(DataFrame)];



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

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } 
  else {
    Serial.println("- delete failed");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(6, OUTPUT);

  Serial.println("NAU7802");
  if (! myScale.begin()) {
    Serial.println("Failed to find NAU7802");
  }
  Serial.println("Found NAU7802");

    if (!LittleFS.begin()) {
    Serial.println("LITTLEFS Mount Failed");
    off = true;
  }

   
  delay(10000);         // czeka na ewntualne wpisanie "dane"

  
  if (Serial.available() > 0)
  {
    String text = Serial.readString();
    Serial.print(text);
    if (text == ("dane\n"))     //sprawdza czy wpisane zostało "dane"
    {
      Serial.print("DANE");
      digitalWrite(37, HIGH);
      readFile("/data.txt");                //Czyta z pamięci flash

      delay(10000);

      off = true;

      if (Serial.available() > 0)
      {
        text = Serial.readString();
        if (text == ("usun\n")){
          deleteFile(LittleFS, "/all.csv");
          delay(100);
      }}
      digitalWrite(37, LOW);
    }
  }

myScale.setGain(NAU7802_GAIN_128); //Gain can be set to 1, 2, 4, 8, 16, 32, 64, or 128.

myScale.setSampleRate(NAU7802_SPS_320); //Sample rate can be set to 10, 20, 40, 80, or 320Hz

myScale.calibrateAFE(); //Does an internal calibration. Recommended after power up, gain changes, sample rate changes, or channel changes.
  // Take 10 readings to flush out readings
  

  while(off){
    delay (10);
  }

}

void loop() {

  while (! myScale.available()) {
    delay(1);
  }

  int32_t val = myScale.getReading();
  val = val/4.391825;
  aktualny_czas = millis();
  Serial.print("Read "); Serial.print(val); Serial.print(" czas "); Serial.println(aktualny_czas);

      pakiecik2[currentIndex].zapamietanyCzas2 = aktualny_czas;
      pakiecik2[currentIndex].nacisk = val;
      currentIndex++;

    if (currentIndex >= 12000) {
    
  
    File file = LittleFS.open("/all.csv", "a");
    digitalWrite(6, HIGH); 

   for (int i = 0; i < 12000; i++) {
       
       memcpy(tempBuffer, &pakiecik2[i], sizeof(DataFrame));

        file.write(tempBuffer, sizeof(DataFrame));
    }

    file.close();
    digitalWrite(6, LOW);
    currentIndex = 0;
  }
}