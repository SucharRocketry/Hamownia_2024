#include <Wire.h>
#include <SparkFun_Qwiic_Scale_NAU7802_Arduino_Library.h>  // Poprawna biblioteka
#include <LittleFS.h>                                      // System plików LittleFS
#include <TaskScheduler.h>                                 // Biblioteka do zarządzania zadaniami
#include <vector>

#define SAMPLE_RATE 320          // Częstotliwość próbkowania 320Hz
#define BUFFER_SIZE 128          // Rozmiar bufora zmniejszony
#define FILE_NAME "/data.txt"    // Nazwa pliku na systemie LittleFS

// Inicjalizacja czujnika NAU7802
NAU7802 myScale;

// Bufor do przechowywania danych
struct DataPoint {
  long value;
  unsigned long timestamp;
};

std::vector<DataPoint> dataQueue;
volatile bool isBufferFull = false;  // Flaga oznaczająca pełny bufor

// Task Scheduler
Scheduler ts;

// Funkcja do odczytu danych z czujnika
void readSensor() {
  // Odczytaj wartość z tensometru bezpośrednio
  long value = myScale.getReading();  // Odczytaj pojedynczą wartość
  unsigned long currentTime = millis();  // Pobierz aktualny czas w ms

  // Blokada bufora
  noInterrupts();
  if (dataQueue.size() < BUFFER_SIZE) {
    dataQueue.push_back({value, currentTime});  // Wstawienie wartości i znacznika czasowego do bufora
  } else {
    isBufferFull = true;
  }
  interrupts();
}

// Funkcja do zapisu danych na systemie plików
void saveDataToFile() {
  if (!LittleFS.exists(FILE_NAME)) {
    File dataFile = LittleFS.open(FILE_NAME, "w");
    dataFile.close();
  }

  File dataFile = LittleFS.open(FILE_NAME, "a");

  if (!dataFile) {
    Serial.println("Nie można otworzyć pliku do zapisu!");
    return;
  }

  while (!dataQueue.empty()) {
    noInterrupts();
    DataPoint dataPoint = dataQueue.front();  // Pobierz wartość i znacznik czasowy z bufora
    dataQueue.erase(dataQueue.begin());       // Usuń pierwszy element (pop)
    interrupts();

    // Zapisz dane w formacie: "timestamp,value"
    dataFile.print(dataPoint.timestamp);  // Zapisz znacznik czasowy
    dataFile.print(",");                  // Separator
    dataFile.println(dataPoint.value);    // Zapisz wartość
  }

  dataFile.close();
}

// Funkcja do wyświetlenia zawartości pliku data.txt
void displayFileContents() {
  if (LittleFS.exists(FILE_NAME)) {
    File dataFile = LittleFS.open(FILE_NAME, "r");  // Otwórz plik do odczytu
    if (dataFile) {
      Serial.println("Zawartość pliku data.txt:");
      while (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');  // Czytaj linia po linii
        Serial.println(line);  // Wyświetl linie w konsoli
      }
      dataFile.close();
    } else {
      Serial.println("Błąd przy otwieraniu pliku data.txt.");
    }
  } else {
    Serial.println("Plik data.txt nie istnieje.");
  }
}

// Zadanie do odczytu danych z czujnika
Task taskReadSensor(1000 / SAMPLE_RATE, TASK_FOREVER, &readSensor);

// Zadanie do zapisywania danych do pliku
Task taskSaveData(1000 / SAMPLE_RATE, TASK_FOREVER, &saveDataToFile);

void setup() {
  Serial.begin(115200);
  
  // Inicjalizacja systemu plików
  if (!LittleFS.begin()) {
    Serial.println("Błąd montowania systemu plików.");
    return;
  }

  // Wyświetl zawartość pliku data.txt przy starcie
  displayFileContents();
  
  // Inicjalizacja magistrali I2C
  Wire.begin();  // Inicjalizuje magistralę I2C (domyślnie na pinach SDA/SCL)

  // Inicjalizacja czujnika NAU7802
  if (!myScale.begin()) {
    Serial.println("Nie można zainicjować NAU7802!");
    return;
  }

  // Ustaw częstotliwość próbkowania
  myScale.setSampleRate(NAU7802_SPS_320);

  // Dodaj zadania do planera
  ts.addTask(taskReadSensor);
  ts.addTask(taskSaveData);
  
  // Uruchom zadania
  taskReadSensor.enable();
  taskSaveData.enable();
}

void loop() {
  ts.execute();  // Uruchomienie zadań planera
}
