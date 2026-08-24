#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "DHT.h"
#include <LittleFS.h>
#include "soil_moisture_sensor.h"

// Sensor configuration
#define DHTPIN 2        // GPIO2 = D4 on NodeMCU
#define DHTTYPE DHT22

// Maximum number of hourly history entries
#define MAX_HISTORY 48

struct DataPoint {
  time_t timestamp;
  float temperature;
  float humidity;
  float soilMoisture1;
  float soilMoisture2;
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

void addHistoryReading(time_t timestamp, float temp, float hum, float soil1, float soil2, float pump1WateringMinutes, float pump2WateringMinutes)
{
  history[historyIndex].timestamp = timestamp;
  history[historyIndex].temperature = temp;
  history[historyIndex].humidity = hum;
  history[historyIndex].soilMoisture1 = soil1;
  history[historyIndex].soilMoisture2 = soil2;
  history[historyIndex].pump1WateringMinutes = pump1WateringMinutes;
  history[historyIndex].pump2WateringMinutes = pump2WateringMinutes;

  historyIndex = (historyIndex + 1) % MAX_HISTORY;
  if (historyCount < MAX_HISTORY) historyCount++;
}

bool isDigitChar(char c)
{
  return c >= '0' && c <= '9';
}

bool isUnsignedIntegerField(String field)
{
  field.trim();
  if (field.length() == 0) return false;

  for (unsigned int i = 0; i < field.length(); i++) {
    if (!isDigitChar(field.charAt(i))) return false;
  }

  return true;
}

bool isDecimalField(String field)
{
  field.trim();
  if (field.length() == 0) return false;

  bool hasDigit = false;
  bool hasDecimalPoint = false;

  for (unsigned int i = 0; i < field.length(); i++) {
    char c = field.charAt(i);

    if (isDigitChar(c)) {
      hasDigit = true;
      continue;
    }

    if ((c == '-' || c == '+') && i == 0) continue;

    if (c == '.' && !hasDecimalPoint) {
      hasDecimalPoint = true;
      continue;
    }

    return false;
  }

  return hasDigit;
}

bool parseHistoryLine(String line, DataPoint& point)
{
  line.trim();
  if (line.length() == 0) return false;

  int idx1 = line.indexOf(',');
  int idx2 = (idx1 < 0) ? -1 : line.indexOf(',', idx1 + 1);
  int idx3 = (idx2 < 0) ? -1 : line.indexOf(',', idx2 + 1);
  int idx4 = (idx3 < 0) ? -1 : line.indexOf(',', idx3 + 1);
  int idx5 = (idx4 < 0) ? -1 : line.indexOf(',', idx4 + 1);
  int idx6 = (idx5 < 0) ? -1 : line.indexOf(',', idx5 + 1);
  if (idx1 < 0 || idx2 < 0 || idx3 < 0) return false;

  String timestampField = line.substring(0, idx1);
  String temperatureField = line.substring(idx1 + 1, idx2);
  String humidityField = line.substring(idx2 + 1, idx3);
  String soil1Field = (idx4 < 0) ? line.substring(idx3 + 1) : line.substring(idx3 + 1, idx4);
  String soil2Field = soil1Field;
  String pump1WateringMinutesField = "0";
  String pump2WateringMinutesField = "0";

  if (idx6 >= 0) {
    soil2Field = line.substring(idx4 + 1, idx5);
    pump1WateringMinutesField = line.substring(idx5 + 1, idx6);
    pump2WateringMinutesField = line.substring(idx6 + 1);
  } else if (idx5 >= 0) {
    pump1WateringMinutesField = line.substring(idx4 + 1, idx5);
    pump2WateringMinutesField = line.substring(idx5 + 1);
  } else if (idx4 >= 0) {
    pump1WateringMinutesField = line.substring(idx4 + 1);
  }

  if (!isUnsignedIntegerField(timestampField)
      || !isDecimalField(temperatureField)
      || !isDecimalField(humidityField)
      || !isDecimalField(soil1Field)
      || !isDecimalField(soil2Field)
      || !isDecimalField(pump1WateringMinutesField)
      || !isDecimalField(pump2WateringMinutesField)) {
    return false;
  }

  point.timestamp = (time_t)timestampField.toInt();
  if (point.timestamp <= 0) return false;

  point.temperature = temperatureField.toFloat();
  point.humidity = humidityField.toFloat();
  point.soilMoisture1 = soil1Field.toFloat();
  point.soilMoisture2 = soil2Field.toFloat();
  point.pump1WateringMinutes = pump1WateringMinutesField.toFloat();
  point.pump2WateringMinutes = pump2WateringMinutesField.toFloat();
  return true;
}

int countValidHistoryRows(const char* path)
{
  File file = LittleFS.open(path, "r");
  if (!file) return -1;

  int validRows = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    DataPoint parsedPoint;
    if (parseHistoryLine(line, parsedPoint)) validRows++;
  }

  file.close();
  return validRows;
}

int loadSensorHistoryFromPath(const char* path)
{
  File file = LittleFS.open(path, "r");
  if (!file) return -1;

  historyCount = 0;
  historyIndex = 0;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    DataPoint parsedPoint;
    if (parseHistoryLine(line, parsedPoint)) {
      addHistoryReading(
        parsedPoint.timestamp,
        parsedPoint.temperature,
        parsedPoint.humidity,
        parsedPoint.soilMoisture1,
        parsedPoint.soilMoisture2,
        parsedPoint.pump1WateringMinutes,
        parsedPoint.pump2WateringMinutes
      );
    }
  }

  file.close();
  return historyCount;
}

int loadSensorHistory()
{
  return loadSensorHistoryFromPath("/history.txt");
}

bool saveSensorHistory()
{
  File file = LittleFS.open("/history.txt", "w");
  if (!file) return false;

  int start = (historyIndex + MAX_HISTORY - historyCount) % MAX_HISTORY;
  for (int i = 0; i < historyCount; i++) {
    int idx = (start + i) % MAX_HISTORY;
    file.printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
      (unsigned long)history[idx].timestamp,
      history[idx].temperature,
      history[idx].humidity,
      history[idx].soilMoisture1,
      history[idx].soilMoisture2,
      history[idx].pump1WateringMinutes,
      history[idx].pump2WateringMinutes
    );
  }
  file.close();
  return true;
}

String getSensorHistoryJson()
{
  String json = "[";
  int start = (historyIndex + MAX_HISTORY - historyCount) % MAX_HISTORY;

  for (int i = 0; i < historyCount; i++) {
    int idx = (start + i) % MAX_HISTORY;
    if (i > 0) json += ",";

    json += "{\"t\":" + String((unsigned long)history[idx].timestamp);
    json += ",\"temp\":" + String(history[idx].temperature, 2);
    json += ",\"hum\":" + String(history[idx].humidity, 2);
    json += ",\"soil\":" + String(history[idx].soilMoisture1, 2);
    json += ",\"soil1\":" + String(history[idx].soilMoisture1, 2);
    json += ",\"soil2\":" + String(history[idx].soilMoisture2, 2);
    json += ",\"pump1WaterMinutes\":" + String(history[idx].pump1WateringMinutes, 2);
    json += ",\"pump2WaterMinutes\":" + String(history[idx].pump2WateringMinutes, 2);
    json += "}";
  }

  json += "]";
  return json;
}

void logDHTReading(float pump1WateringMinutes = 0.0, float pump2WateringMinutes = 0.0) {
  time_t now = time(nullptr);

  addHistoryReading(now, temperature, humidity, soilMoisture1, soilMoisture2, pump1WateringMinutes, pump2WateringMinutes);
  saveSensorHistory();
}


#endif
