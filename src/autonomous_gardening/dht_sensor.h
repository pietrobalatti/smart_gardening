#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "DHT.h"
#include <LittleFS.h>

// Sensor configuration
#define DHTPIN 2        // GPIO2 = D4 on NodeMCU
#define DHTTYPE DHT22

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
  File file = LittleFS.open("/history.txt", "a");
  if (!file) return;

  // unsigned long timestamp = millis(); // or get time from NTP
  // file.printf("%lu,%.2f,%.2f\n", timestamp, temperature, humidity);
  time_t now = time(nullptr);  // Get UNIX time
  file.printf("%lu,%.2f,%.2f\n", now, temperature, humidity);

  file.close();
}

#endif
