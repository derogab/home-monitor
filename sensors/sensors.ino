#define ALARM D1 // D1
#define FLAME D2 // D2

void setup() {
  Serial.begin(115200);
  pinMode(FLAME, INPUT); // Define FLAME input pin
  pinMode(ALARM, OUTPUT); // Define ALARM output pin

}

void loop() {

  int fire = digitalRead(FLAME); // Read FLAME sensor
  
  if(fire == HIGH) {
    Serial.println("Fire! Fire!");
    digitalWrite(ALARM, HIGH);   // Set the buzzer on by making the voltage LOW
    delay(1000);                // Wait for a second
    digitalWrite(ALARM, LOW);  // Set the buzzer off
  } else {
    Serial.println("No fire detected :)");
    digitalWrite(ALARM, LOW);  // Set the buzzer off
  }

  delay(1000);
}
