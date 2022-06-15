#include <Arduino.h>

// Include sensors libraries
// --------------
// Include DHT 11 library
#include <Adafruit_Sensor.h>
#include <DHT.h>
// Include WIFi Library
#include <ESP8266WiFi.h>
// Include JSON Library
#include <ArduinoJson.h>
// Include MQTT Library
#include <MQTT.h>

// Include SECRETs
#include "secrets.h"

// Init Mode
#define DEBUG

// Sensors
// --------------
// Buildi-In LEDs
#define LED1 D0
#define LED2 D4
// Flame Detector
#define FLAME D1
// DHT11 - Temperature & Humidity Sensor
#define DHT_PIN D2
#define DHT_TYPE DHT11 // Sensor type: DHT 11
// Photoresistor
#define PHOTORESISTOR A0            // photoresistor pin
#define PHOTORESISTOR_THRESHOLD 900 // turn led on for light values lesser than this
// WiFi signal
#define RSSI_THRESHOLD -60 // WiFi signal strength threshold

#define LOG_DELAY 60000
#define MQTT_CONTROL_DELAY 10000
#define AC_CONTROL_DELAY 30000

#define MQTT_TOPIC_SETUP "unishare/devices/setup"

// Actuators
//-----
#define LIGHT D5
#define AC_R D6
#define AC_G D7
#define AC_B D8

// Init
// --------------
// Initialize millis var
unsigned long currentTime;
// Initialize setup time
unsigned long lastSetupTime = 0;
// Initialize temperature & humidity time
unsigned long lastTempTime = 0;
// Initialize photoresistor log time
unsigned long lastLightLogTime = 0;
// Initialize flame log time
unsigned long lastFlameLogTime = 0;
// Initialize rssi log time
unsigned long lastRssiLog = 0;
// Initialize ac control time
unsigned long lastAcControl = 0;
bool temp_read = false;

// Initialize DHT sensor
DHT dht = DHT(DHT_PIN, DHT_TYPE);

// Configs
// --------------
// WiFi config
char ssid[] = SECRET_SSID; // your network SSID (name)
char pass[] = SECRET_PASS; // your network password
#ifdef IP
IPAddress ip(IP);
IPAddress subnet(SUBNET);
IPAddress dns(DNS);
IPAddress gateway(GATEWAY);
#endif
// MQTT cfg
#define MQTT_BUFFER_SIZE 1024            // the maximum size for packets being published and received
MQTTClient mqttClient(MQTT_BUFFER_SIZE); // handles the MQTT communication protocol
WiFiClient networkClient;                // handles the network connection to the MQTT broker

String clean_mac_address;
String sensors_topic = "unishare/sensors/";
String control_topic = "unishare/control/";
String light_control_topic;
String ac_control_topic;
String mqtt_topic_status = "unishare/devices/status/";

// Globals
// --------------
// Sensors data values
bool data_light;
bool data_flame;
double data_temperature;
double data_apparent_temperature;
double data_humidity;
long rssi;

// actuators values;
double ac_temp;
String ac_mode;
String ac_previous_state;

// Functions
// -------------------------------
void printWifiStatus();
void awakeConnection();
long connectToWiFi();
void connectToMQTTBroker();
void mqttMessageReceived(String &topic, String &payload);
String clearMacAddress(String mac_address);
void sendMqttDouble(String attribute, double value);
void sendMqttLong(String attribute, long value);
void sendMqttBool(String attribute, bool value);
void acAutoControl();

