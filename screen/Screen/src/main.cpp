/*  PINOUT:
 *   
 *  D1 SCL
 *  D2 SDA
 *  D3 CLK
 *  D4 DT
 *  D5 SW
 */
 
#define DEBUG

#include <LiquidCrystal_I2C.h>   // display library
#include <Wire.h>                // I2C library
#include "ESPRotary.h"

#include <ESP8266WiFi.h>
#include "secrets.h"
#include <InfluxDbClient.h>

#define ROTARY_PIN1  D3
#define ROTARY_PIN2 D4
#define CLICKS_PER_STEP   4   // this number depends on your rotary encoder 

#define DISPLAY_CHARS 16    // number of characters on a line
#define DISPLAY_LINES 2     // number of display lines
#define DISPLAY_ADDR 0x27   // display address on I2C bus

#define BUTTON D5                  // button pin, eg. D1 is GPIO5 and has an optional internal pull-up
#define BUTTON_DEBOUNCE_DELAY 200  // button debounce time in ms

// WiFi cfg
char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password
#ifdef IP
IPAddress ip(IP);
IPAddress subnet(SUBNET);
IPAddress dns(DNS);
IPAddress gateway(GATEWAY);
#endif
WiFiClient client;

// InfluxDB cfg
InfluxDBClient client_idb(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point pointDevice("device_status");

LiquidCrystal_I2C lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES);   // display object
ESPRotary rot;

volatile byte displayMode = 0;
bool dispChangeMode = 0;
unsigned long lastQueryTime = 0;
unsigned long queryDelay = 10000;

unsigned long last_interrupt = 0;
volatile bool alarm_active = true;

void incDisp(ESPRotary&);
void decDisp(ESPRotary&);
void connectToWiFi();
void check_influxdb();
double getFieldResultDouble(String);
bool getFieldResultBool(String);
long getFieldResultLong(String);
void IRAM_ATTR buttonInterrupt();

void setup() {

  Serial.begin(115200);

  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

  // set BUTTON pin as input with pull-up
  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonInterrupt, FALLING);

  if (error == 0) {
    #ifdef DEBUG
      Serial.println(F("LCD found."));
    #endif
    
    lcd.begin(DISPLAY_CHARS, 2);   // initialize the lcd

  } else {
    #ifdef DEBUG
      Serial.print(F("LCD not found. Error "));
      Serial.println(error);
      Serial.println(F("Check connections and configuration. Reset to try again!"));
    #endif
    
    while (true)
      delay(1);
  }

  WiFi.mode(WIFI_STA);

  rot.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP);
  rot.setLeftRotationHandler(decDisp);
  rot.setRightRotationHandler(incDisp);
  lcd.setBacklight(255);
  dispChangeMode = 1;
}

double humidity;
double temperature;
double apparent_temperature;
bool light;
bool flame;
long rssi;

void loop() {
  rot.loop();
  unsigned long timeNow = millis();
  if (timeNow - lastQueryTime > queryDelay){
    connectToWiFi();   // WiFi connect if not established and if connected get wifi signal strength
    lastQueryTime = timeNow;
    check_influxdb();
    
    humidity =  getFieldResultDouble("humidity");
    temperature =  getFieldResultDouble("temperature");
    apparent_temperature = getFieldResultDouble("apparent_temperature");
    light = getFieldResultBool("light");
    flame = getFieldResultBool("flame");
    rssi = getFieldResultLong("rssi");

    #ifdef DEBUG
      Serial.printf("humidity: %2.2f \n", humidity);
      Serial.printf("temperature: %2.2f \n", temperature);
      Serial.printf("apparent_temperature: %2.2f \n", apparent_temperature);
      Serial.printf("light: %s \n", light?"true":"false");
      Serial.printf("flame: %s \n", flame?"true":"false");      
      Serial.printf("rssi: %ld \n", rssi);
    #endif
  }
}

// Helpers

void incDisp(ESPRotary& r) {
  displayMode++;
  displayMode = displayMode % 5;
  dispChangeMode = 1;

  #ifdef DEBUG
    Serial.print("cambiato");
  #endif
}

void decDisp(ESPRotary& r) {
  if (displayMode == 0){
    displayMode = 5;
  } else {
    displayMode = displayMode - 1;
  }
  dispChangeMode = 1;
}

