
#include <Arduino.h>
#include <Hash.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <WiFiManager.h>
#include <ESP_EEPROM.h>
#include <WebSocketsClient.h>

#define RELAY_PIN D1
#define RED_LED_PIN D0
#define GREEN_LED_PIN D3
#define BLUE_LED_PIN D2
#define BUTTON_PIN D8

#define RESET_HOLD_TIME_MS 3000

#ifndef HOSTNAME
#define HOSTNAME "generic-osc-relay"
#endif


// Wifi Manager
WiFiManager wifiManager;
bool connectionSuccessful = false;
char webSocketHost[40] = "studio@local";
char triggerAddress[40] = "/fog/1/active";
bool shouldSaveConfig = false;
void saveConfigCallback () {
  shouldSaveConfig = true;
}
WiFiManagerParameter webSocketHostParam("websockethost", "the host where the websocket server is running", webSocketHost, 40);
WiFiManagerParameter triggerAddressParam("triggeraddress", "the address to listen on for triggering", triggerAddress, 40);

// websockets
WebSocketsClient webSocket;

// State management
bool readyToReset = 0;
byte buttonValue = 0;
unsigned long buttonChangeMillis = 0;
bool websocketConnected = false;
bool relayState = false;

// write the colors to the leds
void indicatorColor(byte red, byte green, byte blue) {
  digitalWrite(RED_LED_PIN, 255 - red);
  digitalWrite(GREEN_LED_PIN, 255 - green);
  digitalWrite(BLUE_LED_PIN, 255 - blue);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  Serial.printf("websocket event %d\n", type);
  switch(type) {
    case WStype_ERROR:
       Serial.printf("[WSc] error: %s\n", payload);
       break;
    case WStype_DISCONNECTED:
      websocketConnected = false;
      break;
    case WStype_CONNECTED:
      websocketConnected = true;
      char buff[45];
      sprintf(buff, "%s %d", triggerAddress, relayState);
      webSocket.sendTXT(buff);
      break ;
    case WStype_TEXT:
     char *addr = strtok((char *)payload, " ");
     char *value = strtok(NULL, " ");
     int match = strcmp(addr, triggerAddress);
     Serial.printf("addr: %s\n", addr);
     Serial.printf("value: %s\n", value);
     Serial.printf("match: %d\n", match);
     if (match == 0) {
       int val = atoi(value);
       if (val == 1) {
         relayState = true;
       } else {
         relayState = false;
       }
     }
     break ;
  }
}

void resetWifi() {
  wifiManager.erase();
  wifiManager.resetSettings();
  ESP.restart();
}

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  Serial.begin(115200);
  while (!Serial); // wait for Leonardo enumeration, others continue immediately

  /* resetWifi(); */

  // blink the light
  for (byte i = 0 ; i < 5; i++) {
    indicatorColor(255,255,255);
    delay(100);
    indicatorColor(0,0,0);
    delay(100);
  }

  // initialize the filesystem
  EEPROM.begin(80);
  EEPROM.get(0, webSocketHost);
  EEPROM.get(40, triggerAddress);

  // connect to wifi
  indicatorColor(0,0,255);
  WiFi.setHostname(HOSTNAME);
  wifiManager.setClass("invert");
  wifiManager.addParameter(&webSocketHostParam);
  wifiManager.addParameter(&triggerAddressParam);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  bool res = wifiManager.autoConnect(HOSTNAME);

  if(!res) {
    Serial.println("Failed to connect or hit timeout");
    indicatorColor(255, 0, 0);
    connectionSuccessful = false;
  } else {
    indicatorColor(0, 255, 0);
    connectionSuccessful = true;
  }
  if (shouldSaveConfig) {
    Serial.println("saving config to EEPROM");
    strcpy(webSocketHost, webSocketHostParam.getValue());
    strcpy(triggerAddress, triggerAddressParam.getValue());
    EEPROM.put(0, webSocketHost);
    EEPROM.put(40, triggerAddress);
    EEPROM.commit();
  }

  // connect the websocket client
  webSocket.onEvent(webSocketEvent);
  Serial.printf("connecting to %s:8080\n", webSocketHost);
  webSocket.begin(webSocketHost, 8080, "");
}

void loop() {
  // read and manage the reset button
  unsigned long now = millis();
  byte newButtonValue = LOW; //digitalRead(BUTTON_PIN);
  // if the button is held down for 3 seconds, reset the wifi
  if (newButtonValue != buttonValue) {
    if (buttonChangeMillis == 0) {
      buttonChangeMillis = now;
    }
    buttonValue = newButtonValue;
  } else if (newButtonValue == HIGH) {
    if (now - buttonChangeMillis >= RESET_HOLD_TIME_MS) {
      // blink the light to indicate its ready to reset
      for (byte i = 0 ; i < 5 ; i++) {
        indicatorColor(0,0,0);
        delay(100);
        indicatorColor(255, 0, 0);
        delay(100);
      }
      readyToReset = true;
    } else {
      indicatorColor(255, 0, 0);
    }
  } else if (readyToReset && newButtonValue == LOW) {
    indicatorColor(255, 255, 255);
    resetWifi();
  } else {
    if(!connectionSuccessful || !websocketConnected) {
      indicatorColor(255,0,0);
    } else if (newButtonValue == LOW) {
      indicatorColor(0,255,0);
    }
  }

  if (relayState) {
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(RELAY_PIN, LOW);
  }

  webSocket.loop();
}