// CODE
void setup()
{
  // Sync Serial logs
  Serial.begin(115200);

  // Init PINs
  pinMode(LED1, OUTPUT); // Define LED 1 output pin
  pinMode(LED2, OUTPUT); // Define LED 2 output pin
  pinMode(FLAME, INPUT); // Define FLAME input pin

  // Init actuators
  pinMode(LIGHT, OUTPUT);
  pinMode(AC_R, OUTPUT);
  pinMode(AC_G, OUTPUT);
  pinMode(AC_B, OUTPUT);

  // Turn actuators off
  digitalWrite(LIGHT, LOW);
  digitalWrite(AC_R, LOW);
  digitalWrite(AC_G, LOW);
  digitalWrite(AC_B, LOW);

  // Turn LEDs OFF
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);

  // Start DHT
  dht.begin();

  // Start MQTT
  mqttClient.begin(MQTT_BROKERIP, 1883, networkClient); // setup communication with MQTT broker
  mqttClient.onMessage(mqttMessageReceived);            // callback on message received from MQTT broker

  // Start WiFi
  WiFi.mode(WIFI_STA);

  clean_mac_address = clearMacAddress(String(WiFi.macAddress()));
  mqtt_topic_status = mqtt_topic_status + clean_mac_address;

  DynamicJsonDocument doc_will(128);
  doc_will["connected"] = false;
  char buffer_will[128];
  serializeJson(doc_will, buffer_will);
  const char *topic_status = mqtt_topic_status.c_str();
  mqttClient.setWill(topic_status, buffer_will, true, 1);
  mqttClient.setKeepAlive(LOG_DELAY/1000 + 2);
  mqttClient.setCleanSession(false);

#ifdef DEBUG
  Serial.println("Setup completed!");
#endif

  // Init delay
  delay(2000);
}

bool sent_setup = false;
unsigned long last_log_time = 0;
unsigned long last_control_time = 0;
bool wifi_awake = false;

void loop()
{
  if (!sent_setup)
  {
    connectToWiFi(); // connect to WiFi (if not already connected)
    light_control_topic = control_topic + clean_mac_address + "/light";
    ac_control_topic = control_topic + clean_mac_address + "/ac";
    connectToMQTTBroker(); // connect to MQTT broker (if not already connected)
    DynamicJsonDocument doc(256);
    doc["mac_address"] = clean_mac_address;
    doc["type"] = "sensors";
    doc["name"] = "sensors1";
    char buffer[256];
    size_t n = serializeJson(doc, buffer);

#ifdef DEBUG
    Serial.print(F("JSON setup message: "));
    Serial.println(buffer);
#endif
    if (mqttClient.publish(MQTT_TOPIC_SETUP, buffer, n, false, 1))
      sent_setup = true;
  }
  else
  {
    // Send flame data if status changed
    int fire = digitalRead(FLAME);
    if (fire == HIGH && !data_flame)
    {

#ifdef DEBUG
      Serial.println("Fire! Fire!");
#endif
      data_flame = true;
      String attribute = "flame";

      if (!wifi_awake)
      {
        awakeConnection();
      }

      sendMqttBool(attribute, data_flame);
    }
    else if (fire == LOW && data_flame)
    {

#ifdef DEBUG
      Serial.println("No more fire!");
#endif
      data_flame = false;

      String attribute = "flame";
      if (!wifi_awake)
      {
        awakeConnection();
      }
      sendMqttBool(attribute, data_flame);
    }

    currentTime = millis();

    // Check incoming mqtt controls
    if (currentTime - last_control_time > MQTT_CONTROL_DELAY)
    {
#ifdef DEBUG
      Serial.println("MQTT CONTROL LOOP");
#endif
      if (!wifi_awake)
      {
        awakeConnection();
      }

      if (!mqttClient.loop())
      {
#ifdef DEBUG
        Serial.println(mqttClient.lastError());
#endif
        mqttClient.disconnect();
      }
      last_control_time = currentTime;
    }

    // automatic AC control
    if (ac_mode == "auto" && (currentTime - lastAcControl > AC_CONTROL_DELAY))
    {
      if (!temp_read)
      {
        double t = dht.readTemperature(); // temperature Celsius, range 0-50°C (±2°C accuracy)

        if (isnan(t))
        { // readings failed, skip
          Serial.println(F("Failed to read from DHT sensor!"));
          return;
        }
        data_temperature = t;
      }
      lastAcControl = currentTime;
      acAutoControl();
    }

    // Send data periodically
    if (currentTime - last_log_time > LOG_DELAY)
    {
#ifdef DEBUG
      Serial.println("LOG LOOP");
#endif
      last_log_time = currentTime;
      if (!wifi_awake)
      {
        awakeConnection();
      }

      String attribute = "";

      // log RSSI
      attribute = "rssi";
      sendMqttLong(attribute, rssi);

      // log LIGHT
      unsigned int lightSensorValue;
      lastLightLogTime = currentTime;
      lightSensorValue = analogRead(PHOTORESISTOR); // read analog value (range 0-1023)
      if (lightSensorValue >= PHOTORESISTOR_THRESHOLD)
      {
        data_light = true;
      }
      else
      {
        data_light = false;
      }
      attribute = "light";
      sendMqttBool(attribute, data_light);

      // log TEMP/HUM

      double h = dht.readHumidity();
      double t = dht.readTemperature();

      if (isnan(h) || isnan(t))
      { // readings failed, skip
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }

      temp_read = true;
      double hic = dht.computeHeatIndex(t, h, false);

      data_humidity = h;
      data_temperature = t;
      data_apparent_temperature = hic;

#ifdef DEBUG
      Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.print(F("%  Temperature: "));
      Serial.print(t);
      Serial.print(F("°C  Apparent temperature: ")); // the temperature perceived by humans (takes into account humidity)
      Serial.print(hic);
      Serial.println(F("°C"));
#endif

      attribute = "humidity";
      sendMqttDouble(attribute, data_humidity);
      attribute = "temperature";
      sendMqttDouble(attribute, data_temperature);
      attribute = "apparent_temperature";
      sendMqttDouble(attribute, data_apparent_temperature);
    }

    // send modem to sleep if awake
    if (wifi_awake)
    {
      WiFi.mode(WIFI_OFF);
      WiFi.forceSleepBegin();
      wifi_awake = false;
    }
  }
}

