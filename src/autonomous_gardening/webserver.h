#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Ticker.h>

// DHT22 library
#include "dht_sensor.h"

// Set pumps GPIOs
const int pump2Pin = 15; // D8
const int pump1Pin = 13; // D7
const unsigned int pump1DefaultWateringMinutes = 8;
const unsigned int pump2DefaultWateringMinutes = 3;
const unsigned int minWateringMinutes = 1;
const unsigned int maxWateringMinutes = 15;
const float pump1TankCapacityMinutes = 90.0;
const float pump2TankCapacityMinutes = 30.0;

// Stores pumps state and tickers for timed operations
String pump1State;
String pump2State;
Ticker pump1Ticker;
Ticker pump2Ticker;
time_t lastWateredPump1 = 0;
time_t lastWateredPump2 = 0;
volatile bool sensorRefreshRequested = false;
unsigned long sensorRefreshRequestMillis = 0;
unsigned long sensorLastRefreshMillis = 0;
time_t pump1StartedAt = 0;
time_t pump2StartedAt = 0;
volatile bool pump1StopRequested = false;
volatile bool pump2StopRequested = false;
volatile time_t pump1RequestedStopAt = 0;
volatile time_t pump2RequestedStopAt = 0;

struct WateringBucket {
  time_t hourStart;
  float pump1WateringMinutes;
  float pump2WateringMinutes;
};

WateringBucket wateringBuckets[MAX_HISTORY];
int wateringBucketCount = 0;
File historyRestoreFile;
bool historyRestoreOk = false;
bool historyRestoreFinished = false;
float pump1TankUsedMinutes = 0.0;
float pump2TankUsedMinutes = 0.0;
time_t pump1TankLastFilled = 0;
time_t pump2TankLastFilled = 0;


String getTemperature() // function to get temperature from dht22
{
  // temperature gets updated in dht_sensor.h 
  return String(temperature);
}

String getHumidity()  //  // function to get humifdity from dht22
{
  // humidity gets updated in dht_sensor.h
  return String(humidity);
}

String getSoilMoisture()
{
  return String(soilMoisture, 1);
}

String getSoilMoistureRaw()
{
  return String(soilMoistureRaw);
}

String getSensorReadingsJson()
{
  String json = "{";
  json += "\"temperature\":" + getTemperature() + ",";
  json += "\"humidity\":" + getHumidity() + ",";
  json += "\"soilMoisture\":" + getSoilMoisture() + ",";
  json += "\"soilMoistureRaw\":" + getSoilMoistureRaw() + ",";
  json += "\"refreshPending\":" + String(sensorRefreshRequested ? "true" : "false") + ",";
  json += "\"lastRefreshMillis\":" + String(sensorLastRefreshMillis);
  json += "}";
  return json;
}

void requestSensorRefresh()
{
  sensorRefreshRequestMillis = millis();
  sensorRefreshRequested = true;
}

void refreshSensorReadings()
{
  updateDHT();
  updateSoilMoisture();
  sensorLastRefreshMillis = millis();
}

void handleRequestedSensorRefresh()
{
  if (!sensorRefreshRequested) return;

  sensorRefreshRequested = false;
  refreshSensorReadings();
}

String formatTime(time_t t) {
  struct tm* tm = localtime(&t);
  char buf[32];
  if (t == 0) return "never";
  sprintf(buf, "%04d-%02d-%02d %02d:%02d",
          tm->tm_year + 1900,
          tm->tm_mon + 1,
          tm->tm_mday,
          tm->tm_hour,
          tm->tm_min);
  return String(buf);
}

unsigned int getWateringMinutes(AsyncWebServerRequest *request, unsigned int defaultMinutes)
{
  unsigned int minutes = defaultMinutes;

  if (request->hasParam("minutes")) {
    int requestedMinutes = request->getParam("minutes")->value().toInt();
    if (requestedMinutes > 0) {
      minutes = (unsigned int)requestedMinutes;
    }
  }

  if (minutes < minWateringMinutes) return minWateringMinutes;
  if (minutes > maxWateringMinutes) return maxWateringMinutes;
  return minutes;
}

