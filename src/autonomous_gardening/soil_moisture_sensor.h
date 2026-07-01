#ifndef SOIL_MOISTURE_SENSOR_H
#define SOIL_MOISTURE_SENSOR_H

// Soil moisture sensor configuration
#define SOIL_MOISTURE_PIN A0
#define SOIL_MOISTURE_1_POWER_PIN 16 // GPIO16 = D0 on NodeMCU
#define SOIL_MOISTURE_2_POWER_PIN 5  // GPIO5 = D1 on NodeMCU

const int SoilAirValue = 730;
const int SoilWaterValue = 265;
const unsigned int SoilPowerUpDelayMs = 500;
const unsigned int SoilPowerDownDelayMs = 10;

extern int soilMoisture1Raw;
extern int soilMoisture2Raw;
extern float soilMoisture1;
extern float soilMoisture2;

int soilMoisture1Raw = 0;
int soilMoisture2Raw = 0;
float soilMoisture1 = 0.0;
float soilMoisture2 = 0.0;

float calculateSoilMoisturePercent(int rawValue) {
  float range = (float)(SoilAirValue - SoilWaterValue);
  if (range == 0.0) return 0.0;

  float percent = 100.0 * ((float)SoilAirValue - (float)rawValue) / range;
  if (percent > 100.0) return 100.0;
  if (percent < 0.0) return 0.0;
  return percent;
}

void powerOffSoilMoistureSensors() {
  digitalWrite(SOIL_MOISTURE_1_POWER_PIN, LOW);
  digitalWrite(SOIL_MOISTURE_2_POWER_PIN, LOW);
}

int readSoilMoistureRaw(int powerPin) {
  powerOffSoilMoistureSensors();
  delay(SoilPowerDownDelayMs);
  digitalWrite(powerPin, HIGH);
  delay(SoilPowerUpDelayMs);
  int rawValue = analogRead(SOIL_MOISTURE_PIN);
  digitalWrite(powerPin, LOW);
  delay(SoilPowerDownDelayMs);
  return rawValue;
}

void updateSoilMoisture() {
  soilMoisture1Raw = readSoilMoistureRaw(SOIL_MOISTURE_1_POWER_PIN);
  soilMoisture1 = calculateSoilMoisturePercent(soilMoisture1Raw);

  soilMoisture2Raw = readSoilMoistureRaw(SOIL_MOISTURE_2_POWER_PIN);
  soilMoisture2 = calculateSoilMoisturePercent(soilMoisture2Raw);
}

void setupSoilMoisture() {
  pinMode(SOIL_MOISTURE_1_POWER_PIN, OUTPUT);
  pinMode(SOIL_MOISTURE_2_POWER_PIN, OUTPUT);
  powerOffSoilMoistureSensors();
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  updateSoilMoisture();
}

#endif
