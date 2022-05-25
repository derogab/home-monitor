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
#define DEBUG true

// Sensors
// --------------
// Buildi-In LEDs
#define LED1 D0
#define LED2 D4
// Flame Detector
#define FLAME D1
#define FLAME_LOG_DELAY 30000
#define FLAME_CHANGE_DELAY 2500
// DHT11 - Temperature & Humidity Sensor
#define DHT_PIN D2
#define DHT_TYPE DHT11 // Sensor type: DHT 11
#define DHT_DELAY 60000 // Needed delay for DHT sensors (Warning! Min = 2000)
// Photoresistor
#define PHOTORESISTOR A0              // photoresistor pin
#define PHOTORESISTOR_THRESHOLD 900   // turn led on for light values lesser than this
#define PHOTORESISTOR_LOG_DELAY 5000
#define PHOTORESISTOR_CHANGE_DELAY 5000
// WiFi signal
#define RSSI_THRESHOLD -60            // WiFi signal strength threshold
#define RSSI_LOG_DELAY 2500


#define SETUP_LOG_DELAY 60000


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
unsigned long lastLightChangeTime = 0;
// Initialize flame log time
unsigned long lastFlameLogTime = 0;
unsigned long lastFlameChangeTime = 0;
// Initialize rssi log time
unsigned long lastRssiLog = 0;
// Initialize DHT sensor
DHT dht = DHT(DHT_PIN, DHT_TYPE);


// Configs
// --------------
// WiFi config
char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password
#ifdef IP
IPAddress ip(IP);
IPAddress subnet(SUBNET);
IPAddress dns(DNS);
IPAddress gateway(GATEWAY);
#endif
// MQTT cfg
#define MQTT_BUFFER_SIZE 1024               // the maximum size for packets being published and received
MQTTClient mqttClient(MQTT_BUFFER_SIZE);   // handles the MQTT communication protocol
WiFiClient networkClient;                  // handles the network connection to the MQTT broker

// Globals
// --------------
// Sensors data values
bool data_light;
bool data_flame;
double data_temperature;
double data_apparent_temperature;
double data_humidity;


// CODE

void setup() {
  // Sync Serial logs
  Serial.begin(115200);
  Serial.println("Setup...");
  
  // Init PINs
  pinMode(LED1, OUTPUT); // Define LED 1 output pin
  pinMode(LED2, OUTPUT); // Define LED 2 output pin
  pinMode(FLAME, INPUT); // Define FLAME input pin

  // Turn LEDs OFF
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);

  // Start DHT
  dht.begin();

  // Start MQTT
  mqttClient.begin(MQTT_BROKERIP, 1883, networkClient);   // setup communication with MQTT broker
  mqttClient.onMessage(mqttMessageReceived);              // callback on message received from MQTT broker

  // Start WiFi
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  #ifdef DEBUG
    Serial.println("Setup completed!");
  #endif

  // Init delay
  delay(2000);
}

