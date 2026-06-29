#ifndef SOIL_MOISTURE_SENSOR_H
#define SOIL_MOISTURE_SENSOR_H

// Soil moisture sensor configuration
#define SOIL_MOISTURE_PIN A0

const int SoilAirValue = 788;
const int SoilWaterValue = 315;

extern int soilMoistureRaw;
extern float soilMoisture;

int soilMoistureRaw = 0;
float soilMoisture = 0.0;

float calculateSoilMoisturePercent(int rawValue) {
  float range = (float)(SoilAirValue - SoilWaterValue);
  if (range == 0.0) return 0.0;

  float percent = 100.0 * ((float)SoilAirValue - (float)rawValue) / range;
  if (percent > 100.0) return 100.0;
  if (percent < 0.0) return 0.0;
  return percent;
}

void updateSoilMoisture() {
  soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);
  soilMoisture = calculateSoilMoisturePercent(soilMoistureRaw);
}

void setupSoilMoisture() {
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  updateSoilMoisture();
}

#endif
