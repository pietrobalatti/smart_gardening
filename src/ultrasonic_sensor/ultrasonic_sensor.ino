#include <NewPing.h>

#define TRIG_PIN 2
#define ECHO_PIN 0
#define MAX_DISTANCE 200  // cm

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

void setup() {
  Serial.begin(9600);
}

void loop() {
  delay(50);
  int distance = sonar.ping_cm();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
}