// Functions
// -------------------------------
void printWifiStatus()
{
  Serial.println(F("\n=== WiFi connection status ==="));

  // SSID
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // signal strength
  Serial.print(F("Signal strength (RSSI): "));
  Serial.print(WiFi.RSSI());
  Serial.println(F(" dBm"));

  // current IP
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());

  // subnet mask
  Serial.print(F("Subnet mask: "));
  Serial.println(WiFi.subnetMask());

  // gateway
  Serial.print(F("Gateway IP: "));
  Serial.println(WiFi.gatewayIP());

  // DNS
  Serial.print(F("DNS IP: "));
  Serial.println(WiFi.dnsIP());

  Serial.println(F("==============================\n"));
}

void awakeConnection()
{
  WiFi.forceSleepWake();
  delay(1);
  rssi = connectToWiFi();
  connectToMQTTBroker();
  wifi_awake = true;
}

long connectToWiFi()
{
  long rssi_strength;
  // connect to WiFi (if not already connected)
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(F("Connecting to SSID: "));
    Serial.println(ssid);

#ifdef IP
    WiFi.config(ip, dns, gateway, subnet); // by default network is configured using DHCP
#endif

#ifdef DEBUG
    Serial.print(F("Connecting"));
#endif
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
#ifdef DEBUG
      Serial.print(F("."));
#endif
      delay(150);
    }
#ifdef DEBUG
    Serial.println(F("\nConnected!"));
#endif

    rssi_strength = WiFi.RSSI(); // get wifi signal strength

#ifdef DEBUG
    printWifiStatus();
#endif
  }
  else
  {
    rssi_strength = WiFi.RSSI(); // get wifi signal strength
  }

  return rssi_strength;
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
      Serial.print(F("."));
      delay(150);
    }

#ifdef DEBUG
    Serial.println(F("\nConnected!"));
#endif

    mqttClient.subscribe(light_control_topic, 1);
    mqttClient.subscribe(ac_control_topic, 1);
#ifdef DEBUG
    Serial.println("Subscribed to " + light_control_topic + "topic");
    Serial.println("Subscribed to " + ac_control_topic + "topic");
#endif

    DynamicJsonDocument doc_stat(128);
    doc_stat["connected"] = true;
    char buffer_stat[128];
    size_t n = serializeJson(doc_stat, buffer_stat);
    const char *topic_status = mqtt_topic_status.c_str();
    mqttClient.publish(topic_status, buffer_stat, n, true, 1);
  }
}

