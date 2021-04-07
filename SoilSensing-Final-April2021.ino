
//temp sensor wiring: https://lastminuteengineers.com/ds18b20-arduino-tutorial/
//upload to TTGO 1 Board
//pint out esp32 ttgo https://raw.githubusercontent.com/LilyGO/ESP32-TTGO-T1/master/image/image4.jpg
//download file from sd : https://github.com/G6EJD/ESP32-8266-File-Download/blob/master/ESP_File_Download_v01.ino

//TODO: SOIL SENSING name netowrk, guide, time 
#include <TinyGPS++.h>
#include "HardwareSerial.h";

// The TinyGPS++ object
TinyGPSPlus gps;
HardwareSerial SerialGPS(1);


#include <WiFi.h>
#include <WebServer.h>

/* Define data pins for the sensors */
#define TEMP_PIN 18
#define TEMP_PIN2 19
#define MOIST_PIN 33

#define GPS_RX 17
#define GPS_TX 16

/* Put your SSID & Password */
const char* ssid = "SoilSensing1";  // Enter SSID here
const char* password = "123456789";  //Enter Password here
int interval = 5000; //Reading interval 

/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

#include "FS.h"
#include "SD.h"
#include "SPI.h"


#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier


#define ONE_WIRE_BUS TEMP_PIN
#define ONE_WIRE_BUS2 TEMP_PIN2
OneWire oneWire(TEMP_PIN);
OneWire oneWire2(TEMP_PIN2);
DallasTemperature sensors(&oneWire);
DallasTemperature sensors2(&oneWire2);

int temp_value;
int temp_value2;
int moist_value;
String gps_date;
String gps_time;

unsigned long timer = 0;

String fileSize;
String lastDataSD;
#define LAST_DATA_ARRAY_SIZE 10 //size of array for last readings 
String lastDataArray [LAST_DATA_ARRAY_SIZE];


void setup() {

  Serial.begin(9600);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);


  server.on("/", onConnect);
  //server.on("/refresh", refresh);
  server.on("/download", SD_file_download);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");


  SerialGPS.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX); //GPS TX, RX
  pinMode(MOIST_PIN, INPUT);

  sensors.begin();
  sensors2.begin();


  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  //fs::FS &check = SD;
  //const char * pathcheck = "/soil_sensing.txt";
  File filecheck = SD.open("/soil_sensing.txt");
  Serial.println("filecheck: ");
  Serial.println(filecheck);
  if (!filecheck) {
    filecheck.close();
    writeFile(SD, "/soil_sensing.txt", "Temperature1, Temperature2, Moisture, Time, Date\n"); //first row 
  } else {
    Serial.println("file already exists");
    filecheck.close();
  }

}

void loop() {
  server.handleClient();

  while (SerialGPS.available() > 0) {
    gps.encode(SerialGPS.read());
  }

  //Date
  if (gps.date.isValid())
  {
    gps_date =  gps.date.value() ;
  }
  else
  {
    //Serial.print(F("INVALID"));
  }

  //Time
  if (gps.time.isValid())
  {
    gps_time = gps.time.value();
  }
  else
  {
    //Serial.print(F("INVALID"));
  }


  moist_value = analogRead(MOIST_PIN);
  sensors.requestTemperatures();
  sensors2.requestTemperatures();

  //Float to String to char for temp
  String str_temp = String(sensors.getTempCByIndex(0));
  int str_len_temp = str_temp.length() + 1;
  char char_array_temp[str_len_temp];
  str_temp.toCharArray(char_array_temp, str_len_temp);

  //Float to String to char for temp2
  String str_temp2 = String(sensors2.getTempCByIndex(0));
  int str_len_temp2 = str_temp2.length() + 1;
  char char_array_temp2[str_len_temp2];
  str_temp2.toCharArray(char_array_temp2, str_len_temp2);

  //Float to String to char for moisture
  String str_moist = String(moist_value);
  int str_len_moist = str_moist.length() + 1;
  char char_array_moist[str_len_moist];
  str_moist.toCharArray(char_array_moist, str_len_moist);

  //String to char for time
  String str_time = gps_time;
  int str_len_time = str_time.length() + 1;
  char char_array_time[str_len_time];
  str_time.toCharArray(char_array_time, str_len_time);

  //String to char for moisture
  String str_date = gps_date;
  int str_len_date = str_date.length() + 1;
  char char_array_date[str_len_date];
  str_date.toCharArray(char_array_date, str_len_date);

  if (millis() - timer > interval ) // do something every hour:3600000)
  {

    appendFile(SD, "/soil_sensing.txt", char_array_temp);
    appendFile(SD, "/soil_sensing.txt", ",");

    appendFile(SD, "/soil_sensing.txt", char_array_temp2);
    appendFile(SD, "/soil_sensing.txt", ",");

    appendFile(SD, "/soil_sensing.txt", char_array_moist);
    appendFile(SD, "/soil_sensing.txt", ",");

    appendFile(SD, "/soil_sensing.txt", char_array_time);
    appendFile(SD, "/soil_sensing.txt", ",");

    appendFile(SD, "/soil_sensing.txt", char_array_date);
    appendFile(SD, "/soil_sensing.txt", "\n");


    Serial.print("Temperature: ");
    Serial.println(char_array_temp);


    Serial.print("Temperature2: ");
    Serial.println(char_array_temp2);

    Serial.print("Moisture: ");
    Serial.println(char_array_moist);

    Serial.print("Time: ");
    Serial.println(char_array_time);

    Serial.print("Date: ");
    Serial.println(char_array_date);

    timer = millis();

    String lastData =  str_temp + ", " + str_temp2 + ", " + str_moist +  ", " + str_time +  ", " + str_date +  "<br>\n";

    for (int i = 0; i < LAST_DATA_ARRAY_SIZE - 1; i++) {
      lastDataArray [i] = lastDataArray[i + 1];
    }
    lastDataArray [LAST_DATA_ARRAY_SIZE - 1] = lastData;


    for (int i = 0; i < LAST_DATA_ARRAY_SIZE; i++) {
      Serial.print(i);
      Serial.print(" : ");
      Serial.println(lastDataArray[i]);
    }

  }

}


void writeFile(fs::FS & fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS & fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
   while (file.available()) {
     Serial.write(file.read());
    }
  file.close();
}


void onConnect() {

  server.send(200, "text/html", SendHTML());
  readFile(SD, "/soil_sensing.txt");
}

String SendHTML() {
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head>\n";
  ptr += "<title>Soil Sensing Data Device 1</title>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<br>\n";
  ptr += "<p>Soil Sensing 1 </p>\n";
  ptr += "<br>\n";
  ptr += "<a href='/download'><button>Download</button></a>\n";
  ptr += "<br>\n";
  ptr += "<p>Last 10 readings: </p>\n";
  ptr += "<br>\n";
  for (int i = 0; i < LAST_DATA_ARRAY_SIZE; i++) {
    ptr += lastDataArray[i];
    // ptr += "<br>\n";
  }
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}



void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}


void SD_file_download() {

  File file = SD.open("/soil_sensing.txt");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  file = SD.open("/soil_sensing.txt");
  if (file) {
    server.sendHeader("Content-Type", "text/text");
    server.sendHeader("Content-Disposition", "attachment; filename=soil_sensing.txt");
    server.sendHeader("Connection", "close");
    server.streamFile(file, "application/octet-stream");
    file.close();
  }
}
