/*
  ESP32_Barometer.ino
  Misst Temperatur und Luftfeuchtigkeit mit BME280
  und sendet alle 10 Minuten via MQTT an einen Homeserver.

  Bibliotheken: Adafruit BME280, PubSubClient

  Konfiguration: SSID, Passwort und MQTT-Server in den Defines unten anpassen.
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <PubSubClient.h>

// === Konfiguration ===
const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* MQTT_SERVER = "192.168.1.100"; // IP oder Hostname deines Homeservers
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER   = ""; // optional
const char* MQTT_PASS   = ""; // optional
const char* TOPIC_PREFIX = "home/esp32/bme280"; // Basis-Topic

// Messintervall: 10 Minuten
const unsigned long PUBLISH_INTERVAL_MS = 10UL * 60UL * 1000UL;

Adafruit_BME280 bme; // I2C
WiFiClient  wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastPublish = 0;

void setupWifi() {
  Serial.print("Verbinde mit WLAN");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
    if (millis() - start > 20000) {
      Serial.println("\nWLAN-Verbindung fehlgeschlagen - Neustart in 5s");
      delay(5000);
      ESP.restart();
    }
  }
  Serial.println();
  Serial.print("WLAN verbunden, IP: ");
  Serial.println(WiFi.localIP());
}

void connectMqtt() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  if (mqttClient.connected()) return;

  Serial.print("Verbinde mit MQTT...");
  String clientId = "esp32-bme280-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  unsigned long start = millis();
  while (!mqttClient.connected()) {
    if (MQTT_USER && strlen(MQTT_USER) > 0) {
      if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) break;
    } else {
      if (mqttClient.connect(clientId.c_str())) break;
    }
    Serial.print('.');
    delay(1000);
    if (millis() - start > 15000) {
      Serial.println("\nMQTT-Connect Timeout, versuche WLAN neu zu verbinden");
      setupWifi();
      start = millis();
    }
  }
  Serial.println(" verbunden");
}

void publishSensorData() {
  float temperature = bme.readTemperature();
  float humidity    = bme.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Fehler beim Lesen des Sensors");
    return;
  }

  char topicTemp[64];
  char topicHum[64];
  snprintf(topicTemp, sizeof(topicTemp), "%s/temperature", TOPIC_PREFIX);
  snprintf(topicHum,  sizeof(topicHum),  "%s/humidity", TOPIC_PREFIX);

  char payload[128];
  // JSON-Payload mit zwei Werten (kompakt)
  snprintf(payload, sizeof(payload), "{\"temperature\":%.2f,\"humidity\":%.2f}", temperature, humidity);

  if (!mqttClient.publish(topicTemp, String(temperature, 2).c_str(), true)) {
    Serial.println("Temperatur-Publish fehlgeschlagen");
  }
  if (!mqttClient.publish(topicHum, String(humidity, 2).c_str(), true)) {
    Serial.println("Feuchte-Publish fehlgeschlagen");
  }
  // zusätzlich komplettes JSON auf eigenes Topic
  char topicJson[64];
  snprintf(topicJson, sizeof(topicJson), "%s/state", TOPIC_PREFIX);
  if (!mqttClient.publish(topicJson, payload, true)) {
    Serial.println("JSON-Publish fehlgeschlagen");
  }

  Serial.print("Veröffentlicht: ");
  Serial.println(payload);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("ESP32 BME280 MQTT Sender");

  setupWifi();

  // BME280 initialisieren (I2C Adresse 0x76 oder 0x77)
  if (!bme.begin(0x76)) {
    Serial.println("BME280 nicht gefunden bei Adresse 0x76, versuche 0x77");
    if (!bme.begin(0x77)) {
      Serial.println("BME280 nicht gefunden - Stop");
      while (1) {
        delay(1000);
      }
    }
  }

  connectMqtt();
  lastPublish = millis() - PUBLISH_INTERVAL_MS; // sofort erste Messung
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }
  if (!mqttClient.connected()) {
    connectMqtt();
  }
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;
    publishSensorData();
  }
}