void mqttMessageReceived(String &topic, String &payload)
{
// This function handles a message from the MQTT broker
#ifdef DEBUG
  Serial.println("Incoming MQTT message: " + topic + " - " + payload);
#endif
  if (topic == light_control_topic)
  {

    StaticJsonDocument<128> doc;
    deserializeJson(doc, payload);
    String light_control = doc["control"];

    if (light_control == "on")
    {
      digitalWrite(LIGHT, HIGH);
#ifdef DEBUG
      Serial.println("Light on");
#endif
      return;
    }
    else if (light_control == "off")
    {
      digitalWrite(LIGHT, LOW);
#ifdef DEBUG
      Serial.println("Light off");
#endif
      return;
    }
    else
    {
#ifdef DEBUG
      Serial.println("Unrecognized light command!");
#endif
      return;
    }
  }
  if (topic == ac_control_topic)
  {
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload);
    ac_mode = doc["control"].as<String>();
    ;
    if (ac_mode == "on")
    {
      digitalWrite(AC_R, LOW);
      digitalWrite(AC_G, HIGH);
      digitalWrite(AC_B, LOW);
#ifdef DEBUG
      Serial.println("AC on");
#endif
      return;
    }
    else if (ac_mode == "off")
    {
      digitalWrite(AC_R, HIGH);
      digitalWrite(AC_G, LOW);
      digitalWrite(AC_B, LOW);
#ifdef DEBUG
      Serial.println("AC off");
#endif
      return;
    }
    else if (ac_mode == "auto")
    {
      ac_temp = doc["temp"];
#ifdef DEBUG
      Serial.println("AC auto");
      Serial.printf("Actual temperature: %f \n", data_temperature);
      Serial.printf("Desired temperature: %f \n", ac_temp);
#endif
      return;
    }
    else
    {
#ifdef DEBUG
      Serial.println("Unrecognized AC command");
#endif
      return;
    }
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

void sendMqttDouble(String attribute, double value)
{
  // Send data to MQTT
  DynamicJsonDocument doc(128);
  doc["value"] = value;
  char buffer[128];
  size_t n = serializeJson(doc, buffer);
  String topic = sensors_topic + clean_mac_address + "/" + attribute;
  const char *topic_c = topic.c_str();
  bool sent = false;
  if (mqttClient.publish(topic_c, buffer, n, true, 1))
    sent = true;
#ifdef DEBUG
  Serial.println(topic);
  Serial.print(F("JSON message: "));
  Serial.println(buffer);
  if (sent)
    Serial.println("Send OK");
  else
    Serial.println("Send NOT OK");
#endif
}

void sendMqttLong(String attribute, long value)
{
  // Send data to MQTT
  DynamicJsonDocument doc(128);
  doc["value"] = value;
  char buffer[128];
  size_t n = serializeJson(doc, buffer);
  String topic = sensors_topic + clean_mac_address + "/" + attribute;
  const char *topic_c = topic.c_str();
  bool sent = false;
  if (mqttClient.publish(topic_c, buffer, n, true, 1))
    sent = true;
#ifdef DEBUG
  Serial.println(topic);
  Serial.print(F("JSON message: "));
  Serial.println(buffer);
  if (sent)
    Serial.println("Send OK");
  else
    Serial.println("Send NOT OK");
#endif
}

void sendMqttBool(String attribute, bool value)
{
  // Send data to MQTT
  DynamicJsonDocument doc(128);
  doc["value"] = value;
  char buffer[128];
  size_t n = serializeJson(doc, buffer);
  String topic = sensors_topic + clean_mac_address + "/" + attribute;
  const char *topic_c = topic.c_str();
  bool sent = false;
  if (mqttClient.publish(topic_c, buffer, n, true, 1))
    sent = true;
#ifdef DEBUG
  Serial.println(topic);
  Serial.print(F("JSON message: "));
  Serial.println(buffer);
  if (sent)
    Serial.println("Send OK");
  else
    Serial.println("Send NOT OK");
#endif
}

void acAutoControl()
{
  String ac_current_state;
  if (data_temperature >= ac_temp)
  {
    ac_current_state = "on";
  }
  else
  {
    ac_current_state = "off";
  }

  if (ac_current_state != ac_previous_state)
  {
    ac_previous_state = ac_current_state;

    if (ac_current_state == "on")
    {
      digitalWrite(AC_R, LOW);
      digitalWrite(AC_G, LOW);
      digitalWrite(AC_B, HIGH);
#ifdef DEBUG
      Serial.println("High temp, turn AC on");
#endif
    }
    else
    {
      digitalWrite(AC_R, HIGH);
      digitalWrite(AC_G, LOW);
      digitalWrite(AC_B, HIGH);
#ifdef DEBUG
      Serial.println("Low temp, turn AC off");
#endif
    }
  }
}