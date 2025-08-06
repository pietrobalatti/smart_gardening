# Smart gardening with NodeMCU (esp8266)
This project...

It includes:
- DDNS client to support network without static IP
- Web server to control the pins state
- ...

## Prerequistes
Install Arduino libraries within the Arduino IDE by following [this guide](https://github.com/esp8266/Arduino#installing-with-boards-manager).

Select as board "Generic ESP 8266": `Tools->Board->ESP8266 Boards (3.1.2)->Generic ESP 8266 module`.

## HOW TO
After having installed all the library and plugins listed below (EasyDDNS, ESPAsyncWebServer, LittleFS) you need to:
- copy the file `config/user_data_example.h`, rename it as `config/user_data.h`, and place your data. Notice that this private data can not be commited in this repo since the `config/user_data.h` file is listed in the [.gitignore](https://git-scm.com/docs/gitignore) file
- select the correct serial port: *Tools->Port*
- select *Tools > ESP8266 LittleFS Data Upload* to upload the data folder files onto your device (index.html, style.css)
- upload the "autonomous_gardening.ino" sketch

## DDNS for esp8266
Since I did not have a static IP, this project includes also the implementation of a Dynamic DNS ([DDNS](https://en.wikipedia.org/wiki/Dynamic_DNS)). For this purpose, the [EasyDDNS](https://github.com/ayushsharma82/EasyDDNS) library was included in the code. Multiple DDNS services are supported, namely "duckdns", "noip", "dyndns", "dynu", "enom", "all-inkl", "selfhost.de", "dyndns.it", "strato", "freemyip", and "afraid.org".

I personally used "duckdns" since it is a free and open-source platform, but any of the above should work fine. 

To include this library, follow the steps on the [EasyDDNS]((https://github.com/ayushsharma82/EasyDDNS#how-to-install)) GitHub page.


## Web server
Install [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer):
- Click [here](https://github.com/me-no-dev/ESPAsyncWebServer/archive/refs/heads/master.zip) to download the ESPAsyncWebServer library. You should have a .zip folder in your Downloads folder
- Unzip the .zip folder and you should get ESPAsyncWebServer-master folder
- Rename your folder from ESPAsyncWebServer-master to ESPAsyncWebServer
- Move the ESPAsyncWebServer folder to your Arduino IDE installation libraries folder

Install [LittleFS plugin](https://github.com/earlephilhower/arduino-esp8266littlefs-plugin) to be able to pack sketch data folder into LittleFS filesystem image, and upload the image to ESP8266 flash memory.

### Installation
- Make sure you use one of the supported versions of Arduino IDE and have ESP8266 core installed.
- Download the tool archive from (https://github.com/earlephilhower/arduino-esp8266littlefs-plugin/releases).
- In your Arduino sketchbook directory, create tools directory if it doesn't exist yet.
- Unpack the tool into tools directory (the path will look like `<home_dir>/Arduino/tools/ESP8266LittleFS/tool/esp8266littlefs.jar)`.
- Restart Arduino IDE. 

### Usage
- Open a sketch (or create a new one and save it).
- Go to sketch directory (choose Sketch > Show Sketch Folder).
- Create a directory named `data` and any files you want in the file system there.
- Make sure you have selected a board, port, and closed Serial Monitor.
- Select *Tools > ESP8266 LittleFS Data Upload* menu item. This should start uploading the files into ESP8266 flash file system.
  When done, IDE status bar will display LittleFS Image Uploaded message. Might take a few minutes for large file system sizes.


<!-- TO-DO -->
<!-- - Mention token or other auth methods for DDNS -->

## Useful links
Set a LAN static IP on ESP8266: https://randomnerdtutorials.com/esp8266-nodemcu-static-fixed-ip-address-arduino/

Build a Web Server using SPIFFS: https://watchitgroup.wixsite.com/website/post/part-4-lesson-2-nodemcu-web-server-using-spiffs-and-dht11-sensor
Since SPIFFS has been deprecated, I substitued every SPIFFS instance with the more modern LittleFS according to [this link](https://microcontrollerslab.com/littlefs-introduction-install-esp8266-nodemcu-filesystem-uploader-arduino/).