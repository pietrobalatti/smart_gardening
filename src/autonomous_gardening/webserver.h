// webserver.h
#ifndef WEBSERVER_H
#define WEBSERVER_H

// Set pumps GPIOs
const int pump2Pin = 15; // D8
const int pump1Pin = 13; // D7

// Stores pumps state
String pump1State;
String pump2State;

String getTemperature() // function to get temperature from dht11
{
  
//  float temperature =dht.readTemperature();
  float temperature = 30.0;
 
  return String(temperature);
}

String getHumidity()  //  // function to get humifdity from dht11
{
    
//  float humidity = dht.readHumidity();
  float humidity = 40.0;
  
  return String(humidity);
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
  }

  return "none";
   
}

void initialize_webserver(AsyncWebServer& server)
 {
    // Initialize LittleFS
    if(!LittleFS.begin())
    {
      Serial.println("An Error has occurred while mounting LittleFS");
    }


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
    });

    server.on("/off2", HTTP_GET, [](AsyncWebServerRequest *request){
      digitalWrite(pump2Pin, LOW);
      request->send(LittleFS, "/index.html", String(), false, processor);
    });

    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", getTemperature().c_str());
    });
    
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/plain", getHumidity().c_str());
    });


    return;
}


#endif // WEBSERVER_H
