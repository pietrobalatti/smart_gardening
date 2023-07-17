# Smart gardening with NodeMCU (esp8266)
This project...

It includes:
- DDNS client to support network without static IP
- Web server to control the pins state
- ...

## Prerequistes
Install Arduino libraries within the Arduino IDE by following [this guide](https://github.com/esp8266/Arduino#installing-with-boards-manager).

Select as board "Generic ESP 8266".

## DDNS for esp8266
Since I did not have a static IP, this project includes also the implementation of a Dynamic DNS [(DDNS)](). For this purpose, the [EasyDDNS](https://github.com/ayushsharma82/EasyDDNS) library was included in the code. Multiple DDNS services are supported, namely "duckdns", "noip", "dyndns", "dynu", "enom", "all-inkl", "selfhost.de", "dyndns.it", "strato", "freemyip", and "afraid.org".

I personally used "duckdns" since it is a free and open-source platform, but any of the above should work fine. 

To include this library, follow the steps on the [EasyDDNS]((https://github.com/ayushsharma82/EasyDDNS#how-to-install)) GitHub page.

<!-- TO-DO -->
<!-- - Mention token or other auth methods for DDNS -->

## Useful links
Set a LAN static IP on ESP8266: https://randomnerdtutorials.com/esp8266-nodemcu-static-fixed-ip-address-arduino/