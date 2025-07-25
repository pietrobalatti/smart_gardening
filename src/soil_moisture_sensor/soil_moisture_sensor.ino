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