unsigned long wateringDurationMs(unsigned int minutes)
{
  return (unsigned long)minutes * 60UL * 1000UL;
}

void saveTankState()
{
  File file = LittleFS.open("/tank_state.txt", "w");
  if (!file) return;

  file.printf("%.2f,%.2f,%lu,%lu\n",
    pump1TankUsedMinutes,
    pump2TankUsedMinutes,
    pump1TankLastFilled,
    pump2TankLastFilled
  );
  file.close();
}

void loadTankState()
{
  File file = LittleFS.open("/tank_state.txt", "r");
  if (!file) return;

  String line = file.readStringUntil('\n');
  line.trim();
  file.close();
  if (line.length() == 0) return;

  int idx1 = line.indexOf(',');
  int idx2 = line.indexOf(',', idx1 + 1);
  int idx3 = line.indexOf(',', idx2 + 1);
  if (idx1 < 0 || idx2 < 0 || idx3 < 0) return;

  pump1TankUsedMinutes = line.substring(0, idx1).toFloat();
  pump2TankUsedMinutes = line.substring(idx1 + 1, idx2).toFloat();
  pump1TankLastFilled = (time_t)line.substring(idx2 + 1, idx3).toInt();
  pump2TankLastFilled = (time_t)line.substring(idx3 + 1).toInt();
}

void addTankUsage(unsigned int pumpNumber, float minutes)
{
  if (minutes <= 0.0) return;

  if (pumpNumber == 1) {
    pump1TankUsedMinutes += minutes;
    if (pump1TankUsedMinutes > pump1TankCapacityMinutes) {
      pump1TankUsedMinutes = pump1TankCapacityMinutes;
    }
  } else if (pumpNumber == 2) {
    pump2TankUsedMinutes += minutes;
    if (pump2TankUsedMinutes > pump2TankCapacityMinutes) {
      pump2TankUsedMinutes = pump2TankCapacityMinutes;
    }
  }

  saveTankState();
}

void resetTank(unsigned int pumpNumber)
{
  time_t now = time(nullptr);

  if (pumpNumber == 1) {
    pump1TankUsedMinutes = 0.0;
    pump1TankLastFilled = now;
    if (pump1StartedAt > 0) pump1StartedAt = now;
  } else if (pumpNumber == 2) {
    pump2TankUsedMinutes = 0.0;
    pump2TankLastFilled = now;
    if (pump2StartedAt > 0) pump2StartedAt = now;
  }

  saveTankState();
}

String getTankStatusJson()
{
  String json = "{";
  json += "\"pump1\":{";
  json += "\"used\":" + String(pump1TankUsedMinutes, 2) + ",";
  json += "\"capacity\":" + String(pump1TankCapacityMinutes, 0) + ",";
  json += "\"lastFilled\":" + String((unsigned long)pump1TankLastFilled);
  json += "},";
  json += "\"pump2\":{";
  json += "\"used\":" + String(pump2TankUsedMinutes, 2) + ",";
  json += "\"capacity\":" + String(pump2TankCapacityMinutes, 0) + ",";
  json += "\"lastFilled\":" + String((unsigned long)pump2TankLastFilled);
  json += "}";
  json += "}";
  return json;
}

time_t getHourStart(time_t timestamp)
{
  if (timestamp <= 0) return 0;

  struct tm* timeInfo = localtime(&timestamp);
  if (timeInfo == nullptr) return 0;

  struct tm hourInfo = *timeInfo;
  hourInfo.tm_min = 0;
  hourInfo.tm_sec = 0;
  return mktime(&hourInfo);
}

int findWateringBucket(time_t hourStart)
{
  for (int i = 0; i < wateringBucketCount; i++) {
    if (wateringBuckets[i].hourStart == hourStart) return i;
  }

  return -1;
}

