# Hardware

 |__NodeMCU__|__HC-SR04__|
 |-----------|-----------|
 |    Vin    |    Vcc    | --> It needs 5V, not 3.3V as described by another instructable
 |    D4     |    Trig   |
 |    D3     |    Echo   |
 |    Gnd    |    Gnd    |
 
# Software
```c
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
```

## Requirement
Install NewPing Library (via Library Manager)
- Open the Arduino IDE.
- Go to Sketch > Include Library > Manage Libraries...
- In the Library Manager, type NewPing in the search bar.
- Look for NewPing by Tim Eckel and click Install.

# Reference
https://www.instructables.com/Distance-Measurement-Using-HC-SR04-Via-NodeMCU/
https://randomnerdtutorials.com/esp8266-nodemcu-hc-sr04-ultrasonic-arduino/
