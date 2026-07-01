# Hardware

 |__NodeMCU__|__S/M sensor 1__|__S/M sensor 2__|
 |-----------|----------------|----------------|
 |    D0     |      Vcc       |                |
 |    D1     |                |      Vcc       |
 |    A0     |      Aout      |      Aout      |
 |    Gnd    |      Gnd       |      Gnd       |

Only one Vcc pin is set HIGH while reading. If both sensor Aout pins are connected to A0, use a resistor in series with each Aout line to reduce backfeeding between the modules.
 
# Software
```c
#define SENSOR_PIN A0

const int AirValue = 803;   //you need to replace this value with analog value when the sensor is in air
const int WaterValue = 327;  //you need to replace this value with analog value when the sensor is in water

void setup() {
  Serial.begin(9600);
}

void loop() {
  int rawValue = analogRead(SENSOR_PIN); // Range 0–1023 (scaled)
  float moisturePercent = map(rawValue, AirValue, WaterValue, 0, 100);

  if(moisturePercent > 100)
  {
    moisturePercent = 100; // Cap at 100%
  }else if(moisturePercent <0)
  {
    moisturePercent = 0; // Floor at 0%
  }
  // Print the raw value and the calculated moisture percentage
  Serial.print("Analog Value: ");
  Serial.print(rawValue);
  Serial.print("  |  Soil Moisture: ");
  Serial.print(moisturePercent);
  Serial.println(" %");

  delay(2000);
}

```

## Calibration
- Insert sensor in dry soil → note the analog value (e.g. ~700–800)
- Insert in wet soil → note the value (e.g. ~300–400)

Use these in your map() range: map(raw, dry_val, wet_val, 0, 100)

_Higher value = dryer soil (sensor has higher resistance)_


# Reference
https://how2electronics.com/capacitive-soil-moisture-sensor-esp8266-esp32-oled-display/  
<!-- ChatGPT: https://chatgpt.com/c/6842fb43-4c1c-800b-ac62-14043a477519 -->
