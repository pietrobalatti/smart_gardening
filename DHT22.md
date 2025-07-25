# Hardware

 |__NodeMCU__|__DHT22__ |
 |-----------|----------|
 |    Vin    |    Vcc   | --> It needs 3.3V or 5V
 |    D4     |    Out   |
 |    Gnd    |    Gnd   |
 
# Software
```c
#include "DHT.h"

#define DHTPIN 2       // Pin D4 on NodeMCU connected to DHT11 OUT
#define DHTTYPE DHT22  // DHT22 sensor type

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  delay(3000); // wait 3 seconds before next reading
}

```

## Requirement
Install DHT sensor Library (via Library Manager)
- Open the Arduino IDE.
- Go to Sketch > Include Library > Manage Libraries...
- In the Library Manager, type "DHT sensor library" in the search bar.
- Look for DHT sensor library by Rob Tillaart and click Install.

# Reference
https://github.com/adafruit/DHT-sensor-library/blob/master/examples/DHTtester/DHTtester.ino
<!-- ChatGPT: https://chatgpt.com/c/6842fb43-4c1c-800b-ac62-14043a477519 -->