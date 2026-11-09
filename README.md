# Smart gardening with NodeMCU (esp8266)
This project...

It includes:
- DDNS client to support network without static IP
- Web server to control the pins state
- ...

## Prerequistes
### Install Arduine IDE and ESP8266 libraries
Starting with 1.6.4, Arduino allows installation of third-party platform packages using Boards Manager:
- [Download and install Arduino IDE 2.x](https://www.arduino.cc/en/software)
- Start Arduino, sometimes you need --no-sandbox: `./arduino-ide_*.AppImage --no-sandbox`.
- Go to  `File > Preferences > Additional Boards Manager URLs` .
- Enter `https://arduino.esp8266.com/stable/package_esp8266com_index.json` (you can add multiple URLs, separating them with commas).
- Go to `Tools > Board > Boards Manager...`  menu and install *esp8266* platform.

[Source](https://github.com/esp8266/Arduino#installing-with-boards-manager)

### Board selection
Select as board "Generic ESP 8266": `Tools > Board > esp8266 > Generic ESP8266 Module`.

## Additional libraries and plugins

### DHT22 sensor
In Arduino IDE, open `Sketch > Include Library > Manage Libraries...`, search for **DHT sensor library**, and install the version by **Adafruit**. Choose **INSTALL ALL** when prompted to also install **Adafruit Unified Sensor**. This provides the `DHT.h` header required by the sketch.


### DDNS for esp8266
Since I did not have a static IP, this project includes also the implementation of a Dynamic DNS ([DDNS](https://en.wikipedia.org/wiki/Dynamic_DNS)). For this purpose, the [EasyDDNS](https://github.com/ayushsharma82/EasyDDNS) library was included in the code. Multiple DDNS services are supported, namely "duckdns", "noip", "dyndns", "dynu", "enom", "all-inkl", "selfhost.de", "dyndns.it", "strato", "freemyip", and "afraid.org".

I personally used "duckdns" since it is a free and open-source platform, but any of the above should work fine. \
To include this library, go to `Sketch > Include Library > Manage Libraries`, search for `EasyDDNS` and install it.

[Source](https://github.com/ayushsharma82/EasyDDNS#how-to-install)

### Web server
Install [ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer).
To include this library, go to `Sketch > Include Library > Manage Libraries`, search for `ESPAsyncWebServer` and install it. You'll be asked to install some other dependencies as `AsyncTCP` and `ESPAsyncTCP`, click on `INSTALL ALL`.

### File System plugin
Install [LittleFS plugin](https://github.com/earlephilhower/arduino-esp8266littlefs-plugin) to be able to pack sketch data folder into LittleFS filesystem image, and upload the image to ESP8266 flash memory.

### DHT sensor
Install [DHT](https://github.com/RobTillaart/DHTlib).
To include this library, go to `Sketch > Include Library > Manage Libraries`, search for `DHTlib by Rob Tillaart` and install it.

#### Installation
- Download the latest release (.vsix file) from (https://github.com/earlephilhower/arduino-littlefs-upload/releases).
- Create the `plugins` folder: `mkdir -p ~/.arduinoIDE/plugins`
- Copy the .vsix file you downloaded: `mv ~/Downloads/arduino-littlefs-upload-*.vsix ~/.arduinoIDE/plugins/`
- Restart Arduino IDE. 

#### Usage
- Open a sketch (or create a new one and save it).
- Go to sketch directory (choose Sketch > Show Sketch Folder).
- Create a directory named `data` and any files you want in the file system there.
- Make sure you have selected a board, port, and closed Serial Monitor.
- Select *Tools > ESP8266 LittleFS Data Upload* menu item. This should start uploading the files into ESP8266 flash file system.
  When done, IDE status bar will display LittleFS Image Uploaded message. Might take a few minutes for large file system sizes.

## HOW TO
After having installed all the library and plugins listed above (EasyDDNS, ESPAsyncWebServer, LittleFS) you need to:
- copy the file `config/user_data_example.h`, rename it as `config/user_data.h`, and place your data. Notice that this private data can not be commited in this repo since the `config/user_data.h` file is listed in the [.gitignore](https://git-scm.com/docs/gitignore) file
- select the correct serial port: *Tools->Port*
<!-- - select *Tools > ESP8266 LittleFS Data Upload* to upload the data folder files onto your device (index.html, style.css) -->
- [Ctrl] + [Shift] + [P], then "Upload LittleFS to Pico/ESP8266/ESP32" to upload the data folder files onto your device (index.html, style.css)
- upload the "autonomous_gardening.ino" sketch
 
<!-- TO-DO -->
<!-- - Mention token or other auth methods for DDNS -->

## Useful links
Set a LAN static IP on ESP8266: https://randomnerdtutorials.com/esp8266-nodemcu-static-fixed-ip-address-arduino/

Build a Web Server using SPIFFS: https://watchitgroup.wixsite.com/website/post/part-4-lesson-2-nodemcu-web-server-using-spiffs-and-dht11-sensor
Since SPIFFS has been deprecated, I substitued every SPIFFS instance with the more modern LittleFS according to [this link](https://microcontrollerslab.com/littlefs-introduction-install-esp8266-nodemcu-filesystem-uploader-arduino/).
