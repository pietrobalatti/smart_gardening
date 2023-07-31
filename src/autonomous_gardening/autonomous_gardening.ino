// WiFi libraries
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// DDNS libraries
#include <EasyDDNS.h>

// Web server libraries
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// Project libraries
#include "webserver.h"

// Import private user data
#include "config/user_data.h"

// Global variables
AsyncWebServer server(port);


void setup() { 

  /*****************************/
  /*        WiFi config        */
  /*****************************/
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure");
  }else{
    Serial.println("Static IP correctly configured");
  }
  // Connect to WiFi
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP()); // Print the IP address

  /*****************************/
  /*       Webserver init      */
  /*****************************/
  initialize_webserver(server);
  server.begin();

  /*****************************/
  /*        DDNS config        */
  /*****************************/
  /* Supported DDNS providers: "duckdns", "noip", "dyndns", "dynu", "enom", "all-inkl",
     "selfhost.de", "dyndns.it", "strato", "freemyip", and "afraid.org" */
  EasyDDNS.service("duckdns");

  /*
    For DDNS Providers where you get a token:
      Use this: EasyDDNS.client("domain", "token");
    
    For DDNS Providers where you get username and password: ( Leave the password field empty "" if not required )
      Use this: EasyDDNS.client("domain", "username", "password");
  */
  EasyDDNS.client(my_DDNS_domain, my_DDNS_token);

  // Get Notified when your IP changes
  EasyDDNS.onUpdate([&](const char* oldIP, const char* newIP){
    Serial.print("EasyDDNS - IP Change Detected: ");
    Serial.println(newIP);
  });


}

void loop() {


  /*****************************/
  /*           WiFi            */
  /*****************************/
  // Check if connection has been lost and try to reconnect
  if(!WiFi.isConnected()) {
    Serial.println("WiFi connection lost - Trying to reconnect...");
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(5000);
      Serial.print(".");
    }
  }/*else
    Serial.println("WiFi connection is ok.");*/

  /*****************************/
  /*           DDNS            */
  /*****************************/
  // Check for new public IP every 10 seconds
  EasyDDNS.update(10000);

  // Loop rate (1s)
  // delay(1000);
  
}
