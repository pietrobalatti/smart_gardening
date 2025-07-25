#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "DHT.h"

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



#endif
