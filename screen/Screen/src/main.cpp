#define DEBUG

#include <LiquidCrystal_I2C.h> // display library
#include <Wire.h>              // I2C library
#include <ArduinoJson.h>
#include <MQTT.h>

#include <ESP8266WiFi.h>
#include "secrets.h"
#include "sensors_t.h"

#define DISPLAY_CHARS 16  // number of characters on a line
#define DISPLAY_LINES 2   // number of display lines
#define DISPLAY_ADDR 0x27 // display address on I2C bus
#define DISPLAY_MODE_N 6
#define DISPLAY_REFRESH_RATE 5000

#define incPin D3
#define decPin D4
#define ALARM_BUTTON D5
#define DEVICE_BUTTON D6
#define BUTTON_DEBOUNCE_DELAY 200 // button debounce time in ms

#define BUZZER D8

#define MQTT_BUFFER_SIZE 2048            // the maximum size for packets being published and received
MQTTClient mqttClient(MQTT_BUFFER_SIZE); // handles the MQTT communication protocol
WiFiClient networkClient;                // handles the network connection to the MQTT broker
#define MQTT_TOPIC_DEVICES "unishare/devices/all_sensors"
#define MQTT_TOPIC_SENSORS "unishare/sensors/"
#define MQTT_TOPIC_SETUP "unishare/devices/setup"

String mqtt_topic_status = "unishare/devices/status/";
String mac_address;
String mqtt_topic_my_status;

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
volatile int device_index = 0;

volatile unsigned long last_interrupt_inc = 0;
volatile unsigned long last_interrupt_dec = 0;
volatile unsigned long last_interrupt_alarm = 0;
volatile unsigned long last_interrupt_devices_display = 0;

unsigned long last_refresh = 0;

volatile bool alarm_active = true;
volatile bool change_alarm = false;
bool flame;

sensors_t all_sensors[10];
int number_of_devices = 0;

void connectToWiFi();
void IRAM_ATTR alarmInterrupt();
void IRAM_ATTR deviceDisplayInterrupt();
void IRAM_ATTR isrInc();
void IRAM_ATTR isrDec();
void printDisplayInfo();
void connectToMQTTBroker();
void mqttMessageReceived(String &topic, String &payload);
String clearMacAddress(String mac_address);

void setup()
{

  Serial.begin(115200);

  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

  // set BUTTON pin as input with pull-up
  pinMode(ALARM_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALARM_BUTTON), alarmInterrupt, FALLING);
  pinMode(DEVICE_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DEVICE_BUTTON), deviceDisplayInterrupt, FALLING);
  pinMode(incPin, INPUT_PULLUP);
  pinMode(decPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(incPin), isrInc, FALLING);
  attachInterrupt(digitalPinToInterrupt(decPin), isrDec, FALLING);

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
  mqttClient.begin(MQTT_BROKERIP, 1883, networkClient); // setup communication with MQTT broker
  mqttClient.onMessage(mqttMessageReceived);            // callback on message received from MQTT broker

  String to_replace = String(':');
  String replaced = "";
  mac_address = clearMacAddress(String(WiFi.macAddress()));
  mqtt_topic_my_status = mqtt_topic_status + mac_address;
  mac_address.replace(to_replace, replaced);

  DynamicJsonDocument doc_will(128);
  doc_will["connected"] = false;
  char buffer_will[128];
  serializeJson(doc_will, buffer_will);
  const char *topic_status = mqtt_topic_my_status.c_str();
  mqttClient.setWill(topic_status, buffer_will, true, 1);
}

