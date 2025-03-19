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

uint32_t previousMillis = 0;
uint16_t interval = 2000;
uint32_t buttonPressTime = 0;

typedef enum led_e {
    GREEN,
    YELLOW,
    RED
} led_t;

led_t currentLED = GREEN;
bool fastMode = false;
bool buttonPressed = false;
bool webButtonPressed = false;


void handleButtonPress() {
    webButtonPressed = true;
    buttonPressTime = millis();
    server.send(200, "text/plain", "Button Pressed");
}

void handleButtonRelease() {
    webButtonPressed = false;
    server.send(200, "text/plain", "Button Released");
}

void handleClientRequests() {
    server.on("/", []() { server.send_P(200, "text/html", index_html); });
    server.on("/buttonPress", handleButtonPress);
    server.on("/buttonRelease", handleButtonRelease);
    server.begin();
}

void setupHardware() {
    pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);
    pinMode(led3, OUTPUT);
    pinMode(button, INPUT);
}

void connectWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    connectWiFi();
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
    if ((buttonPressed || webButtonPressed) && millis() - buttonPressTime >= 1500) {
        fastMode = true;
        interval = 500;
    } else if (!buttonPressed && !webButtonPressed) {
        fastMode = false;
        interval = 2000;
    }
}

void nextLED() {
    currentLED = (led_t)((currentLED + 1) % 3);
    digitalWrite(led1, currentLED == GREEN);
    digitalWrite(led2, currentLED == YELLOW);
    digitalWrite(led3, currentLED == RED);
}

void loop() {
    server.handleClient();
    checkButtonState();
    updateMode();
    if (millis() - previousMillis >= interval) {
        previousMillis = millis();
        nextLED();
    }
}
