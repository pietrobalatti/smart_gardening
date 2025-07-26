#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "DHT.h"
#include <LittleFS.h>

// Sensor configuration
#define DHTPIN 2        // GPIO2 = D4 on NodeMCU
#define DHTTYPE DHT22

// Maximum number of history entries
#define MAX_HISTORY 48

struct DataPoint {
  time_t timestamp;
  float temperature;
  float humidity;
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

void logDHTReading() {
  time_t now = time(nullptr);

  // Save in circular buffer
  history[historyIndex].timestamp = now;
  history[historyIndex].temperature = temperature;
  history[historyIndex].humidity = humidity;

  historyIndex = (historyIndex + 1) % MAX_HISTORY;
  if (historyCount < MAX_HISTORY) historyCount++;

  // Write entire buffer to file
  File file = LittleFS.open("/history.txt", "w");
  if (!file) return;

  int start = (historyIndex + MAX_HISTORY - historyCount) % MAX_HISTORY;
  for (int i = 0; i < historyCount; i++) {
    int idx = (start + i) % MAX_HISTORY;
    file.printf("%lu,%.2f,%.2f\n",
      history[idx].timestamp,
      history[idx].temperature,
      history[idx].humidity
    );
  }
  file.close();
}


#endif
