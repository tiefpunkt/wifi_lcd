#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#include <MQTT.h>
#include <LiquidCrystal.h>
#include "config.h"


WiFiClient wifiClient;
PubSubClient mqtt_client( wifiClient );
LiquidCrystal lcd(12, 13, 16, 5, 4, 14);

#define BUTTON_PIN 0
int buttonState;
int lastButtonState = LOW;
long lastDebounceTime = 0;
long debounceDelay = 50;

int connectState = 0;

#define NO_OF_PAGES 8

struct page {
  char name[9];
  char text[35]; //2x16 + 2 linebreaks + \0
  long last_update;
} pages[NO_OF_PAGES];

int current_page = -1;

void writeToLCD(char* text) {
  lcd.clear();
  lcd.setCursor(0,0);
  int line_pos = 0;
  for (int i = 0; i< strlen(text);i++) {
    if (text[i] == '\n') {
      line_pos = 0;
      i++;
      lcd.setCursor(0,1);
      
    }
    if (line_pos == 16) {
      line_pos = 0;
      lcd.setCursor(0,1);
    }
    
    lcd.print(text[i]);
    line_pos++;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  payload[length] = 0;
  
  if (strcmp(topic, MQTT_TOPIC_BASE "/text") == 0) {
    current_page = -1;
    writeToLCD(reinterpret_cast<char*>(payload));
  }
  
  else if (strncmp(topic, MQTT_TOPIC_BASE "/page/", strlen(MQTT_TOPIC_BASE "/page/")) == 0) {
    Serial.println("== Page handler ==");
    // We have a page command
    // Only look at the last part of the topic now
    topic += strlen(MQTT_TOPIC_BASE "/page/");
    char *pagename = strtok(topic, "/");
    char *command = strtok(NULL, "/");
    
    Serial.print("Page: ");
    Serial.println(pagename);
    Serial.print("Command: ");
    Serial.println(command);
    
    if (strcmp(command, "text") == 0) {
      Serial.print("Setting text for page ");
      Serial.println(pagename);
      int page_idx = -1;
      // Does page already exist?
      for (int i = 0; i<NO_OF_PAGES;i++) {
        if (strcmp(pagename,pages[i].name) == 0) {
          page_idx = i;
          Serial.print("Page found, index ");
          Serial.println(page_idx);
          break;
        }
      }

      // Page not found? - Find an empty page
      if (page_idx == -1) {
        Serial.println("Page not found");
        for (int i = 0; i<NO_OF_PAGES;i++) {
          if (strlen(pages[i].text) == 0) {
            strcpy(pages[i].name,pagename);
            page_idx = i;
            Serial.print("Empty page found, index ");
            Serial.println(page_idx);
            break;
          }
        }
      }

      // no empty page? overwrite the oldest page
      if (page_idx == -1) {
        Serial.println("No empty page found");
        page_idx=0;
        for (int i = 1; i<NO_OF_PAGES;i++) {
          if (pages[i].last_update < pages[page_idx].last_update) {
            page_idx = i;
          }
        }
        Serial.print("Overwriting oldest page ");
        Serial.println(pages[page_idx].name);
        strcpy(pages[page_idx].name,pagename);
      }
      strcpy(pages[page_idx].text, reinterpret_cast<char*>(payload));
      pages[page_idx].last_update = millis();
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println();
  pinMode(BUTTON_PIN, INPUT);
  lcd.begin(16, 2);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WLAN ");
  writeToLCD("Connecting...");
  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(" connected");
  writeToLCD("WiFi connected");

  mqtt_client.setServer(mqtt_broker, 1883);
  mqtt_client.setCallback(callback);  

}

void checkConnect() {
  uint32_t beginWait = millis();
  Serial.println("MQTT reconnecting.");
  while (millis() - beginWait < 1000) {
    delay(100);
    if ( WiFi.status() == WL_CONNECTED ) {
      connectState = 1;

      mqtt_client.loop();
      mqtt_client.disconnect();
      mqtt_client.loop();
      delay(100);
      mqtt_client.connect("lcd01",mqtt_user, mqtt_pass);
      mqtt_client.loop();
      delay(100);

      if ( mqtt_client.connected() ) {
        Serial.println("connected");
        mqtt_client.subscribe(MQTT_TOPIC_BASE "/#");
        writeToLCD("MQTT connected");
        return;
      }
    }
  }
  Serial.print(".");
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        current_page++;
        if (current_page == NO_OF_PAGES) {
          current_page = 0;
        }
        Serial.print("Showing page ");
        Serial.println(current_page);
        writeToLCD(pages[current_page].text);
        lcd.setCursor(15,1);
        lcd.print(current_page+1);
      }
    }
  }

  if ( WiFi.status() != WL_CONNECTED || connectState == 0 || mqtt_client.connected() != 1 ) {
    connectState = 0;
    checkConnect();
  } else {
    mqtt_client.loop();
  }

  lastButtonState = reading;
  delay(10);
}
