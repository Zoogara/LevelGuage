# LevelGuage

NOTE: Some software / hardware providers are blocking websockets on non-secure connections.  Samsung Internet browser is one example.  The workaround is to use another browser (Chrome) which currently doesn't do this.  Longer term I plan on implementing https and a root certificate with long expiry.

A software framework for universal level gauge based on ESP32 and MPU6050 with web based front-end.

Follow these links for [documentation](https://github.com/Zoogara/LevelGuage/wiki/LevelGuage-Documentation) and [build instructions](https://github.com/Zoogara/LevelGuage/wiki/Build-Log-for-$AU70-version-($AU40-without-battery)---coming-soon)

![Interface](https://www.dropbox.com/s/y6bet9axlc7e0zn/ExampleLevel_1_1.jpg?raw=1)

As supplied it includes files and configuration to perform as a level guage for a caravan - but feel free to apply to any other purpose.

Basic circuit for testing:
https://www.circuito.io/static/reply/index.html?solutionId=60c05f86b0ba3a00308cdf74&solutionPath=storage.circuito.io

### SPIFFS

Familiarise yourself with installing and configuring SPIFFS for ESP32 before loading this software.  There are numerous examples and resources on the web.

The SPIFFS partition scheme used for this app is: No OTA (2MB APP / 2MB SPIFFS)

### Libraries

The libraries used in this app are:

  * Adafruit MPU6050 by Adafruit
  * Arduino_JSON by Arduino
  * ESPmDNS by Hristo Gochkov
  
These libraries should be available in the Arduino repository.

Other libraries used are:

  * ESPAsyncWebServer by me-no-dev https://github.com/me-no-dev/ESPAsyncWebServer
  * AsyncTCP by me-no-dev https://github.com/me-no-dev/AsyncTCP
  
Random Nerd Tutorials has a good example of installing and using these libraries to serve web pages from SPIFFS: https://randomnerdtutorials.com/esp32-web-server-spiffs-spi-flash-file-system/
  
### Documentation

See the wiki for documentation: https://github.com/darylsargent/LevelGuage/wiki/

### Future changes:

* Audible tones on level condition

# License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License v3.0 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see https://www.gnu.org/licenses/gpl.html
