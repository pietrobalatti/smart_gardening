#if defined(ESP8266)
  #include "ESP8266WiFi.h"
  #include "ESP8266HTTPClient.h"
#elif defined(ESP32)
  #include "WiFi.h"
  #include "HTTPClient.h"
#endif

#include <EasyDDNS.h>

const char * ssid = "Network";
const char * password = "Password";

WiFiServer server(80);

// Static IP params
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 254); // it is your router address
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8); //optional
  
void setup() { 

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure");
  }else{
    Serial.println("Static IP correctly configured");
  }

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP()); // Print the IP address
  server.begin();

  /* Supported DDNS providers: "duckdns", "noip", "dyndns", "dynu", "enom", "all-inkl",
     "selfhost.de", "dyndns.it", "strato", "freemyip", and "afraid.org" */
  EasyDDNS.service("duckdns");

  /*
    For DDNS Providers where you get a token:
      Use this: EasyDDNS.client("domain", "token");
    
    For DDNS Providers where you get username and password: ( Leave the password field empty "" if not required )
      Use this: EasyDDNS.client("domain", "username", "password");
  */
  EasyDDNS.client("mydomain.duckdns.org", "mytoken"); // Enter your DDNS Domain & Token

  // Get Notified when your IP changes
  EasyDDNS.onUpdate([&](const char* oldIP, const char* newIP){
    Serial.print("EasyDDNS - IP Change Detected: ");
    Serial.println(newIP);
  });
}

void loop() {
  // Check for new public IP every 10 seconds
  EasyDDNS.update(10000);

  // Check if connection has been lost and try to reconnect
  if(!WiFi.isConnected()) {
    Serial.println("WiFi connection lost - Trying to reconnect...");
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(5000);
      Serial.print(".");
    }
  }else
    Serial.println("WiFi connection is ok.");

  // Loop rate (1s)
  delay(1000);
  
}
