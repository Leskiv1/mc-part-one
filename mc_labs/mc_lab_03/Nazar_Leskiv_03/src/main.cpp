#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <time.h>
#include <WiFiManager.h>

// OLED Display (SH1106 1.3")
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Global variables
String temperature = "N/A";
String humidity = "N/A";

uint32_t startupMillis = 0;
uint32_t lastScreenUpdate = 0;
const uint32_t screenUpdateInterval = 500;
uint32_t lastWeatherUpdate = 0;
const uint32_t weatherInterval = 3600000;

time_t savedEpoch = 0;
uint32_t lastSyncMillis = 0;
bool wasWiFiConnected = false;

// ------------------ Display Functions ------------------
void initDisplay() {
  Wire.begin(D2, D1);  // I2C pins for ESP8266
  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_5x8_tf);
  display.drawStr(0, 10, "Starting...");
  display.sendBuffer();
  delay(500);
}

void drawWiFiSignal(bool connected) {
  uint8_t x = 110;
  uint8_t y = 4;
  uint8_t barWidth = 3;
  uint8_t barSpacing = 2;
  uint8_t heights[3] = {4, 8, 12};

  for (uint8_t i = 0; i < 3; i++) {
    uint8_t barX = x + i * (barWidth + barSpacing);
    uint8_t barY = y + (12 - heights[i]);
    if (connected) {
      display.drawBox(barX, barY, barWidth, heights[i]);
    } else {
      display.drawFrame(barX, barY, barWidth, heights[i]);
    }
  }
}

void showWiFiConnecting() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tr);
  display.drawStr(0, 12, "Connecting to WiFi...");
  display.sendBuffer();
}

void showOfflineMessage() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tr);
  display.drawStr(0, 20, "Offline mode");
  display.drawStr(0, 35, "No WiFi found");
  display.sendBuffer();
}

void drawMainScreen(struct tm timeinfo) {
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  char dateStr[16];
  strftime(dateStr, sizeof(dateStr), "%a:%d:%b", &timeinfo);

  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tr);
  display.drawStr(0, 10, dateStr);
  drawWiFiSignal(WiFi.status() == WL_CONNECTED);

  display.setFont(u8g2_font_logisoso24_tr);
  uint16_t strWidth = display.getUTF8Width(timeStr);
  display.drawUTF8((128 - strWidth) / 2, 40, timeStr);

  display.drawHLine(0, 45, 128);
  display.setFont(u8g2_font_6x13_t_cyrillic);

  String streetData = temperature + "/" + humidity;
  uint16_t w = display.getUTF8Width(streetData.c_str());
  display.drawUTF8(128 - w, 62, streetData.c_str());

  display.sendBuffer();
}

// ------------------ WiFi and Time ------------------
bool connectToWiFi(uint32_t timeoutMillis = 120000) {
  WiFiManager wm;
  wm.setTimeout(timeoutMillis / 1000);

  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tr);
  display.drawStr(0, 12, "WiFiManager...");
  display.sendBuffer();

  bool connected = wm.autoConnect("WeatherDisplayAP");

  display.clearBuffer();
  display.drawStr(0, 12, connected ? "WiFi connected!" : "WiFi Timeout!");
  display.sendBuffer();

  return connected;
}

void checkWiFiReconnectAndSyncTime() {
  bool wifiNow = WiFi.status() == WL_CONNECTED;
  if (wifiNow && !wasWiFiConnected) {
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      savedEpoch = mktime(&timeinfo);
      lastSyncMillis = millis();
    }
  }
  wasWiFiConnected = wifiNow;
}

// ------------------ Weather ------------------
void fetchWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[fetchWeather] WiFi OK, fetching...");

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = "https://api.open-meteo.com/v1/forecast?latitude=49.84&longitude=24.03&current=temperature_2m,relative_humidity_2m";

    if (https.begin(client, url)) {
      int httpCode = https.GET();
      Serial.printf("[fetchWeather] HTTP Code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        Serial.println("[fetchWeather] JSON Payload received.");

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          float temp = doc["current"]["temperature_2m"];
          int hum = doc["current"]["relative_humidity_2m"];

          temperature = String(temp, 1) + " C";
          humidity = String(hum) + "%";

          Serial.println("[fetchWeather] Temperature: " + temperature);
          Serial.println("[fetchWeather] Humidity: " + humidity);
        } else {
          Serial.println("[fetchWeather] JSON parse failed.");
          temperature = "N/A";
          humidity = "N/A";
        }
      } else {
        Serial.println("[fetchWeather] Failed to fetch data.");
        temperature = "N/A";
        humidity = "N/A";
      }
      https.end();
    } else {
      Serial.println("[fetchWeather] HTTPS.begin() failed.");
    }
  } else {
    Serial.println("[fetchWeather] WiFi not connected.");
  }
}

// ------------------ Main Logic ------------------
void updateDisplay() {
  struct tm timeinfo;

  if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo)) {
    savedEpoch = mktime(&timeinfo);
    lastSyncMillis = millis();
  } else {
    time_t now = savedEpoch + (millis() - lastSyncMillis) / 1000;
    localtime_r(&now, &timeinfo);
  }

  drawMainScreen(timeinfo);
}

void setup() {
  Serial.begin(115200);
  initDisplay();
  startupMillis = millis();

  bool wifiOK = connectToWiFi();
  if (wifiOK) {
    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      savedEpoch = mktime(&timeinfo);
      lastSyncMillis = millis();
    }
    fetchWeather();
  } else {
    savedEpoch = 0;
    lastSyncMillis = millis();
    showOfflineMessage();
  }
  fetchWeather();
}

void loop() {
  uint32_t currentMillis = millis();

  if (currentMillis - lastScreenUpdate >= screenUpdateInterval) {
    lastScreenUpdate = currentMillis;
    updateDisplay();
  }

  if (currentMillis - lastWeatherUpdate >= weatherInterval) {
    lastWeatherUpdate = currentMillis;
    fetchWeather();
  }

  checkWiFiReconnectAndSyncTime();
}