bool sent_setup = false;
void loop()
{
  if (!sent_setup)
  {
    connectToWiFi();       // connect to WiFi (if not already connected)
    connectToMQTTBroker(); // connect to MQTT broker (if not already connected)
    DynamicJsonDocument doc(256);
    doc["mac_address"] = mac_address;
    doc["type"] = "screen";
    doc["name"] = "schermo1";
    char buffer[256];
    size_t n = serializeJson(doc, buffer);

#ifdef DEBUG
    Serial.print(F("JSON setup message: "));
    Serial.println(buffer);
#endif

    if (mqttClient.publish(MQTT_TOPIC_SETUP, buffer, n, false, 1))
    {
      sent_setup = true;
    }
  }
  else
  {
    connectToWiFi();       // connect to WiFi (if not already connected)
    connectToMQTTBroker(); // connect to MQTT broker (if not already connected)

    if (!mqttClient.loop())
    {
#ifdef DEBUG
      Serial.println(mqttClient.lastError());
#endif
      mqttClient.disconnect();
    }

    if (alarm_active && flame)
      digitalWrite(BUZZER, HIGH);
    else
      digitalWrite(BUZZER, LOW);

    unsigned long now = millis();
    if (now - last_refresh > DISPLAY_REFRESH_RATE)
    {
      printDisplayInfo();
      last_refresh = now;
    }
  }
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
    Serial.printf("DisplayMode: %d \n", displayMode);
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
    Serial.printf("DisplayMode: %d \n", displayMode);
#endif

    last_interrupt_dec = now;
    printDisplayInfo();
  }
}

void IRAM_ATTR alarmInterrupt()
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

