// Include sensors libraries
// --------------
// Include DHT 11 library
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Sensors
// --------------
// Active Buzzer
#define ALARM D0
// Flame Detector
#define FLAME D1
// DHT11 - Temperature & Humidity Sensor
#define DHTPIN D2
#define DHTTYPE DHT11 // Sensor type: DHT 11

// Init
// --------------
// Initialize millis var
unsigned long currentTime;
// Initialize temperature & humidity time
unsigned long lastTempTime;
// Initialize DHT sensor
DHT dht = DHT(DHTPIN, DHTTYPE);

// CODE

void setup() {
  // Sync Serial logs
  Serial.begin(115200);

  // Init PINs
  pinMode(FLAME, INPUT); // Define FLAME input pin
  pinMode(ALARM, OUTPUT); // Define ALARM output pin

  // Start DHT
  dht.begin();

  // Init delay
  delay(2000);

}

void loop() {

  // Get millis time
  currentTime = millis();


  // FLAME ALARM
  // -----------------------------

  int fire = digitalRead(FLAME); // Read FLAME sensor
  
  if(fire == HIGH) {
    Serial.println("Fire! Fire!");
    digitalWrite(ALARM, LOW);   // Set the buzzer on by making the voltage LOW
    delay(500);                // Wait for a second
    digitalWrite(ALARM, HIGH);  // Set the buzzer off
  } else {
    digitalWrite(ALARM, HIGH);  // Set the buzzer off
  }

  // TEMPERATURE & HUMIDITY DETECTION

  // Temp Delay
  unsigned long tempDelay = 2000;
  // Check if frequency is good :)
  if (currentTime - lastTempTime > tempDelay) {

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


  // FINAL SLEEP
  // -----------------------------
  delay(250);
}
