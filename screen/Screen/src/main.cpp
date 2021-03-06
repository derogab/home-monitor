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

#define INC_PIN D3
#define DEVICE_BUTTON D6
#define BUTTON_DEBOUNCE_DELAY 200 // button debounce time in ms
#define USER_DELAY 30000
#define CONNECTION_TIMEOUT_CUSTOM 15000

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
volatile unsigned long last_interrupt_devices_display = 0;
volatile unsigned long last_user_interaction = 0;

unsigned long last_refresh = 0;

sensors_t all_sensors[10];
int number_of_devices = 0;

bool connectToWiFi();
void IRAM_ATTR deviceDisplayInterrupt();
void IRAM_ATTR isrInc();
void printDisplayInfo();
bool connectToMQTTBroker();
void mqttMessageReceived(String &topic, String &payload);
String clearMacAddress(String mac_address);

void setup()
{

  Serial.begin(115200);

  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

  // set BUTTON pin as input with pull-up
  pinMode(DEVICE_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DEVICE_BUTTON), deviceDisplayInterrupt, FALLING);
  pinMode(INC_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INC_PIN), isrInc, FALLING);

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
    if (connectToWiFi())
    {
      if (connectToMQTTBroker())
      {
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
    }
  }
  else
  {
    if (connectToWiFi())
    {
      if (connectToMQTTBroker())
      {
        if (!mqttClient.loop())
        {
#ifdef DEBUG
          Serial.println(mqttClient.lastError());
#endif
          mqttClient.disconnect();
        }

        unsigned long now = millis();
        if (now - last_refresh > DISPLAY_REFRESH_RATE)
        {
          printDisplayInfo();
          last_refresh = now;
        }
      }
    }
  }
  unsigned long now = millis();
  if (now - last_user_interaction > USER_DELAY)
  {
#ifdef DEBUG
    Serial.println("Going to sleep");
#endif
    mqttClient.disconnect();
    lcd.clear();
    lcd.noBacklight();
    ESP.deepSleep(0);
  }
}

// Helpers
void isrInc()
{
  unsigned long now = millis();
  last_user_interaction = now;
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

void IRAM_ATTR deviceDisplayInterrupt()
{
  unsigned long now = millis();
  last_user_interaction = now;
  if (number_of_devices == 0)
  {
    lcd.home();
    lcd.clear();
    lcd.printf("No devices");
    return;
  }
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
    last_refresh = now;
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

bool connectToWiFi()
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
    unsigned long wifi_now = millis();
    unsigned long wifi_start_time = millis();
    while (WiFi.status() != WL_CONNECTED && (wifi_now - wifi_start_time < CONNECTION_TIMEOUT_CUSTOM))
    {
#ifdef DEBUG
      Serial.print(F("."));
#endif
      delay(250);
      wifi_now = millis();
    }

#ifdef DEBUG
    if (WiFi.status() != WL_CONNECTED)
      Serial.println(F("\nFailed to connect to wifi"));
    else
      Serial.println(F("\nConnected!"));
#endif
    if (WiFi.status() != WL_CONNECTED)
      return false;
    else
      return true;
  }
  return true;
}

bool connectToMQTTBroker()
{
  if (!mqttClient.connected())
  { // not connected
#ifdef DEBUG
    Serial.print(F("\nConnecting to MQTT broker..."));
#endif
    unsigned long mqtt_now = millis();
    unsigned long mqtt_start_time = millis();
    while (!mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD) && (mqtt_now - mqtt_start_time < CONNECTION_TIMEOUT_CUSTOM))
    {
#ifdef DEBUG
      Serial.print(F("."));
#endif
      delay(200);
      mqtt_now = millis();
    }
    if (!mqttClient.connected())
    {
#ifdef DEBUG
      Serial.println(F("\nFailed to connect to MQTT"));
#endif
      return false;
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
    return true;
  }
  return true;
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

    if (i == 10)
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
    if (i == 10)
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