void loop() {
  
  // Get millis time
  currentTime = millis();

  
  // WIFI
  // -----------------------------
  // Init variables
  int static init_db = 0;
  bool static led_status = HIGH; // Default: OFF
  // Connect to WiFi
  long rssi = connectToWiFi();   // WiFi connect if not established and if connected get wifi signal strength
  
  if ((rssi > RSSI_THRESHOLD) && (led_status)) {   // if wifi signal strength is high then keep led on
    led_status = LOW;
    digitalWrite(LED2, led_status);
  } else if ((rssi <= RSSI_THRESHOLD) && (!led_status)) {   // if wifi signal strength is high then keep led off
    led_status = HIGH;
    digitalWrite(LED2, led_status);
  }

  // MQTT
  // -----------------------------
  // Check MQTT connection
  connectToMQTTBroker();   // Connect to MQTT broker (if not already connected)
  // Exec MQTT client loop & check
  if(!mqttClient.loop()){
    Serial.print(F("MQTT Error: "));  
    Serial.println(mqttClient.lastError());
  }

  // Send to MQTT
  // Check if frequency is good :)
  if (currentTime - lastSetupTime > SETUP_LOG_DELAY) {

    // Update reading time
    lastSetupTime = currentTime;

    // Send Setup
    sendSetup("test_name");
  }
  
  // Send to MQTT
  // Check if frequency is good :)
  if (currentTime - lastRssiLog > RSSI_LOG_DELAY) {

    // Update reading time
    lastRssiLog = currentTime;

    // Send to MQTT
    sendNumericDataToMQTT("rssi", (double)rssi);
  }
  

  // PHOTORESISTOR
  // -----------------------------
  static unsigned int lightSensorValue;

  lightSensorValue = analogRead(PHOTORESISTOR);   // read analog value (range 0-1023)

  if (lightSensorValue >= PHOTORESISTOR_THRESHOLD) {   // high brightness
    // Set the LED off
    digitalWrite(LED1, HIGH);
    // Set status on memory
    data_light = true;
    // Update reading time
    lastLightChangeTime = currentTime;
  } else {                                             // low brightness
    // Set the LED on
    digitalWrite(LED1, LOW);
    // Set status on memory
    data_light = false;
    // Update reading time
    lastLightChangeTime = currentTime;
  }

  // Re-send LIGHT data sometimes :)
  if (currentTime - lastLightLogTime > PHOTORESISTOR_LOG_DELAY) {

    // Update reading time
    lastLightLogTime = currentTime;
    // Send data to MQTT
    sendBooleanDataToMQTT("light", data_light);

    #ifdef DEBUG
      // Logs
      Serial.print(F("Light sensor value: "));
      Serial.println(lightSensorValue);
    #endif
  }


  // FLAME DETECTION
  // -----------------------------
  // Read digital PIN of Flame Sensor
  int fire = digitalRead(FLAME); // Read FLAME sensor
  // Sent fast data only each some seconds
  if(currentTime - lastFlameChangeTime > FLAME_CHANGE_DELAY) {

    // Check flame level 
    if(fire == HIGH && !data_flame) {
  
      #ifdef DEBUG
        Serial.println("Fire! Fire!");
      #endif
      
      // Send flame data
      sendBooleanDataToMQTT("flame", true);
      // Save flame data
      data_flame = true;
      // Update reading time
      lastFlameChangeTime = currentTime;
    
    } else if (data_flame) { // only if previous is flame!
  
      #ifdef DEBUG
        Serial.println("No more fire!");
      #endif
  
      // Send no-more-flame data
      sendBooleanDataToMQTT("flame", false);
      // Save no-more-flame data
      data_flame = false;
      // Update reading time
      lastFlameChangeTime = currentTime;
    }
    
  }

  // Re-send FLAME data sometimes :)
  if (currentTime - lastFlameLogTime > FLAME_LOG_DELAY) {

    // Update reading time
    lastFlameLogTime = currentTime;
    // Send data to MQTT
    sendBooleanDataToMQTT("flame", data_flame);
  }

  // TEMPERATURE & HUMIDITY DETECTION
  // -------------------------------

  // Check if frequency is good :)
  if (currentTime - lastTempTime > DHT_DELAY) {

    // Update reading time
    lastTempTime = currentTime;

    // reading temperature or humidity takes about 250 milliseconds!
    double h = dht.readHumidity();      // humidity percentage, range 20-80% (±5% accuracy)
    double t = dht.readTemperature();   // temperature Celsius, range 0-50°C (±2°C accuracy)

    if (isnan(h) || isnan(t)) {   // readings failed, skip
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    // compute heat index in Celsius (isFahreheit = false)
    double hic = dht.computeHeatIndex(t, h, false);

    data_humidity = h;
    data_temperature = t;
    data_apparent_temperature = hic;

    #ifdef DEBUG
      Serial.print(F("Humidity: "));
      Serial.print(h);   
      Serial.print(F("%  Temperature: "));
      Serial.print(t);
      Serial.print(F("°C  Apparent temperature: "));   // the temperature perceived by humans (takes into account humidity)
      Serial.print(hic);
      Serial.println(F("°C"));
    #endif

    // Send data to MQTT
    sendNumericDataToMQTT("humidity", h);
    //delay(500);
    sendNumericDataToMQTT("temperature", t);
    //delay(500);
    sendNumericDataToMQTT("apparent_temperature", hic);
    
  }

}

// Functons
// -------------------------------
void printWifiStatus() {
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

long connectToWiFi() {
  long rssi_strength;
  // connect to WiFi (if not already connected)
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("Connecting to SSID: "));
    Serial.println(ssid);

#ifdef IP
    WiFi.config(ip, dns, gateway, subnet);   // by default network is configured using DHCP
#endif

    #ifdef DEBUG
      Serial.print(F("Connecting"));
    #endif
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      #ifdef DEBUG
        Serial.print(F("."));
      #endif
      delay(150);
    }
    #ifdef DEBUG
      Serial.println(F("\nConnected!"));
    #endif
    
    rssi_strength = WiFi.RSSI();   // get wifi signal strength
    
    #ifdef DEBUG
      printWifiStatus();
    #endif
  
  } else {
    rssi_strength = WiFi.RSSI();   // get wifi signal strength
  }

  return rssi_strength;
}