int getWateringBucket(time_t hourStart)
{
  int existingBucket = findWateringBucket(hourStart);
  if (existingBucket >= 0) return existingBucket;

  if (wateringBucketCount < MAX_HISTORY) {
    int newBucket = wateringBucketCount++;
    wateringBuckets[newBucket].hourStart = hourStart;
    wateringBuckets[newBucket].pump1WateringMinutes = 0.0;
    wateringBuckets[newBucket].pump2WateringMinutes = 0.0;
    return newBucket;
  }

  int oldestBucket = 0;
  for (int i = 1; i < MAX_HISTORY; i++) {
    if (wateringBuckets[i].hourStart < wateringBuckets[oldestBucket].hourStart) {
      oldestBucket = i;
    }
  }

  wateringBuckets[oldestBucket].hourStart = hourStart;
  wateringBuckets[oldestBucket].pump1WateringMinutes = 0.0;
  wateringBuckets[oldestBucket].pump2WateringMinutes = 0.0;
  return oldestBucket;
}

void addWateringMinutes(time_t hourStart, unsigned int pumpNumber, float minutes)
{
  if (hourStart <= 0 || minutes <= 0.0) return;

  int bucket = getWateringBucket(hourStart);
  if (pumpNumber == 1) {
    wateringBuckets[bucket].pump1WateringMinutes += minutes;
  } else if (pumpNumber == 2) {
    wateringBuckets[bucket].pump2WateringMinutes += minutes;
  }
}

void addPumpWateringDuration(unsigned int pumpNumber, time_t startTime, time_t endTime)
{
  if (startTime <= 0 || endTime <= startTime) return;

  addTankUsage(pumpNumber, (float)(endTime - startTime) / 60.0);

  time_t cursor = startTime;
  while (cursor < endTime) {
    time_t hourStart = getHourStart(cursor);
    if (hourStart <= 0) return;

    time_t hourEnd = hourStart + 3600;
    time_t segmentEnd = (endTime < hourEnd) ? endTime : hourEnd;
    addWateringMinutes(hourStart, pumpNumber, (float)(segmentEnd - cursor) / 60.0);
    cursor = segmentEnd;
  }
}

void accountPumpRuntimeUntil(time_t boundary)
{
  if (boundary <= 0) return;

  if (pump1StartedAt > 0 && pump1StartedAt < boundary) {
    addPumpWateringDuration(1, pump1StartedAt, boundary);
    pump1StartedAt = boundary;
  }

  if (pump2StartedAt > 0 && pump2StartedAt < boundary) {
    addPumpWateringDuration(2, pump2StartedAt, boundary);
    pump2StartedAt = boundary;
  }
}

float consumeWateringMinutes(time_t hourStart, unsigned int pumpNumber)
{
  int bucket = findWateringBucket(hourStart);
  if (bucket < 0) return 0.0;

  if (pumpNumber == 1) {
    float minutes = wateringBuckets[bucket].pump1WateringMinutes;
    wateringBuckets[bucket].pump1WateringMinutes = 0.0;
    return minutes;
  }

  if (pumpNumber == 2) {
    float minutes = wateringBuckets[bucket].pump2WateringMinutes;
    wateringBuckets[bucket].pump2WateringMinutes = 0.0;
    return minutes;
  }

  return 0.0;
}

void startPump1()
{
  if (pump1StartedAt == 0) {
    pump1StartedAt = time(nullptr);
  }
  digitalWrite(pump1Pin, HIGH);
}

void startPump2()
{
  if (pump2StartedAt == 0) {
    pump2StartedAt = time(nullptr);
  }
  digitalWrite(pump2Pin, HIGH);
}

void stopPump1At(time_t stopTime)
{
  bool wasRunning = (pump1StartedAt > 0 || digitalRead(pump1Pin));

  pump1Ticker.detach();
  if (pump1StartedAt > 0) {
    addPumpWateringDuration(1, pump1StartedAt, stopTime);
    pump1StartedAt = 0;
  }

  digitalWrite(pump1Pin, LOW);
  if (wasRunning) {
    lastWateredPump1 = stopTime;
  }
}

