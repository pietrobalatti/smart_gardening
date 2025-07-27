#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Ticker.h>

// DHT22 library
#include "dht_sensor.h"

// Set pumps GPIOs
const int pump2Pin = 15; // D8
const int pump1Pin = 13; // D7

// Stores pumps state and tickers for timed operations
String pump1State;
String pump2State;
Ticker pump1Ticker;
Ticker pump2Ticker;
time_t lastWateredPump1 = 0;
time_t lastWateredPump2 = 0;


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
  }else if (var == "LASTWATERED1"){
    return formatTime(lastWateredPump1);
  }  else if (var == "LASTWATERED2"){
    return formatTime(lastWateredPump2);
  }

  return "none";
   
}

void turnOffPump1() {
  digitalWrite(pump1Pin, LOW);
  // Serial.println("Pump 1 OFF (after 5 min)");
  lastWateredPump1 = time(nullptr);
  // html.replace("%LAST1%", formatTime(lastWateredPump1));
}

void turnOffPump2() {
  digitalWrite(pump2Pin, LOW);
  // Serial.println("Pump 2 OFF (after 5 min)");
  lastWateredPump2 = time(nullptr);
}

void initialize_webserver(AsyncWebServer& server)
 {
    // Initialize LittleFS
    if(!LittleFS.begin())
    {
      Serial.println("An Error has occurred while mounting LittleFS");
    }
    

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
      digitalWrite(pump1Pin, HIGH);
      request->send(LittleFS, "/index.html", String(), false, processor);
    });
    
    server.on("/on2", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      digitalWrite(pump2Pin, HIGH);
      request->send(LittleFS, "/index.html", String(), false, processor);
    });
    
    // Route to set GPIO to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
      digitalWrite(pump1Pin, LOW);
      request->send(LittleFS, "/index.html", String(), false, processor);
      lastWateredPump1 = time(nullptr);
    });

    server.on("/off2", HTTP_GET, [](AsyncWebServerRequest *request){
      digitalWrite(pump2Pin, LOW);
      request->send(LittleFS, "/index.html", String(), false, processor);
      lastWateredPump2 = time(nullptr);
    });

    server.on("/on1timed", HTTP_GET, [](AsyncWebServerRequest *request){
      digitalWrite(pump1Pin, HIGH);
      // pump1Ticker.once_ms(5 * 60 * 1000, turnOffPump1); // 5 minutes timer
      pump1Ticker.once_ms(5 * 1000, turnOffPump1); // 5 seconds timer for testing
      // Serial.println("Pump 1 ON (timed for 5 min)");
      request->redirect("/");
    });
    
    server.on("/on2timed", HTTP_GET, [](AsyncWebServerRequest *request){
      digitalWrite(pump2Pin, HIGH);
      // pump2Ticker.once_ms(5 * 60 * 1000, turnOffPump2);
      pump2Ticker.once_ms(5 * 1000, turnOffPump2); // 5 seconds timer for testing
      // Serial.println("Pump 2 ON (timed for 5 min)");
      request->redirect("/");
    });

    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", getTemperature().c_str());
    });
    
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", getHumidity().c_str());
    });

    server.on("/history.json", HTTP_GET, [](AsyncWebServerRequest *request){
      File file = LittleFS.open("/history.txt", "r");
      if (!file) {
        request->send(500, "text/plain", "Failed to open file");
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
        String ts = line.substring(0, idx1);
        String temp = line.substring(idx1 + 1, idx2);
        String hum = line.substring(idx2 + 1);
  
        if (!first) json += ",";
        first = false;
  
        json += "{\"t\":" + ts + ",\"temp\":" + temp + ",\"hum\":" + hum + "}";
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
