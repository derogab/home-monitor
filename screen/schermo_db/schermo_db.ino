/*  PINOUT:
 *   
 *  D1 SCL
 *  D2 SDA
 *  D3 CLK
 *  D4 DT
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

byte displayMode = 0;
bool dispChangeMode = 0;
unsigned long lastQueryTime = 0;
int queryDelay = 10000;

void setup() {

  Serial.begin(115200);

  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

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
    
    //while (true)
      //delay(1);
  }

  WiFi.mode(WIFI_STA);

  rot.begin(ROTARY_PIN1, ROTARY_PIN2, CLICKS_PER_STEP);
  rot.setLeftRotationHandler(decDisp);
  rot.setRightRotationHandler(incDisp);
  lcd.setBacklight(255);
  dispChangeMode = 1;
}

void loop() {
  rot.loop();
  unsigned long timeNow = millis();
  if (timeNow - lastQueryTime > queryDelay){
    connectToWiFi();   // WiFi connect if not established and if connected get wifi signal strength
    lastQueryTime = timeNow;
    check_influxdb();
    
    //TODO query vere
    double resultA =  getFieldResultDouble("A");
    double resultB =  getFieldResultDouble("B");
    bool resultC =  getFieldResultBool("C");

    #ifdef DEBUG
      Serial.printf("A: %2.2f \n", resultA);
      Serial.printf("B: %2.2f \n", resultB);
      Serial.printf("C: %s \n", resultC?"true":"false");
    #endif
  }

  if(dispChangeMode){
    lcd.home();
    lcd.clear();

    //TODO print vere
    if(displayMode == 0){
      lcd.print("mod0");
    } else if (displayMode == 1){
      lcd.print("mod1");
    } else if (displayMode == 2){
      lcd.print("mod2");
    }
    dispChangeMode = 0;
  }
}

void incDisp(ESPRotary& r) {
  displayMode++;
  displayMode = displayMode % 3;
  dispChangeMode = 1;

  #ifdef DEBUG
    Serial.print("cambiato");
  #endif
}

void decDisp(ESPRotary& r) {
  if (displayMode == 0){
    displayMode = 2;
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
        delay(250);
      #endif
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
  // Construct a Flux query
  String query = "from(bucket: \"fdilauro2-bucket\") |> range(start: -2h) |> filter(fn: (r) => r[\"_field\"] == \"" + field + "\") |> last()";

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

double getFieldResultBool(String field) {
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