void stopPump2At(time_t stopTime)
{
  bool wasRunning = (pump2StartedAt > 0 || digitalRead(pump2Pin));

  pump2Ticker.detach();
  if (pump2StartedAt > 0) {
    addPumpWateringDuration(2, pump2StartedAt, stopTime);
    pump2StartedAt = 0;
  }

  digitalWrite(pump2Pin, LOW);
  if (wasRunning) {
    lastWateredPump2 = stopTime;
  }
}

void stopPump1()
{
  stopPump1At(time(nullptr));
}

void stopPump2()
{
  stopPump2At(time(nullptr));
}

void handlePumpStopRequests()
{
  if (pump1StopRequested) {
    time_t stopTime = pump1RequestedStopAt;
    pump1RequestedStopAt = 0;
    pump1StopRequested = false;
    stopPump1At((stopTime > 0) ? stopTime : time(nullptr));
  }

  if (pump2StopRequested) {
    time_t stopTime = pump2RequestedStopAt;
    pump2RequestedStopAt = 0;
    pump2StopRequested = false;
    stopPump2At((stopTime > 0) ? stopTime : time(nullptr));
  }
}

// Replaces placeholder with LED state value
String processor(const String& var)
{
  Serial.println(var);
  if(var == "STATE")
  {
    if(digitalRead(pump1Pin))
    {
      pump1State = "ON";
    }
    else
    {
      pump1State = "OFF";
    }
    Serial.print(pump1State);
    return pump1State;
  }else if(var == "STATE2")
  {
    if(digitalRead(pump2Pin))
    {
      pump2State = "ON";
    }
    else
    {
      pump2State = "OFF";
    }
    Serial.print(pump2State);
    return pump2State;
  }else if (var == "TEMPERATURE")
  {
    return getTemperature();
  }
  else if (var == "HUMIDITY"){
    return getHumidity();
  }else if (var == "SOILMOISTURE"){
    return getSoilMoisture();
  }else if (var == "SOILMOISTURERAW"){
    return getSoilMoistureRaw();
  }else if (var == "LASTWATERED1"){
    return formatTime(lastWateredPump1);
  }  else if (var == "LASTWATERED2"){
    return formatTime(lastWateredPump2);
  }else if (var == "PUMP1DEFAULTWATERINGMINUTES"){
    return String(pump1DefaultWateringMinutes);
  }else if (var == "PUMP2DEFAULTWATERINGMINUTES"){
    return String(pump2DefaultWateringMinutes);
  }else if (var == "MINWATERINGMINUTES"){
    return String(minWateringMinutes);
  }else if (var == "MAXWATERINGMINUTES"){
    return String(maxWateringMinutes);
  }

  return "none";
   
}

void turnOffPump1() {
  digitalWrite(pump1Pin, LOW);
  pump1RequestedStopAt = time(nullptr);
  pump1StopRequested = true;
}

void turnOffPump2() {
  digitalWrite(pump2Pin, LOW);
  pump2RequestedStopAt = time(nullptr);
  pump2StopRequested = true;
}

