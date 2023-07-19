// data.h
#ifndef DATA_H
#define DATA_H

#include <IPAddress.h>

// WiFi params
const char * ssid = "Network";
const char * password = "Password";
const int port = 80;
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 254); // your router address
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);

// DDNS params
const char * my_DDNS_domain = "mydomain.duckdns.org";
const char * my_DDNS_token = "mytoken";

#endif // DATA_H
