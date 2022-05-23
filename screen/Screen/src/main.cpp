#define DEBUG

#include <LiquidCrystal_I2C.h> // display library
#include <Wire.h>              // I2C library
#include <ArduinoJson.h>
#include <MQTT.h>

#include <ESP8266WiFi.h>
#include "secrets.h"

#define DISPLAY_CHARS 16  // number of characters on a line
#define DISPLAY_LINES 2   // number of display lines
#define DISPLAY_ADDR 0x27 // display address on I2C bus
#define DISPLAY_MODE_N 6

#define incPin D3
#define decPin D4
#define ALARM_BUTTON D5           
#define BUTTON_DEBOUNCE_DELAY 200 // button debounce time in ms

#define BUZZER D8

#define MQTT_BUFFER_SIZE 1024               // the maximum size for packets being published and received
MQTTClient mqttClient(MQTT_BUFFER_SIZE);   // handles the MQTT communication protocol
WiFiClient networkClient;                  // handles the network connection to the MQTT broker
#define MQTT_TOPIC_DEVICES "unishare/devices/all_devices" 

// WiFi cfg
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password
#ifdef IP
IPAddress ip(IP);
IPAddress subnet(SUBNET);
IPAddress dns(DNS);
IPAddress gateway(GATEWAY);
#endif

LiquidCrystal_I2C lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES); // display object

volatile byte displayMode = 0;

unsigned long last_interrupt_inc = 0;
unsigned long last_interrupt_dec = 0;
unsigned long last_interrupt_alarm = 0;
unsigned long last_connect_time = 0;
unsigned long connect_delay = 5000;

volatile bool alarm_active = true;
volatile bool change_alarm = false;

void connectToWiFi();
void IRAM_ATTR buttonInterrupt();
void IRAM_ATTR isrInc();
void IRAM_ATTR isrDec();
void printDisplayInfo();
void connectToMQTTBroker();
void mqttMessageReceived(String &topic, String &payload);

void setup()
{

  Serial.begin(115200);

  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

  // // set BUTTON pin as input with pull-up
  // pinMode(ALARM_BUTTON, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(ALARM_BUTTON), buttonInterrupt, FALLING);

  // pinMode(incPin, INPUT_PULLUP);
  // pinMode(decPin, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(incPin), isrInc, FALLING);
  // attachInterrupt(digitalPinToInterrupt(decPin), isrDec, FALLING);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  if (error == 0)
  {
#ifdef DEBUG
    Serial.println(F("LCD found."));
#endif

    lcd.begin(DISPLAY_CHARS, 2); // initialize the lcd
  }
  else
  {
#ifdef DEBUG
    Serial.print(F("LCD not found. Error "));
    Serial.println(error);
    Serial.println(F("Check connections and configuration. Reset to try again!"));
#endif

    while (true)
      delay(1);
  }

  WiFi.mode(WIFI_STA);

  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.print("Home");
  lcd.setCursor(0, 1);
  lcd.print("Monitor");

  // setup MQTT
  mqttClient.begin(MQTT_BROKERIP, 1883, networkClient);   // setup communication with MQTT broker
  mqttClient.onMessage(mqttMessageReceived);              // callback on message received from MQTT broker
}

double humidity;
double temperature;
double apparent_temperature;
bool light;
bool flame;
long rssi;

void loop()
{ 
  connectToWiFi();   // connect to WiFi (if not already connected)

  connectToMQTTBroker();   // connect to MQTT broker (if not already connected)

  if(!mqttClient.loop())
    Serial.println(mqttClient.lastError());

  delay(10);

  // unsigned long timeNow = millis();
  // if (timeNow - last_connect_time > connect_delay)
  // {
  //   last_connect_time = timeNow;
  //   connectToWiFi(); // WiFi connect if not established and if connected get wifi signal strength
  //   connectToMQTTBroker();   // connect to MQTT broker (if not already connected)
  // }

  // if (alarm_active && flame)
  //   digitalWrite(BUZZER, HIGH);
  // else
  //   digitalWrite(BUZZER, LOW);
  
  // mqttClient.loop();       // MQTT client loop
    
// #ifdef DEBUG
//     Serial.printf("humidity: %2.2f \n", humidity);
//     Serial.printf("temperature: %2.2f \n", temperature);
//     Serial.printf("apparent_temperature: %2.2f \n", apparent_temperature);
//     Serial.printf("light: %s \n", light ? "true" : "false");
//     Serial.printf("flame: %s \n", flame ? "true" : "false");
//     Serial.printf("rssi: %ld \n", rssi);
// #endif
}

