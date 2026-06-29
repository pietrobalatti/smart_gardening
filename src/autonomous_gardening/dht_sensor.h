#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "DHT.h"
#include <LittleFS.h>
#include "soil_moisture_sensor.h"

// Sensor configuration
#define DHTPIN 2        // GPIO2 = D4 on NodeMCU
#define DHTTYPE DHT22

// Maximum number of history entries
#define MAX_HISTORY 48

struct DataPoint {
  time_t timestamp;
  float temperature;
  float humidity;
  float soilMoisture;
  float pump1WateringMinutes;
  float pump2WateringMinutes;
};

DataPoint history[MAX_HISTORY];
int historyCount = 0;
int historyIndex = 0;

// DHT object
extern DHT dht;

// Sensor values
extern float temperature;
extern float humidity;


DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float humidity = 0.0;

void setupDHT() {
  dht.begin();
}

void updateDHT() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;
}

void addHistoryReading(time_t timestamp, float temp, float hum, float soil, float pump1WateringMinutes, float pump2WateringMinutes)
{
  history[historyIndex].timestamp = timestamp;
  history[historyIndex].temperature = temp;
  history[historyIndex].humidity = hum;
  history[historyIndex].soilMoisture = soil;
  history[historyIndex].pump1WateringMinutes = pump1WateringMinutes;
  history[historyIndex].pump2WateringMinutes = pump2WateringMinutes;

  historyIndex = (historyIndex + 1) % MAX_HISTORY;
  if (historyCount < MAX_HISTORY) historyCount++;
}

void loadSensorHistory() {
  File file = LittleFS.open("/history.txt", "r");
  if (!file) return;

  historyCount = 0;
  historyIndex = 0;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int idx1 = line.indexOf(',');
    int idx2 = line.indexOf(',', idx1 + 1);
    int idx3 = line.indexOf(',', idx2 + 1);
    int idx4 = line.indexOf(',', idx3 + 1);
    int idx5 = (idx4 < 0) ? -1 : line.indexOf(',', idx4 + 1);
    if (idx1 < 0 || idx2 < 0 || idx3 < 0) continue;

    time_t timestamp = (time_t)line.substring(0, idx1).toInt();
    float temp = line.substring(idx1 + 1, idx2).toFloat();
    float hum = line.substring(idx2 + 1, idx3).toFloat();
    float soil = (idx4 < 0)
      ? line.substring(idx3 + 1).toFloat()
      : line.substring(idx3 + 1, idx4).toFloat();
    float pump1WateringMinutes = (idx4 < 0)
      ? 0.0
      : ((idx5 < 0) ? line.substring(idx4 + 1).toFloat() : line.substring(idx4 + 1, idx5).toFloat());
    float pump2WateringMinutes = (idx5 < 0) ? 0.0 : line.substring(idx5 + 1).toFloat();

    addHistoryReading(timestamp, temp, hum, soil, pump1WateringMinutes, pump2WateringMinutes);
  }

  file.close();
}

void logDHTReading(float pump1WateringMinutes = 0.0, float pump2WateringMinutes = 0.0) {
  time_t now = time(nullptr);

  addHistoryReading(now, temperature, humidity, soilMoisture, pump1WateringMinutes, pump2WateringMinutes);

  // Write entire buffer to file
  File file = LittleFS.open("/history.txt", "w");
  if (!file) return;

  int start = (historyIndex + MAX_HISTORY - historyCount) % MAX_HISTORY;
  for (int i = 0; i < historyCount; i++) {
    int idx = (start + i) % MAX_HISTORY;
    file.printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f\n",
      history[idx].timestamp,
      history[idx].temperature,
      history[idx].humidity,
      history[idx].soilMoisture,
      history[idx].pump1WateringMinutes,
      history[idx].pump2WateringMinutes
    );
  }
  file.close();
}


#endif
