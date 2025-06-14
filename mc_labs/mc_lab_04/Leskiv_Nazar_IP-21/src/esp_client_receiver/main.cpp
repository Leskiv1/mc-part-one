#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

const char* mqtt_server = "192.168.0.101";
const int mqtt_port = 1885;
const char* mqtt_topic = "weather/data";

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Отримано з [");
    Serial.print(topic);
    Serial.print("]: ");

    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
}

void connectToMQTT() {
    while (!client.connected()) {
        Serial.print("Підключення до MQTT... ");
        if (client.connect("ESP8266Subscriber")) {
            Serial.println("OK");
            client.subscribe(mqtt_topic);
        } else {
            Serial.print("помилка, rc=");
            Serial.print(client.state());
            Serial.println(" спроба через 5 секунд");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);

    WiFiManager wm;
    bool res = wm.autoConnect("ESP8266_Config");

    if (!res) {
        Serial.println("Не вдалося підключитися, перезавантаження...");
        ESP.restart();
    }

    Serial.println("WiFi підключено!");
    Serial.print("IP адреса: ");
    Serial.println(WiFi.localIP());

    // MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
        connectToMQTT();
    }
    client.loop();
}