void initialize_webserver(AsyncWebServer& server)
 {
    // Initialize LittleFS
    if(!LittleFS.begin())
    {
      Serial.println("An Error has occurred while mounting LittleFS");
    }
    loadTankState();
    

    // ✅ Serve all static files from /data
    server.serveStatic("/", LittleFS, "/");

    pinMode(pump1Pin, OUTPUT);
    pinMode(pump2Pin, OUTPUT);


    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      request->send(LittleFS, "/index.html", String(), false, processor);
    });

    // Route to load style.css file
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/style.css", "text/css");
    });

    // Route to checj RSSI (WiFi signal strength)
    server.on("/rssi", HTTP_GET, [](AsyncWebServerRequest *request){
      int rssi = WiFi.RSSI();
      request->send(200, "text/plain", String(rssi));
    });

    // Route to set GPIO to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      pump1Ticker.detach();
      startPump1();
      request->send(LittleFS, "/index.html", String(), false, processor);
    });
    
    server.on("/on2", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      pump2Ticker.detach();
      startPump2();
      request->send(LittleFS, "/index.html", String(), false, processor);
    });
    
    // Route to set GPIO to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
      stopPump1();
      request->send(LittleFS, "/index.html", String(), false, processor);
    });

    server.on("/off2", HTTP_GET, [](AsyncWebServerRequest *request){
      stopPump2();
      request->send(LittleFS, "/index.html", String(), false, processor);
    });

    server.on("/on1timed", HTTP_GET, [](AsyncWebServerRequest *request){
      unsigned int minutes = getWateringMinutes(request, pump1DefaultWateringMinutes);
      startPump1();
      pump1Ticker.detach();
      pump1Ticker.once_ms(wateringDurationMs(minutes), turnOffPump1);
      request->redirect("/");
    });
    
    server.on("/on2timed", HTTP_GET, [](AsyncWebServerRequest *request){
      unsigned int minutes = getWateringMinutes(request, pump2DefaultWateringMinutes);
      startPump2();
      pump2Ticker.detach();
      pump2Ticker.once_ms(wateringDurationMs(minutes), turnOffPump2);
      request->redirect("/");
    });

    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", getTemperature());
    });
    
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", getHumidity());
    });

    server.on("/soilmoisture", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", getSoilMoisture());
    });

    server.on("/soilmoistureraw", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", getSoilMoistureRaw());
    });

    server.on("/refreshsensors", HTTP_GET, [](AsyncWebServerRequest *request){
      requestSensorRefresh();
      String json = "{";
      json += "\"requested\":true,";
      json += "\"requestMillis\":" + String(sensorRefreshRequestMillis);
      json += "}";
      request->send(200, "application/json", json);
    });

    server.on("/sensors.json", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "application/json", getSensorReadingsJson());
    });

    server.on("/tankstatus.json", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "application/json", getTankStatusJson());
    });

    server.on("/resetpump1tank", HTTP_POST, [](AsyncWebServerRequest *request){
      resetTank(1);
      request->send(200, "application/json", getTankStatusJson());
    });

    server.on("/resetpump2tank", HTTP_POST, [](AsyncWebServerRequest *request){
      resetTank(2);
      request->send(200, "application/json", getTankStatusJson());
    });

    server.on("/history.json", HTTP_GET, [](AsyncWebServerRequest *request){
      File file = LittleFS.open("/history.txt", "r");
      if (!file) {
        request->send(200, "application/json", "[]");
        return;
      }
  
      String json = "[";
      bool first = true;
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

        String ts = line.substring(0, idx1);
        String temp = line.substring(idx1 + 1, idx2);
        String hum = line.substring(idx2 + 1, idx3);
        String soil = (idx4 < 0) ? line.substring(idx3 + 1) : line.substring(idx3 + 1, idx4);
        String pump1WaterMinutes = (idx4 < 0)
          ? "0"
          : ((idx5 < 0) ? line.substring(idx4 + 1) : line.substring(idx4 + 1, idx5));
        String pump2WaterMinutes = (idx5 < 0) ? "0" : line.substring(idx5 + 1);
  
        if (!first) json += ",";
        first = false;
  
        json += "{\"t\":" + ts + ",\"temp\":" + temp + ",\"hum\":" + hum + ",\"soil\":" + soil;
        json += ",\"pump1WaterMinutes\":" + pump1WaterMinutes;
        json += ",\"pump2WaterMinutes\":" + pump2WaterMinutes + "}";
      }
      json += "]";
      file.close();
      request->send(200, "application/json", json);
    });


    // Serve the history.txt, to open it: http://<your-device-ip>/history.txt
    server.on("/history.txt", HTTP_GET, [](AsyncWebServerRequest *request){
      File file = LittleFS.open("/history.txt", "r");
      if (!file) {
        request->send(500, "text/plain", "Failed to open file");
        return;
      }
    
      String content = "";
      while (file.available()) {
        content += file.readStringUntil('\n') + "\n";
      }
      file.close();
    
      request->send(200, "text/plain", content);
    });

    server.on("/backuphistory", HTTP_GET, [](AsyncWebServerRequest *request){
      if (!LittleFS.exists("/history.txt")) {
        request->send(404, "text/plain", "No history.txt found.");
        return;
      }

      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/history.txt", "text/plain", true);
      response->addHeader("Content-Disposition", "attachment; filename=\"history.txt\"");
      request->send(response);
    });

    server.on(
      "/restorehistory",
      HTTP_POST,
      [](AsyncWebServerRequest *request){
        bool restored = historyRestoreFinished && historyRestoreOk;
        historyRestoreFinished = false;
        historyRestoreOk = false;

        if (restored) {
          request->redirect("/");
          return;
        }

        request->send(500, "text/plain", "Failed to restore history.txt");
      },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        (void)request;
        (void)filename;

        if (index == 0) {
          historyRestoreFinished = false;
          historyRestoreOk = true;
          if (historyRestoreFile) historyRestoreFile.close();
          if (LittleFS.exists("/history-restore.tmp")) LittleFS.remove("/history-restore.tmp");
          historyRestoreFile = LittleFS.open("/history-restore.tmp", "w");
          if (!historyRestoreFile) historyRestoreOk = false;
        }

        if (historyRestoreOk && len > 0) {
          size_t written = historyRestoreFile.write(data, len);
          if (written != len) historyRestoreOk = false;
        }

        if (final) {
          if (historyRestoreFile) historyRestoreFile.close();

          bool replaced = false;
          if (historyRestoreOk) {
            bool removedOldHistory = true;
            if (LittleFS.exists("/history.txt")) {
              removedOldHistory = LittleFS.remove("/history.txt");
            }
            replaced = removedOldHistory && LittleFS.rename("/history-restore.tmp", "/history.txt");
          }

          if (replaced) {
            loadSensorHistory();
          } else if (LittleFS.exists("/history-restore.tmp")) {
            LittleFS.remove("/history-restore.tmp");
          }

          historyRestoreOk = replaced;
          historyRestoreFinished = true;
        }
      }
    );

    // Delete history file command
    server.on("/deletehistory", HTTP_GET, [](AsyncWebServerRequest *request){
      if (LittleFS.exists("/history.txt")) {
        if (LittleFS.remove("/history.txt")) {
          // Serial.println("history.txt deleted.");
          request->send(200, "text/plain", "history.txt deleted.");
        } else {
          // Serial.println("Failed to delete history.txt.");
          request->send(500, "text/plain", "Failed to delete history.txt.");
        }
      } else {
        // Serial.println("history.txt does not exist.");
        request->send(200, "text/plain", "No history.txt found.");
      }
    });

    // Reboot device command
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "Rebooting...");
      Serial.println("🔄 ESP8266 is rebooting...");
      delay(500);  // give time to send the response
      ESP.restart();
    });

    server.on("/status.json", HTTP_GET, [](AsyncWebServerRequest *request){
      FSInfo fs_info;
      LittleFS.info(fs_info);
    
      unsigned long uptimeSec = millis() / 1000;
      if (uptimeSec == 0 || uptimeSec > 315360000) uptimeSec = 1;  // sanity fallback
      int rssi = WiFi.RSSI();
      int heap = ESP.getFreeHeap();
    
      String json = "{";
      json += "\"uptime\":" + String(uptimeSec) + ",";
      json += "\"heap\":" + String(heap) + ",";
      json += "\"rssi\":" + String(rssi) + ",";
      json += "\"fs_total\":" + String(fs_info.totalBytes) + ",";
      json += "\"fs_used\":" + String(fs_info.usedBytes);
      json += "}";
    
      request->send(200, "application/json", json);
    });
    

    return;
}


#endif // WEBSERVER_H