void connectToMQTTBroker() {
  if (!mqttClient.connected()) {   // not connected

    #ifdef DEBUG
      Serial.print(F("\nConnecting to MQTT broker..."));
    #endif
    
    while (!mqttClient.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.print(F("."));
      delay(150);
    }
    
    #ifdef DEBUG
      Serial.println(F("\nConnected!"));
    #endif

    // connected to broker, subscribe topics
    mqttClient.subscribe("example");
    
    #ifdef DEBUG
      Serial.println(F("\nSubscribed to EXAMPLE topic!"));
    #endif
  }
}

void mqttMessageReceived(String &topic, String &payload) {
  // This function handles a message from the MQTT broker
  #ifdef DEBUG
    Serial.println("Incoming MQTT message: " + topic + " - " + payload);
  #endif
  
  if (topic == "example") {
    // deserialize the JSON object
    //StaticJsonDocument<128> doc;
    //deserializeJson(doc, payload);
    //const char *desiredLedStatus = doc["status"];

    Serial.println(F("Do something here."));
  }
}

void sendNumericDataToMQTT(String attribute, double value) {
  
  // Publish new MQTT data (as a JSON object)
  const int capacity = JSON_OBJECT_SIZE(1);
  StaticJsonDocument<capacity> doc;
  doc["value"] = value;
  char buffer[128];
  size_t n = serializeJson(doc, buffer);

  String topic = "unishare/sensors/"+clearMacAddress(String(WiFi.macAddress()))+"/"+attribute;

  #ifdef DEBUG
    Serial.print(F("JSON to "));
    Serial.print(topic);
    Serial.print(F(":"));
    Serial.print(buffer);
  #endif
  
  int topic_len = topic.length() + 1; 
  char topic_c[topic_len];
  topic.toCharArray(topic_c, topic_len);
  
  bool result = mqttClient.publish(topic_c, buffer, n);

  #ifdef DEBUG
    if (bool) Serial.println(F(" ....OK!")); else Serial.println(F(" ....FAIL!"));
  #endif
  
}

void sendBooleanDataToMQTT(String attribute, bool value) {

  // Publish new MQTT data (as a JSON object)
  const int capacity = JSON_OBJECT_SIZE(1);
  StaticJsonDocument<capacity> doc;
  doc["value"] = value;
  char buffer[128];
  size_t n = serializeJson(doc, buffer);

  String topic = "unishare/sensors/"+clearMacAddress(String(WiFi.macAddress()))+"/"+attribute;

  #ifdef DEBUG
    Serial.print(F("JSON to "));
    Serial.print(topic);
    Serial.print(F(" : "));
    Serial.print(buffer);
  #endif
  
  int topic_len = topic.length() + 1; 
  char topic_c[topic_len];
  topic.toCharArray(topic_c, topic_len);
  
  mqttClient.publish(topic_c, buffer, n);

  #ifdef DEBUG
    Serial.println(F(" ....OK!"));
  #endif
}

void sendSetup(String device_name) {
  
  // Publish new MQTT data (as a JSON object)
  DynamicJsonDocument doc(1024);
  doc["mac_address"] = clearMacAddress(String(WiFi.macAddress()));
  doc["name"] = device_name;
  doc["type"] = "sensors";
  char buffer[1024];
  size_t n = serializeJson(doc, buffer);

  #ifdef DEBUG
    Serial.print(F("JSON to "));
    Serial.print("unishare/devices/setup");
    Serial.print(F(":"));
    Serial.print(buffer);
  #endif
  
  mqttClient.publish("unishare/devices/setup", buffer, n);

  #ifdef DEBUG
    Serial.println(F(" ....OK!"));
  #endif
  
}

String clearMacAddress(String mac_address) {
  // Prepare
  String to_replace = String(':');
  String replaced = "";
  // Exec
  mac_address.replace(to_replace, replaced);
  // Return
  return mac_address;
}
