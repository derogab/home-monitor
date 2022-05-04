// Include sensors libraries
// --------------
// Include DHT 11 library
#include <Adafruit_Sensor.h>
#include <DHT.h>
// Include WIFi Library
#include <ESP8266WiFi.h>
// InfluxDB library
#include <InfluxDbClient.h>

// Include SECRETs
#include "secrets.h"

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
#define DHT_DELAY 10000 // Needed delay for DHT sensors
// Photoresistor
#define PHOTORESISTOR A0              // photoresistor pin
#define PHOTORESISTOR_THRESHOLD 900   // turn led on for light values lesser than this
// Active Buzzer
#define ALARM D3
// WiFi signal
#define RSSI_THRESHOLD -60            // WiFi signal strength threshold


// Init
// --------------
// Initialize millis var
unsigned long currentTime;
// Initialize temperature & humidity time
unsigned long lastTempTime;
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
WiFiClient client;
// InfluxDB cfg
InfluxDBClient client_idb(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point pointDevice("device_status");

// CODE

void setup() {
  // Sync Serial logs
  Serial.begin(115200);

  // Init PINs
  pinMode(LED1, OUTPUT); // Define LED 1 output pin
  pinMode(LED2, OUTPUT); // Define LED 2 output pin
  pinMode(FLAME, INPUT); // Define FLAME input pin
  pinMode(ALARM, OUTPUT); // Define ALARM output pin

  // Turn LEDs OFF
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);

  // Turn ALARM OFF
  digitalWrite(ALARM, HIGH);

  // Start DHT
  dht.begin();

  // Start WiFi
  WiFi.mode(WIFI_STA);

  // Init delay
  delay(DHT_DELAY);

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
  

  // PHOTORESISTOR
  // -----------------------------
  static unsigned int lightSensorValue;

  lightSensorValue = analogRead(PHOTORESISTOR);   // read analog value (range 0-1023)
  Serial.print(F("Light sensor value: "));
  Serial.println(lightSensorValue);

  if (lightSensorValue >= PHOTORESISTOR_THRESHOLD) {   // high brightness
    digitalWrite(LED1, HIGH);                          // LED off
  } else {                                             // low brightness
    digitalWrite(LED1, LOW);                           // LED on
  }


  // FLAME ALARM
  // -----------------------------

  int fire = digitalRead(FLAME); // Read FLAME sensor
  
  if(fire == HIGH) {
    Serial.println("Fire! Fire!");
    digitalWrite(ALARM, LOW);   // Set the buzzer on by making the voltage LOW
    delay(250);                 // Wait for a second
    digitalWrite(ALARM, HIGH);  // Set the buzzer off
  } else {
    digitalWrite(ALARM, HIGH);  // Set the buzzer off
  }

  // TEMPERATURE & HUMIDITY DETECTION
  // -------------------------------

  // Check if frequency is good :)
  if (currentTime - lastTempTime > DHT_DELAY) {

    // Update reading time
    lastTempTime = currentTime;

    // reading temperature or humidity takes about 250 milliseconds!
    float h = dht.readHumidity();      // humidity percentage, range 20-80% (±5% accuracy)
    float t = dht.readTemperature();   // temperature Celsius, range 0-50°C (±2°C accuracy)

    if (isnan(h) || isnan(t)) {   // readings failed, skip
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    // compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);

    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C  Apparent temperature: "));   // the temperature perceived by humans (takes into account humidity)
    Serial.print(hic);
    Serial.println(F("°C"));
    
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

    Serial.print("Connecting to Wifi");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(F("."));
      delay(250);
    }
    Serial.println(F("\nConnected!"));
    rssi_strength = WiFi.RSSI();   // get wifi signal strength
    printWifiStatus();
  } else {
    rssi_strength = WiFi.RSSI();   // get wifi signal strength
  }

  return rssi_strength;
}

void check_influxdb() {
  // Check server connection
  if (client_idb.validateConnection()) {
    Serial.print(F("Connected to InfluxDB: "));
    Serial.println(client_idb.getServerUrl());
  } else {
    Serial.print(F("InfluxDB connection failed: "));
    Serial.println(client_idb.getLastErrorMessage());
  }
}

int WriteMultiToDB(char ssid[], int rssi, int led_status) {
  int writing = 0;
  // Store measured value into point
  pointDevice.clearFields();
  // Report RSSI of currently connected network
  pointDevice.addField("rssi", rssi);
  pointDevice.addField("led_status", led_status);
  Serial.print(F("Writing: "));
  Serial.println(pointDevice.toLineProtocol());
  if (!client_idb.writePoint(pointDevice)) {
    Serial.print(F("InfluxDB write failed: "));
    Serial.println(client_idb.getLastErrorMessage());
    writing = 1;
  }

  Serial.println(F("Wait 2s"));
  delay(2000);
  return writing;
}
