# ESP32 BME280 -> MQTT

Dieses Projekt misst Temperatur und Luftfeuchtigkeit mit einem BME280-Sensor an einem ESP32 und sendet die Messwerte alle 10 Minuten per MQTT an einen Homeserver.

Inhalt

- `ESP32_Barometer.ino` — Arduino-Sketch

Benötigte Bibliotheken

- Adafruit BME280 (inkl. Adafruit Unified Sensor)
- PubSubClient

Installation der Bibliotheken

1. Öffne die Arduino IDE.
2. Gehe zu Sketch -> Bibliothek einbinden -> Bibliotheken verwalten.
3. Suche und installiere "Adafruit BME280" und "PubSubClient".

Wiring (I2C)

- BME280 VCC -> 3.3V
- BME280 GND -> GND
- BME280 SDA -> GPIO21 (Default ESP32 SDA)
- BME280 SCL -> GPIO22 (Default ESP32 SCL)

Konfiguration

1. Öffne `ESP32_Barometer.ino`.
2. Trage dein WLAN und MQTT-Server in den Defines am Anfang der Datei ein:
   - `WIFI_SSID`, `WIFI_PASSWORD`
   - `MQTT_SERVER`, `MQTT_PORT`, ggf. `MQTT_USER`, `MQTT_PASS`
   - `TOPIC_PREFIX` (z. B. `home/esp32/bme280`)

MQTT-Topics

- Temperatur: `<TOPIC_PREFIX>/temperature` (Retained = true)
- Luftfeuchtigkeit: `<TOPIC_PREFIX>/humidity` (Retained = true)
- JSON-Gesamtzustand: `<TOPIC_PREFIX>/state` (Retained = true)

Upload-Anleitung

1. Wähle in der Arduino IDE unter Tools die passende ESP32-Platine (z. B. "ESP32 Dev Module").
2. Wähle den richtigen Port.
3. Klicke auf Upload.

Hinweise

- Der Sketch benutzt I2C-Adresse 0x76, falls dein Modul 0x77 verwendet, versucht der Sketch automatisch 0x77.
- Interval ist auf 10 Minuten gesetzt. Zum Testen kannst du `PUBLISH_INTERVAL_MS` temporär (z. B. 10000 ms) verkürzen.
- Wenn du Home Assistant nutzt, kannst du die Topics direkt als Sensoren einbinden oder MQTT Discovery nutzen (erweiterbar).

Fehlerbehebung

- Keine Verbindung zu WLAN: SSID/Passwort prüfen, Abstand zum Router, evtl. Reboot des ESP32.
- Sensor nicht gefunden: Verkabelung prüfen, andere I2C-Adresse testen.

Wenn du möchtest, kann ich eine `config.h`-Datei anlegen, die WLAN- und MQTT-Daten getrennt hält, oder MQTT-Discovery für Home Assistant ergänzen.
