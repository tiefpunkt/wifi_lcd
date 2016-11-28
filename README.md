# WIFI LCD Display

An ESP8266 powered text based LCD display. Connects to an MQTT broker. Supports multiple pages (currently 8). Operated with a single button.

## Hardware
* ESP8266 (I used a Wemos D1 dev board)
* HD44780 based LCD display - 16x2 in this case, other sizes will need code adjustments.
  * Pin assignment:
    * Reset - GPIO 12
	* Enable - GPIO 13
	* D4 - GPIO 16
	* D5 - GPIO 5
	* D6 - GPIO 4
	* D7 - GPIO 14
* A single button, connected to GPIO 0 (with pullup)

## Software
* Copy ```config.h.sample``` to ```config.h``` and adjust settings
* Flash to ESP using Arduino IDE

## Usage
* Message to display immediately: Send text to ```<MQTT_TOPIC_BASE>/text```
* Paging: Send text to ```<MQTT_TOPIC_BASE>/page/<PAGENAME>/text```
  * Page name is limited to 8 characters
  * If a page already exists, it's overwritten
  * If a page does not exist, it's created. If the page buffer is full, the oldest page (in terms of the last update to a page) is overwritten.

## ToDo
* Control page order
* Give pages different importance levels