void connectToWiFi() {
  // connect to WiFi (if not already connected)
  if (WiFi.status() != WL_CONNECTED) {
    #ifdef DEBUG
      Serial.print(F("Connecting to SSID: "));
      Serial.println(ssid);
   #endif

#ifdef IP
    WiFi.config(ip, dns, gateway, subnet);   // by default network is configured using DHCP
#endif

    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      #ifdef DEBUG
        Serial.print(F(".")); 
      #endif
      delay(250);
    }
    
    #ifdef DEBUG
      Serial.println(F("\nConnected!"));
    #endif
  }
}

void check_influxdb() {
  // Check server connection
  if (client_idb.validateConnection()) {
    #ifdef DEBUG
      Serial.print(F("Connected to InfluxDB: "));
      Serial.println(client_idb.getServerUrl());
   #endif
  } else {
    #ifdef DEBUG
      Serial.print(F("InfluxDB connection failed: "));
      Serial.println(client_idb.getLastErrorMessage());
    #endif
  }
}

double getFieldResultDouble(String field) {
  String bucket = INFLUXDB_BUCKET;
  // Construct a Flux query
  String query = "from(bucket: \"" + bucket + "\") |> range(start: -2h) |> filter(fn: (r) => r[\"_field\"] == \"" + field + "\") |> last()";

  #ifdef DEBUG
    // Print composed query
    Serial.print("Querying with: ");
    Serial.println(query);
  #endif
  
  // Send query to the server and get result
  FluxQueryResult result = client_idb.query(query);
  double value = 0;
  // Iterate over rows. Even there is just one row, next() must be called at least once.
  while (result.next()) {
    value = result.getValueByName("_value").getDouble();
  }
  // Check if there was an error
  if(result.getError() != "") {
    #ifdef DEBUG
      Serial.print("Query result error: ");
      Serial.println(result.getError());
    #endif
  }
  result.close();
  return value;
}

bool getFieldResultBool(String field) {
  // Construct a Flux query
  String query = "from(bucket: \"fdilauro2-bucket\") |> range(start: -2h) |> filter(fn: (r) => r[\"_field\"] == \"" + field + "\") |> last()";

  #ifdef DEBUG
    // Print composed query
    Serial.print("Querying with: ");
    Serial.println(query);
  #endif
  
  // Send query to the server and get result
  FluxQueryResult result = client_idb.query(query);
  bool value = 0;
  // Iterate over rows. Even there is just one row, next() must be called at least once.
  while (result.next()) {
    value = result.getValueByName("_value").getBool();
  }
  // Check if there was an error
  if(result.getError() != "") {
    #ifdef DEBUG
      Serial.print("Query result error: ");
      Serial.println(result.getError());
    #endif
  }
  result.close();
  return value;
}

long getFieldResultLong(String field) {
  // Construct a Flux query
  String query = "from(bucket: \"fdilauro2-bucket\") |> range(start: -2h) |> filter(fn: (r) => r[\"_field\"] == \"" + field + "\") |> last()";

  #ifdef DEBUG
    // Print composed query
    Serial.print("Querying with: ");
    Serial.println(query);
  #endif
  
  // Send query to the server and get result
  FluxQueryResult result = client_idb.query(query);
  bool value = 0;
  // Iterate over rows. Even there is just one row, next() must be called at least once.
  while (result.next()) {
    value = result.getValueByName("_value").getLong();
  }
  // Check if there was an error
  if(result.getError() != "") {
    #ifdef DEBUG
      Serial.print("Query result error: ");
      Serial.println(result.getError());
    #endif
  }
  result.close();
  return value;
}

void IRAM_ATTR buttonInterrupt(){
  unsigned long now = millis();
  if (now - last_interrupt > BUTTON_DEBOUNCE_DELAY){
      alarm_active = !alarm_active;
      last_interrupt = now;
      if (displayMode == 5){
        displayMode = 0;
      } else {
        displayMode++;
      }

      lcd.home();
      lcd.clear();

      //TODO print vere
      if(displayMode == 0){
        lcd.printf("humidity: %2.2f ", humidity);
      } else if (displayMode == 1){
        lcd.printf("temperature: %2.2f \n", temperature);
      } else if (displayMode == 2){
        lcd.printf("apparent temperature: %2.2f \n", apparent_temperature);
      } else if (displayMode == 3){
        lcd.printf("light: %s", light?"true":"false");
      } else if (displayMode == 4){
        lcd.printf("flame: %s", flame?"true":"false");
      }
    }

}
