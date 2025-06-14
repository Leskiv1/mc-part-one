#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "index_html.h"

const char *ssid = "";
const char *password = "";

ESP8266WebServer server(80);

const uint8_t led1 = D3;
const uint8_t led2 = D4;
const uint8_t led3 = D7;
const uint8_t button = D8;

const uint8_t numLEDS = 3;
const uint16_t timer = 1500;
uint16_t interval = 2000;
uint32_t previousMillis = 0;
uint32_t buttonPressTime = 0;

typedef enum led_e {
  GREEN,
  YELLOW,
  RED
} led_t;

led_t currentLED = GREEN;
bool buttonPressed = false;
bool webButtonPressed = false;
bool serialButtonPressed = false;


void handleButtonPress() {
  webButtonPressed = true;
  buttonPressTime = millis();
  server.send(200, "text/plain", "Button Pressed");
}

void handleButtonRelease() {
  webButtonPressed = false;
  server.send(200, "text/plain", "Button Released");
}

void handleFriendButtonPress() {
  Serial.print('h');
  server.send(200, "text/plain", "Button Pressed");
}

void handleFriendButtonRelease() {
  Serial.print('r');
  server.send(200, "text/plain", "Button Pressed");
}

void handleLedState() {
  server.send(200, "text/plain", String(currentLED));
}


void handleClientRequests() {
  server.on("/", []() { server.send_P(200, "text/html", index_html); });
  server.on("/buttonPress", handleButtonPress);
  server.on("/buttonRelease", handleButtonRelease);
  server.on("/friendButtonPress", handleFriendButtonPress);
  server.on("/friendButtonRelease", handleFriendButtonRelease);
  server.on("/ledState", handleLedState);
  server.begin();
}

void setupHardware() {
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(button, INPUT);
}

void setup() {
  Serial.begin(115200, SERIAL_8N1);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.print(WiFi.localIP());
  setupHardware();
  handleClientRequests();
}

void checkButtonState() {
  bool currentButtonState = digitalRead(button);
  if (currentButtonState != buttonPressed) {
    buttonPressed = currentButtonState;
    buttonPressTime = millis();
  }
}

void updateMode() {
  if ((buttonPressed || webButtonPressed || serialButtonPressed) && millis() - buttonPressTime >= timer) {
    interval = 700;
  } else if (!buttonPressed && !webButtonPressed && !serialButtonPressed) {
    interval = 2000;
  }
}

void nextLED() {
  currentLED = (led_t) ((currentLED + 1) % numLEDS);
  digitalWrite(led1, currentLED == GREEN);
  digitalWrite(led2, currentLED == YELLOW);
  digitalWrite(led3, currentLED == RED);
}

void checkSerial() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    Serial.print(c);
    switch (c) {
      case 'h':
        serialButtonPressed = true;
        buttonPressTime = millis();
        break;
      case 'r':
        serialButtonPressed = false;
        break;
    }
  }
}

void loop() {
  server.handleClient();
  checkButtonState();
  checkSerial();
  updateMode();
  if (millis() - previousMillis >= interval) {
    previousMillis = millis();
    nextLED();
  }
}