void IRAM_ATTR deviceDisplayInterrupt()
{
  if (number_of_devices == 0)
  {
    lcd.home();
    lcd.clear();
    lcd.printf("No devices");
    return;
  }
  unsigned long now = millis();
  if (now - last_interrupt_devices_display > BUTTON_DEBOUNCE_DELAY)
  {
    last_interrupt_devices_display = now;
    device_index++;
    device_index = device_index % number_of_devices;
    lcd.home();
    lcd.clear();
    String mac_string = all_sensors[device_index].mac;
    char buffer[mac_string.length() + 1];
    mac_string.toCharArray(buffer, mac_string.length() + 1);
    lcd.printf("%s", buffer);
    lcd.setCursor(0, 1);
    lcd.printf("Status %s", all_sensors[device_index].status ? "ON" : "OFF");
#ifdef DEBUG
    Serial.print(F("Device to display: "));
    Serial.println(mac_string);
#endif
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
    lcd.printf("%2.2f %%", all_sensors[device_index].humidity);
  }
  else if (displayMode == 1)
  {
    lcd.printf("Temp:");
    lcd.setCursor(0, 1);
    lcd.printf("%2.2f C", all_sensors[device_index].temperature);
  }
  else if (displayMode == 2)
  {
    lcd.printf("Apparent temp:");
    lcd.setCursor(0, 1);
    lcd.printf("%2.2f C", all_sensors[device_index].apparent_temperature);
  }
  else if (displayMode == 3)
  {
    lcd.printf("Light:");
    lcd.setCursor(0, 1);
    lcd.printf("%s", all_sensors[device_index].light ? "ON" : "OFF");
  }
  else if (displayMode == 4)
  {
    lcd.printf("Fire:");
    lcd.setCursor(0, 1);
    lcd.printf("%s", all_sensors[device_index].flame ? "YES" : "NO");
  }
  else if (displayMode == 5)
  {
    lcd.printf("WiFi Signal:");
    lcd.setCursor(0, 1);
    lcd.printf("%ld dB", all_sensors[device_index].rssi);
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

void connectToMQTTBroker()
{
  if (!mqttClient.connected())
  { // not connected
#ifdef DEBUG
    Serial.print(F("\nConnecting to MQTT broker..."));
#endif
    while (!mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD))
    {
#ifdef DEBUG
      Serial.print(F("."));
#endif
      delay(200);
    }
#ifdef DEBUG
    Serial.println(F("\nConnected!"));
#endif

    // connected to broker, subscribe topics
    mqttClient.subscribe(MQTT_TOPIC_DEVICES, 1);
    String topic_sensors = String(MQTT_TOPIC_SENSORS) + "#";
    mqttClient.subscribe(topic_sensors, 1);
    String topic_status_all = mqtt_topic_status + "#";
    mqttClient.subscribe(topic_status_all);
#ifdef DEBUG
    Serial.printf("Subscribed to %s topic! \n", MQTT_TOPIC_DEVICES);
    Serial.printf("Subscribed to %s topic! \n", MQTT_TOPIC_SENSORS);
#endif

    DynamicJsonDocument doc_stat(128);
    doc_stat["connected"] = true;
    char buffer_stat[128];
    size_t n = serializeJson(doc_stat, buffer_stat);
    const char *topic_status = mqtt_topic_my_status.c_str();
    mqttClient.publish(topic_status, buffer_stat, n, true, 1);
  }
}

void mqttMessageReceived(String &topic, String &payload)
{
// this function handles a message from the MQTT broker
#ifdef DEBUG
  Serial.println("Incoming MQTT message: " + topic + " - " + payload);
#endif

  if (topic == MQTT_TOPIC_DEVICES)
  {
    StaticJsonDocument<2048> devices_doc;
    deserializeJson(devices_doc, payload);
    // extract the values
    JsonArray array = devices_doc.as<JsonArray>();
    number_of_devices = 0;
    bool device_found;
    for (JsonVariant v : array)
    {
      String mac_to_find = v["MAC_ADDRESS"].as<String>();
      device_found = false;
      for (int i = 0; i < 10; i++)
      {
        if (all_sensors[i].mac == mac_to_find)
        {
          device_found = true;
          break;
        }
      }
      if (!device_found)
      {
        all_sensors[number_of_devices].mac = mac_to_find;
      }
      number_of_devices++;
    }
    return;
  }

  if (topic.startsWith(MQTT_TOPIC_SENSORS))
  {
    int s_index = topic.lastIndexOf('/');
    int length = topic.length();
    String data_type = topic.substring(s_index + 1, length);
    String sub_s = topic.substring(0, s_index);
    s_index = sub_s.lastIndexOf('/');
    length = sub_s.length();
    String mac_to_find = sub_s.substring(s_index + 1, length);

#ifdef DEBUG
    Serial.println("Data type: " + data_type);
    Serial.println("MAC: " + mac_to_find);
#endif

    StaticJsonDocument<32> sensor_doc;
    deserializeJson(sensor_doc, payload);
    int index = 0;
    int i = 0;
    for (i = 0; i < 10; i++)
    {
      if (all_sensors[i].mac == mac_to_find)
      {
        index = i;
        break;
      }
    }

    if (i ==  10)
      return;

    if (data_type == "humidity")
    {
      all_sensors[index].humidity = sensor_doc["value"].as<double>();
      return;
    }
    if (data_type == "temperature")
    {
      all_sensors[index].temperature = sensor_doc["value"].as<double>();
      return;
    }
    if (data_type == "apparent_temperature")
    {
      all_sensors[index].apparent_temperature = sensor_doc["value"].as<double>();
      return;
    }
    if (data_type == "flame")
    {
      all_sensors[index].flame = sensor_doc["value"].as<bool>();
      if (sensor_doc["value"].as<bool>())
      {
        flame = true;
      }
      else
      {
        flame = false;
      }
      return;
    }
    if (data_type == "light")
    {
      all_sensors[index].light = sensor_doc["value"].as<bool>();
      return;
    }
    if (data_type == "rssi")
    {
      all_sensors[index].rssi = sensor_doc["value"].as<long>();
      return;
    }
  }

  if (topic.startsWith(mqtt_topic_status))
  {
    int s_index = topic.lastIndexOf('/');
    int length = topic.length();
    String mac_to_find = topic.substring(s_index + 1, length);

    StaticJsonDocument<32> stat_doc;
    deserializeJson(stat_doc, payload);
    int index = 0;
    int i = 0;
    for (i = 0; i < 10; i++)
    {
      if (all_sensors[i].mac == mac_to_find)
      {
        index = i;
        break;
      }
    }
    if (i==10)
      return;
    all_sensors[index].status = stat_doc["connected"].as<bool>();
    return;
  }
  return;
}

String clearMacAddress(String mac_address)
{
  // Prepare
  String to_replace = String(':');
  String replaced = "";
  // Exec
  mac_address.replace(to_replace, replaced);
  // Return
  return mac_address;
}