// Helpers

void isrInc()
{
  unsigned long now = millis();
  if (now - last_interrupt_inc > BUTTON_DEBOUNCE_DELAY)
  {
    displayMode++;
    displayMode = displayMode % DISPLAY_MODE_N;

#ifdef DEBUG
    Serial.printf("DisplayMode: %d", displayMode);
#endif

    last_interrupt_inc = now;
    printDisplayInfo();
  }
}

void isrDec()
{
  unsigned long now = millis();
  if (now - last_interrupt_dec > BUTTON_DEBOUNCE_DELAY)
  {
    if (displayMode == 0)
      displayMode = DISPLAY_MODE_N - 1;
    else
      displayMode--;

#ifdef DEBUG
    Serial.printf("DisplayMode: %d", displayMode);
#endif

    last_interrupt_dec = now;
    printDisplayInfo();
  }
}

void IRAM_ATTR buttonInterrupt()
{
  unsigned long now = millis();
  if (now - last_interrupt_alarm > BUTTON_DEBOUNCE_DELAY)
  {
    alarm_active = !alarm_active;
    last_interrupt_alarm = now;
    change_alarm = true;

    lcd.home();
    lcd.clear();
    lcd.printf("Alarm state:");
    lcd.setCursor(0, 1);
    lcd.printf("%s", alarm_active ? "ON" : "OFF");

    if (alarm_active && flame)
      digitalWrite(BUZZER, HIGH);
    else
      digitalWrite(BUZZER, LOW);
  }
}

void printDisplayInfo()
{
  lcd.home();
  lcd.clear();

  if (displayMode == 0)
  {
    lcd.printf("Humidity:");
    lcd.setCursor(0, 1);
    lcd.printf("%2.2f %%", humidity);
  }
  else if (displayMode == 1)
  {
    lcd.printf("Temp:");
    lcd.setCursor(0, 1);
    lcd.printf("%2.2f C", temperature);
  }
  else if (displayMode == 2)
  {
    lcd.printf("Apparent temp:");
    lcd.setCursor(0, 1);
    lcd.printf("%2.2f C", apparent_temperature);
  }
  else if (displayMode == 3)
  {
    lcd.printf("Light:");
    lcd.setCursor(0, 1);
    lcd.printf("%s", light ? "ON" : "OFF");
  }
  else if (displayMode == 4)
  {
    lcd.printf("Fire:");
    lcd.setCursor(0, 1);
    lcd.printf("%s", flame ? "YES" : "NO");
  }
  else if (displayMode == 5)
  {
    lcd.printf("WiFi Signal:");
    lcd.setCursor(0, 1);
    lcd.printf("%ld dB", rssi);
  }
}

void connectToWiFi()
{
  // connect to WiFi (if not already connected)
  if (WiFi.status() != WL_CONNECTED)
  {
#ifdef DEBUG
    Serial.print(F("Connecting to SSID: "));
    Serial.println(ssid);
#endif

#ifdef IP
    WiFi.config(ip, dns, gateway, subnet); // by default network is configured using DHCP
#endif

    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
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

void connectToMQTTBroker() {
  if (!mqttClient.connected()) {   // not connected
    Serial.print(F("\nConnecting to MQTT broker..."));
    while (!mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.print(F("."));
      delay(1000);
    }
    Serial.println(F("\nConnected!"));

    // connected to broker, subscribe topics
    mqttClient.subscribe(MQTT_TOPIC_DEVICES);
    Serial.println(F("\nSubscribed to devices topic!"));
  }
}

void mqttMessageReceived(String &topic, String &payload) {
  // this function handles a message from the MQTT broker
  Serial.println("Incoming MQTT message: " + topic + " - " + payload);

  StaticJsonDocument<1024> doc;
  deserializeJson(doc, payload);

  // extract the values
  JsonArray array = doc.as<JsonArray>();
  for(JsonVariant v : array) {
      Serial.println(v["MAC_ADDRESS"].as<String>());
  }
}