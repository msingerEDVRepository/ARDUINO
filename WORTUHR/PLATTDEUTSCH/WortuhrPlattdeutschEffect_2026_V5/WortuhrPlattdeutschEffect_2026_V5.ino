// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WORDCLOCK op PLATTDÜÜTSCH by Michael Singer / 2025-12-20
// V 1.0
// Containing :
// WIFI-Scan (max. 5 SSID's) and start WEB-UI with a filled listbox of existing networks
// HTTP REQUEST communication (without WEBSocket)
// for ESP32 (only!) using the EEPROM for storing the basic ceredentials (no more ESP8266 MCU's)
// the main LED / Clock steering is re-located into a separate "Header-file" (LEDCLOCK_PLATT.H)
//
// V 2.0
// Seconds blinking implemented
// Night-SwitchOFF corrected
// Red Blinking = Wait for WiFi Connection
// Green Blinking = Wait for NTP Server found / Time from the Internet retrieved
//
// V 3.0
// AUTOMATIC RECONNECT LOGIC implemented
// incl. EVENT Handler and Watchdog
//
// V 4.0
// Preferences Logic implemented for being able to store the WIFI-Disconnect reason (Code)
//
// V 5.0 (2026-11-23)
// - Ein-/Ausblend-Effekte aus der Kölsch-Variante übernommen (Rain-In, Fly-In/-Out,
//   Letter-by-Letter-Hide). Die Web-UI-Checkbox "Sekunden blinken" schaltet jetzt die
//   Effekte AN/AUS.
// - Sekundenblinken des Logos (LED 60) komplett auskommentiert (Aufgabe 2)
// - Angepasst an ESP32-Core 3.x/4.x ("esp_sntp.h" statt "sntp.h"),
//   ArduinoJson 7 (JsonDocument statt StaticJsonDocument) und FastLED 3.10.x
// - Web-UI bleibt während Countdown, Effekten und Verbindungsaufbau erreichbar
// - Passwort im EEPROM an eigene Adresse verlegt (320 statt 200): lange Passwörter
//   (> 29 Zeichen) haben vorher die Farb- und Nacht-Einstellungen überschrieben.
//   Ein in V4 gespeichertes Passwort wird beim ersten Start automatisch übernommen.
// - weitere Korrekturen: siehe Kommentare mit "FIX"
// - Verbesserungen (2026-11-26, wie Hochdeutsch V5):
//   (1) WLAN-Passwort im seriellen Monitor nur noch maskiert (******** + Länge)
//   (2) WLAN-Historie in der Web-UI: deutsche Überschrift, zentriert, auch ohne gültige WLAN-Daten
//   (3) Passwort-Eingabefeld verdeckt, mit Checkbox "Kennwort wiesen"
//   (4) Erfolgsmeldung nur bei Erfolg, sonst Fehlermeldung; Passwort nicht mehr in der Browser-Konsole
//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#include <Arduino.h>

#include <EEPROM.h>

#include <WiFi.h>                 // needed to connect to WiFi (for ESP32)
#include <WebServer.h>            // needed to create a simple webserver (make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error)

#include <ArduinoJson.h>          // used library "ArduinoJSON" by Benoit Blanchon / Version 7.4.x - needed for JSON encapsulation (send multiple variables with one string)

#include <Preferences.h>          // New Store logic of an ESP32 MCU (better than EEPROM)
Preferences wifiPrefs;            // New Instance for being able to store the WIFI-Connection Errors

// INCLUDE - language specific Header files (for the LED CLOCK Steering and the WEB-UI)
// +++++++++ include Start ++++++++++++++++++++++++++++++++
#include "HTML_PLATT.h"           // include  WEB-UI
#include "LEDCLOCK_PLATT.h"       // include LED CLOCK run
// +++++++++ include Ende ++++++++++++++++++++++++++++++++

// ACCESS POINT -  Konfiguration
const char* ssidAP = "MIENKLOCK";
const char* passwordAP = "mienklock";         // Only simple  password required (8 digits is minimum)

unsigned long autoConnectStartTime = 120000;     // Start intervall = 2 Minuten

unsigned long GET_TIME_interval = 10000;         //  every 10. sec // Network time protocol     --> 6 times a minute
// AUFGABE 2: Variablen für das (auskommentierte) Sekundenblinken
// unsigned long BLINK_interval = 1000;             // every second the LOGO should Blink

unsigned long previousTIMEcall  = 0;          // we use the "millis()" command for time reference and this will output an unsigned long
// unsigned long previousSECOND  = 0;            // we use the "millis()" command for time reference and this will output an unsigned long

bool receivedClockStart = false;
bool countdownDone = false;

String jsArray = "";

// bool LEDBlinkingToggle = true;

// +++++++++++++++++++ WEB-Server objects +++++++++++++++
WebServer server(80);
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++

String foundNetworkSSID[5];
int foundNetworkRSSI[5];
int networkCount = 0;

// +++++++++++++++ START EEPROM Values +++++++++++++++++++++++++++

String receivedSSID = "";
String receivedPASSWORD = "";
int receivedRED = 50;
int receivedGREEN = 50;
int receivedBLUE = 50;
bool receivedNIGHT_JN = false;
int receivedNIGHTSTART = 0;
int receivedNIGHTEND = 0;
bool receivedEFFECTS_JN = false;

bool invalidCharacterExist = false;

String savedSSID = "";
String savedPASSWORD = "";
int savedRGB_RED = 50;
int savedRGB_GREEN = 50;
int savedRGB_BLUE = 50;
bool savedNIGHT_JN = false;
int savedNIGHT_START = 0;
int savedNIGHT_END = 0;
bool savedEFFECTS_JN = false;

#define EEPROM_SIZE 512
// EEPROM Adressen
#define ADDR_SSID 0               // 1 Byte Länge + max. 32 Zeichen  (0 - 32)
#define ADDR_PASSWORD_V4 200      // alte Adresse bis V4 - nur noch zum einmaligen Übernehmen gelesen
#define ADDR_LED_RED 230
#define ADDR_LED_GREEN 240
#define ADDR_LED_BLUE 250
#define ADDR_NIGHTJN 260
#define ADDR_NIGHT_START 270
#define ADDR_NIGHT_END 290
#define ADDR_EFFECTSJN 310        // bis V4: "Sekunden blinken" (JN)
#define ADDR_PASSWORD 320         // 1 Byte Länge + max. 64 Zeichen  (320 - 384)

#define MAX_SSID_LEN      32      // WLAN-Standard
#define MAX_PASSWORD_LEN  64      // WPA2-Maximum

// +++++++++++++++++++ Ende EEPROM Values +++++++++++++++++++++++

unsigned long autoConnectStart = 0;
unsigned long clientConnectTimer = 0;

bool autoConnectStarted = false;
bool webClientActive = false;

// --- Konfiguration WIFI incl. Error-Codes ( in Preferences) ----------------------------------

#define WIFI_LOG_SIZE   10            // Anzahl gespeicherter Ereignisse
#define WIFI_LOG_NS     "wifi"        // Namespace in den Preferences

// "volatile", weil der WiFi-Event-Handler in einem eigenen Task läuft und das Flag
// von dort aus verändert (vorher: "extern bool ... = false" -> Compiler-Warnung)
volatile bool WIFIConnected = false;

// --- Hilfsfunktion: Wifi Disconnects - Reason-Code -> Text  // START -------------------------

const char* wifiReasonToText(uint8_t r) {
  switch (r) {
    case   1: return "UNSPECIFIED";
    case   2: return "AUTH_EXPIRE";
    case   3: return "AUTH_LEAVE";
    case   4: return "ASSOC_EXPIRE";
    case   5: return "ASSOC_TOOMANY";
    case   6: return "NOT_AUTHED";
    case   7: return "NOT_ASSOCED";
    case   8: return "ASSOC_LEAVE";
    case   9: return "ASSOC_NOT_AUTHED";
    case  15: return "4WAY_HANDSHAKE_TIMEOUT";
    case  16: return "GROUP_KEY_UPDATE_TIMEOUT";
    case 200: return "BEACON_TIMEOUT";
    case 201: return "NO_AP_FOUND";
    case 202: return "AUTH_FAIL";
    case 203: return "ASSOC_FAIL";
    case 204: return "HANDSHAKE_TIMEOUT";
    case 205: return "CONNECTION_FAIL";
    default:  return "UNKNOWN";
  }
}
// --- Hilfsfunktion: Wifi Disconnects - Reason-Code -> Text // ENDE -------------------------

void writeStringToEEPROM(int addrOffset, const String &strToWrite, unsigned int maxLen) {
  // FIX: Länge begrenzen, damit ein zu langer Text nie in die nachfolgenden
  // EEPROM-Bereiche (Farben, Nachtabschaltung ...) hineinschreibt
  unsigned int len = strToWrite.length();
  if (len > maxLen) len = maxLen;

  EEPROM.write(addrOffset, (byte)len);

  for (unsigned int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}

// Liest einen String aus dem EEPROM. Bei ungültigen Zeichen wird
// "invalidCharacterExist" auf true gesetzt (aber NIE zurückgesetzt - das macht
// der Aufrufer, siehe loadCredentials()).
String readStringFromEEPROM(int addrOffset, unsigned int maxLen) {
  unsigned int newStrLen = EEPROM.read(addrOffset);

  // Schutz gegen ungültige/undefinierte EEPROM-Inhalte (z.B. frisch geflashtes
  // Board mit unbeschriebenem Flash = 0xFF = 255): Länge auf ein sicheres
  // Maximum begrenzen, BEVOR das Stack-Array angelegt wird.
  if (newStrLen > maxLen) {
    newStrLen = maxLen;
    invalidCharacterExist = true;
  }

  Serial.print("read EEPROM Offset Address: "); Serial.println(addrOffset);
  Serial.print("read EEPROM Storage Length: "); Serial.println(newStrLen);

  char data[newStrLen + 1];

  for (unsigned int i = 0; i < newStrLen; i++) {

    data[i] = EEPROM.read(addrOffset + 1 + i);

    // FIX: als "unsigned" auswerten - sonst wurden Umlaute (UTF-8-Bytes > 127)
    // als negative Werte und damit fälschlich als "ungültig" erkannt
    uint8_t asciiValue = (uint8_t)data[i];

    // Prüfung auf ungültige Zeichen
    // https://www.asciitable.com
    if (asciiValue < 32 || asciiValue == 255) {
      invalidCharacterExist = true;
      Serial.println("EEPROM-Charakter Nr: " + String(i) + " INVALID! (AsciiValue: " + String(asciiValue) + ")");
      newStrLen = i;
      break;
    }

  }

  data[newStrLen] = '\0';

  return String(data);
}

// Liest SSID + Passwort aus dem EEPROM und setzt "invalidCharacterExist".
// FIX: vorher setzte das Lesen des Passworts das Flag wieder zurück, sodass eine
// kaputte SSID nicht erkannt wurde.
void loadCredentials() {
  invalidCharacterExist = false;
  savedSSID = readStringFromEEPROM(ADDR_SSID, MAX_SSID_LEN);
  bool ssidInvalid = invalidCharacterExist;

  invalidCharacterExist = false;
  savedPASSWORD = readStringFromEEPROM(ADDR_PASSWORD, MAX_PASSWORD_LEN);
  bool passwordInvalid = invalidCharacterExist;

  // Einmalige Übernahme des Passworts von der alten V4-Adresse (200)
  if (passwordInvalid || savedPASSWORD.length() == 0) {
    invalidCharacterExist = false;
    String oldPassword = readStringFromEEPROM(ADDR_PASSWORD_V4, MAX_PASSWORD_LEN);

    if (!invalidCharacterExist && oldPassword.length() > 0) {
      Serial.println("Passwort von alter EEPROM-Adresse (V4) übernommen.");
      savedPASSWORD = oldPassword;
      writeStringToEEPROM(ADDR_PASSWORD, savedPASSWORD, MAX_PASSWORD_LEN);
      EEPROM.commit();
      passwordInvalid = false;
    }
  }

  invalidCharacterExist = ssidInvalid || passwordInvalid;
}

// Liest alle übrigen Einstellungen aus dem EEPROM
void loadSettings() {
  savedRGB_RED = EEPROM.read(ADDR_LED_RED);
  savedRGB_GREEN = EEPROM.read(ADDR_LED_GREEN);
  savedRGB_BLUE = EEPROM.read(ADDR_LED_BLUE);
  savedNIGHT_JN = EEPROM.read(ADDR_NIGHTJN);
  savedNIGHT_START = EEPROM.read(ADDR_NIGHT_START);
  savedNIGHT_END = EEPROM.read(ADDR_NIGHT_END);
  savedEFFECTS_JN = EEPROM.read(ADDR_EFFECTSJN);
}

// FIX: Text sicher in einen JavaScript-String ("...") einbetten. Vorher hat ein
// Anführungszeichen oder Backslash in SSID/Passwort die komplette Web-Seite zerstört.
// (1) Passwort für die Ausgabe im seriellen Monitor maskieren: statt Klartext nur
// Sternchen + Länge, z.B. "******** (20 Zeichen)" - so lässt sich weiterhin prüfen,
// OB ein Passwort gespeichert ist, ohne es preiszugeben.
String maskPassword(const String &pw) {
  if (pw.length() == 0) return "(leer)";
  return "******** (" + String(pw.length()) + " Zeichen)";
}

String jsEscape(const String &s) {
  String out;
  out.reserve(s.length() + 8);
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '\\' || c == '"') { out += '\\'; out += c; }
    else if (c == '<')         { out += "\\u003c"; }       // verhindert "</script>" im Text
    else if ((uint8_t)c < 32)  { /* Steuerzeichen weglassen */ }
    else                       { out += c; }
  }
  return out;
}

// Zahl aus dem JSON lesen - egal ob als Zahl oder als Text ("123") gesendet
int jsonInt(JsonVariantConst v, int defaultValue) {
  if (v.is<int>()) return v.as<int>();
  if (v.is<const char*>()) return atoi(v.as<const char*>());
  return defaultValue;
}

// Hilfsfunktion - Check auf doppelte Netzwerknamen (z.B. auf versch. Kanälen)
bool isDuplicate(String ssid, String* arr, int count) {
  for(int i=0;i<count;i++) if(arr[i]==ssid) return true;
  return false;
}

// WLAN Scan
void scanNetworks() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  int netWorks = WiFi.scanNetworks();
  networkCount = 0;

  if(netWorks > 0){

    String tempSSID[netWorks];
    int tempRSSI[netWorks];

    for(int i=0;i < netWorks;i++){
      tempSSID[i] = WiFi.SSID(i);
      tempRSSI[i] = WiFi.RSSI(i);
    }

    for(int i=0;i < netWorks-1;i++){

      for(int j=i+1;j < netWorks;j++){

        if(tempRSSI[j]>tempRSSI[i]){

          int r=tempRSSI[i];
          tempRSSI[i]=tempRSSI[j];
          tempRSSI[j]=r;

          String s=tempSSID[i];
          tempSSID[i]=tempSSID[j];
          tempSSID[j]=s;
        }
      }
    }

    // Sort FoundNetworks decreasing (best Network Signal strength is on top) and eliminate duplicates
    for(int i=0;i<netWorks && networkCount<5;i++){
      // versteckte Netzwerke (leere SSID) nicht anzeigen
      if(tempSSID[i].length() > 0 && !isDuplicate(tempSSID[i], foundNetworkSSID, networkCount)){

        foundNetworkSSID[networkCount] = tempSSID[i];
        foundNetworkRSSI[networkCount] = tempRSSI[i];

        // Ausgabe der gefundenen Netzwerke in der Reihenfolge der Signalstaerke
        Serial.println("foundNetworkSSID : " + String(foundNetworkSSID[networkCount]) + " foundNetworkRSSI: " + String(foundNetworkRSSI[networkCount]) );
        networkCount++;
      }
    }
  }
  WiFi.scanDelete();      // Scan-Ergebnisse freigeben
  WiFi.mode(WIFI_OFF);
}

// ++++++  Ausgabe der WIFI Disconnect Information in eine HTML +++++++++
String getDisconnectLog() {
  wifiPrefs.begin(WIFI_LOG_NS, true);
  uint8_t idx = wifiPrefs.getUChar("idx", 0);

  String out =
    "<table border='1' cellspacing='0' cellpadding='4' style='margin:0 auto'>"
    "<tr><th>#</th><th>Zeit&nbsp;[s]</th><th>Code</th><th>Grund</th></tr>";

  uint8_t nr = 1;
  for (uint8_t i = 0; i < WIFI_LOG_SIZE; i++) {
    uint8_t pos = (idx + i) % WIFI_LOG_SIZE;
    char keyR[6], keyT[6];
    snprintf(keyR, sizeof(keyR), "r%u", pos);
    snprintf(keyT, sizeof(keyT), "t%u", pos);

    uint8_t  r  = wifiPrefs.getUChar(keyR, 0);
    uint32_t ts = wifiPrefs.getULong(keyT, 0);

    if (r != 0) {
      out += "<tr><td>";
      out += nr++;
      out += "</td><td>";
      out += ts;
      out += "</td><td>";
      out += r;
      out += "</td><td>";
      out += wifiReasonToText(r);
      out += "</td></tr>";
    }
  }
  out += "</table>";
  wifiPrefs.end();

  if (nr == 1) return "<i>Keine Einträge.</i>";
  return out;

}
// ++++++  ENDE - Ausgabe der WIFI Disconnect Information in eine HTML +++++++++

// Webserver
void handleRoot() {

  webClientActive = true;  // WEB-Client hat Seite geöffnet → AutoConnect stoppen

  if (!invalidCharacterExist) {
    jsArray = "[\"" + jsEscape(savedSSID) + "\",";
  } else {
    jsArray = "[\"... dien Netzwark ... \",";
  }

  for(int i=0;i<networkCount;i++){
    jsArray += "\"" + jsEscape(foundNetworkSSID[i]) + "\"";
    if(i<networkCount-1) jsArray+=",";
  }

  jsArray += "]";

  String html = String(htmlPage);
  html.replace("%%NETWORKS%%", jsArray);

  if (!invalidCharacterExist) {
    html.replace("%%PASSWORD%%", "\"" + jsEscape(savedPASSWORD) + "\"");
    html.replace("%%RED%%", String(savedRGB_RED));
    html.replace("%%GREEN%%", String(savedRGB_GREEN));
    html.replace("%%BLUE%%", String(savedRGB_BLUE));
    html.replace("%%NIGHTJN%%", String(savedNIGHT_JN));
    html.replace("%%NIGHTSTART%%", String(savedNIGHT_START));
    html.replace("%%NIGHTEND%%", String(savedNIGHT_END));
    html.replace("%%EFFECTSJN%%", String(savedEFFECTS_JN));
    html.replace("%%WIFILOG%%", getDisconnectLog());
  } else {
    html.replace("%%PASSWORD%%", "\"\"");      // leer -> Platzhalter "... dien Passwöör ..." im Eingabefeld
    html.replace("%%RED%%", "100");
    html.replace("%%GREEN%%", "100");
    html.replace("%%BLUE%%", "100");
    html.replace("%%NIGHTJN%%", "true");
    html.replace("%%NIGHTSTART%%", "0");
    html.replace("%%NIGHTEND%%", "6");
    html.replace("%%EFFECTSJN%%", "false");
    html.replace("%%WIFILOG%%", getDisconnectLog());     // (2) Historie auch ohne gültige WLAN-Daten
  }

  server.send(200,"text/html", html);

}


void handleSave(){

  if(!server.hasArg("plain")){
    server.send(400, "text/plain", "No body");
    return;
  }

  String body = server.arg("plain");
  JsonDocument jsonReceive;                       // ArduinoJson 7: Größe wird automatisch verwaltet
  DeserializationError error = deserializeJson(jsonReceive, body);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    // FIX: Browser bekommt jetzt eine Antwort (vorher wartete er ins Leere)
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  // JSON string was received correctly, so information can be retrieved:
  // (Werte werden auf gültige Bereiche begrenzt)
  receivedRED   = constrain(jsonInt(jsonReceive["rgb_red"], 50), 0, 255);
  receivedGREEN = constrain(jsonInt(jsonReceive["rgb_green"], 50), 0, 255);
  receivedBLUE  = constrain(jsonInt(jsonReceive["rgb_blue"], 50), 0, 255);

  receivedSSID     = jsonReceive["ssid"] | "";
  receivedPASSWORD = jsonReceive["password"] | "";

  receivedNIGHT_JN   = jsonReceive["nightJN"] | false;
  receivedNIGHTSTART = constrain(jsonInt(jsonReceive["nightStart"], 0), 0, 23);
  receivedNIGHTEND   = constrain(jsonInt(jsonReceive["nightEnd"], 6), 0, 23);
  receivedEFFECTS_JN = jsonReceive["effectsJN"] | false;

  receivedClockStart = jsonReceive["clockStart"] | false;

  EEPROM.write(ADDR_LED_RED, receivedRED);
  EEPROM.write(ADDR_LED_GREEN, receivedGREEN);
  EEPROM.write(ADDR_LED_BLUE, receivedBLUE);

  writeStringToEEPROM(ADDR_SSID, receivedSSID, MAX_SSID_LEN);
  writeStringToEEPROM(ADDR_PASSWORD, receivedPASSWORD, MAX_PASSWORD_LEN);

  EEPROM.write(ADDR_NIGHTJN, receivedNIGHT_JN);
  EEPROM.write(ADDR_NIGHT_START, receivedNIGHTSTART);
  EEPROM.write(ADDR_NIGHT_END, receivedNIGHTEND);
  EEPROM.write(ADDR_EFFECTSJN, receivedEFFECTS_JN);

  EEPROM.commit();

  loadSettings();
  loadCredentials();

  Serial.println("Gelesene Werte SSID: " + savedSSID + " PW: " + maskPassword(savedPASSWORD) + " im HandleSave.");

  // ++++++++++++++++++++++++++++

  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert RED_RGB_VALUE:" + String(savedRGB_RED));
  Serial.println("Gespeichert GREEN_RGB_VALUE:" + String(savedRGB_GREEN));
  Serial.println("Gespeichert BLUE_RGB_VALUE:" + String(savedRGB_BLUE));
  Serial.println("Ein-/Ausblend-Effekte (JN):" + String(savedEFFECTS_JN));
  Serial.println("NightMode(JN):" + String(savedNIGHT_JN) + " NightStart:" + String(savedNIGHT_START) + " NightEnd:" + String(savedNIGHT_END));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert SSID:" + receivedSSID + " PW:" + maskPassword(receivedPASSWORD));
  Serial.println("Starte Uhr received (JN):" + String(receivedClockStart));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");

  server.send(200,"text/plain","OK");

}

// --- Ringpuffer-Eintrag in Preferences schreiben --------------------
void logDisconnect(uint8_t reason) {
  wifiPrefs.begin(WIFI_LOG_NS, false);

  // aktuellen Schreibindex holen (0..WIFI_LOG_SIZE-1)
  uint8_t idx = wifiPrefs.getUChar("idx", 0);

  // Schlüssel dynamisch zusammenbauen: r0, r1, ... bzw. t0, t1, ...
  char keyR[6], keyT[6];
  snprintf(keyR, sizeof(keyR), "r%u", idx);
  snprintf(keyT, sizeof(keyT), "t%u", idx);

  // nur schreiben, wenn sich der Wert wirklich geändert hat
  // (spart Flash-Zyklen, falls derselbe Reason mehrfach hintereinander kommt)
  uint8_t lastReason = wifiPrefs.getUChar("lastReason", 0);
  if (reason != lastReason) {
    wifiPrefs.putUChar(keyR, reason);
    wifiPrefs.putULong(keyT, (unsigned long)(millis() / 1000));  // Sek seit Boot
    wifiPrefs.putUChar("lastReason", reason);

    idx = (idx + 1) % WIFI_LOG_SIZE;
    wifiPrefs.putUChar("idx", idx);
  }

  wifiPrefs.end();
}

// --- Gespeicherte History auf Serial ausgeben -----------------------
void printDisconnectLog() {
  wifiPrefs.begin(WIFI_LOG_NS, true);  // read-only
  uint8_t idx = wifiPrefs.getUChar("idx", 0);

  Serial.println(F("---- WLAN-Disconnect-History (älteste zuerst) ----"));
  for (uint8_t i = 0; i < WIFI_LOG_SIZE; i++) {
    uint8_t pos = (idx + i) % WIFI_LOG_SIZE;
    char keyR[6], keyT[6];
    snprintf(keyR, sizeof(keyR), "r%u", pos);
    snprintf(keyT, sizeof(keyT), "t%u", pos);

    uint8_t  r  = wifiPrefs.getUChar(keyR, 0);
    uint32_t ts = wifiPrefs.getULong(keyT, 0);
    if (r != 0) {
      Serial.printf("  t=%lus  reason=%3u  %s\n",
                    (unsigned long)ts, r, wifiReasonToText(r));
    }
  }
  Serial.println(F("--------------------------------------------------"));
  wifiPrefs.end();
}

// --- Optional: Log komplett löschen ---------------------------------
void clearDisconnectLog() {
  wifiPrefs.begin(WIFI_LOG_NS, false);
  wifiPrefs.clear();
  wifiPrefs.end();
  Serial.println(F("WLAN-Log gelöscht."));
}

// +++++++++++++++ Auto-Connect ++++++++++++++++++++++++++++

bool autoConnect() {
  if(savedSSID.length()>0 && savedPASSWORD.length()>0){
    Serial.println("Versuche Verbindung zu "+ savedSSID);
    WiFi.mode(WIFI_STA);

    // Diese Settings hier, NACH dem Mode-Wechsel:
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    // FIX: false statt true - die Zugangsdaten liegen ohnehin im EEPROM. Mit "true"
    // schrieb jedes WiFi.begin() (auch die des Watchdogs alle 30 s) in den Flash.
    WiFi.persistent(false);

    WiFi.begin(savedSSID.c_str(), savedPASSWORD.c_str());

    int attempts=0;
    while(WiFi.status()!=WL_CONNECTED && attempts < 20){  // 10s sind oft zu kurz
      server.handleClient();   // Web-UI während des Verbindungsaufbaus erreichbar halten
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if(WiFi.status()==WL_CONNECTED){
      Serial.println("\nVerbunden! IP: "+WiFi.localIP().toString());
      WiFi.softAPdisconnect(true);
      // WIFIConnected wird vom Event-Handler gesetzt
      return true;
    } else {
      Serial.println("\nVerbindung fehlgeschlagen.");
      return false;
    }
  }
  return false;
}

// ++++++++++++++++++++++++++++++

// --- WIFI - Event-Handler ----------------------------------
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {

    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println(F("[WiFi] STA gestartet"));
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println(F("[WiFi] STA mit AP verbunden"));
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print(F("[WiFi] IP erhalten: "));
      Serial.println(WiFi.localIP());
      WIFIConnected = true;
      break;

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println(F("[WiFi] IP verloren"));
      WIFIConnected = false;
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      uint8_t r = info.wifi_sta_disconnected.reason;
      Serial.printf("[WiFi] Disconnect, reason %u (%s)\n",
                    r, wifiReasonToText(r));
      WIFIConnected = false;
      logDisconnect(r);
      // sofortiger Reconnect-Versuch (Watchdog im Loop fängt Folgefehler ab)
      WiFi.reconnect();
      break;
    }

    default:
      break;
  }
}
// ++++++++++++++++ WIFI Event Handler ENDE +++++++++++++

// +++++++++++++++++ END konfiguration +++++++++++++++++++


// ++++++++++++++++++ START SETUP +++++++++++++++++++++++++
void setup() {

  Serial.begin(115200);

  // +++++++++++++ Scanning Local WLAN SSID +++++++++++++++
  scanNetworks();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP,passwordAP);
  Serial.println("AP gestartet: "+String(ssidAP));

  EEPROM.begin(EEPROM_SIZE);

  loadSettings();
  loadCredentials();

  server.on("/",handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  Serial.println("Gelesene Werte SSID: " + savedSSID + " PW: " + maskPassword(savedPASSWORD) + " im setup.");

  // +++++++++++ Init LED-Strip ++++++++++++++++++++++++++
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(LEDstripArray, NUMPIXELS);
  FastLED.setMaxPowerInVoltsAndMilliamps(MAX_POWER_VOLTS, MAX_POWER_MILLIAMPS);   // FastLED dimmt automatisch herunter, wenn zu viel Strom nötig wäre
  FastLED.setBrightness(BRIGHTNESS);

  // ++++++++++ INIT of WORD-Clock Array ++++++++
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    newletter[LEDindicator] = 0;
    shownletter[LEDindicator] = 0;
  }
  // +++++++++++ End of INIT Word-Clock LED Array +++

  // alle LED einschalten, um die LED Color & Brightness zu sehen (die aus dem EEPROM übernommen und dann angepasst wird...)
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
    LEDstripArray[LEDindicator].setRGB( savedRGB_RED, savedRGB_GREEN, savedRGB_BLUE);
  }
  FastLED.show();        // einmal nach der Schleife statt 121 x darin
  oldRED = savedRGB_RED;
  oldGREEN = savedRGB_GREEN;
  oldBLUE = savedRGB_BLUE;

  // +++++++ New EVENT Handler zum Check auf bestehenden WLAN connection ++++++++++
  // Event-Handler registrieren, wirkt automatisch sobald STA aktiv ist

  WiFi.onEvent(onWiFiEvent);

  // Beim Booten einmal anzeigen, was zuletzt - mit dem WIFI Connect - los war:
  printDisconnectLog();

  // ++++++++++++++++++++New EVENT Handler ENDE ++++++++++++++++++++++++++++++++++

  // ++++++++++++++++ AUTOSTART ++++++++++++++++++++++++++
  if(savedPASSWORD.length()>0){
    autoConnectStarted = true;
    autoConnectStart = millis();
    Serial.println("AutoConnect startet in 2 Minuten wenn die WEB-Site nicht vorher aufgerufen wird ...");
  }
  // ++++++ Collect time information from NTP +++++++++
  esp_sntp_servermode_dhcp(true);      // ESP32-Core 3.x/4.x (früher: sntp_servermode_dhcp(1))
  configTzTime(time_zone, ntpServer1, ntpServer2);
}


// ++++++++++++++++++ START LOOP +++++++++++++++++++++++++
void loop() {

  unsigned long now = millis();         // read out the current "time" ("millis()" gives the time in ms since the Arduino started)

  // ++++++++++++++++++++++ Watchdog Start +++++++++++++++++
  // Watchdog für den Fall, dass der automatische Reconnect nicht greift

  static unsigned long lastReconnectAttempt = 0;

  if (!WIFIConnected && WiFi.getMode() == WIFI_STA &&
      (millis() - lastReconnectAttempt > 30000)) {
    lastReconnectAttempt = millis();
    Serial.println("Watchdog: WLAN weg, neuer Versuch...");
    WiFi.disconnect();
    WiFi.begin(savedSSID.c_str(), savedPASSWORD.c_str());
  }
  // +++++++++++++++++ Watchdog Ende ++++++++++++

  server.handleClient();

  // Non-blocking Auto-Connect nach 2 Minuten
  if (receivedClockStart || (autoConnectStarted && !webClientActive  &&  (millis() - autoConnectStart >= autoConnectStartTime))){

    autoConnectStarted = false;

    if(!WIFIConnected && !invalidCharacterExist){

      if (!countdownDone){

        runCountdownPlatt();

        countdownDone = true;

      } else {

        waitForWLANconnect();

      }

      autoConnect();

    }

  }
    // Check every 10 sec. if a WEB-Client is connected/active
  if(!WIFIConnected && now - clientConnectTimer >= 10000){
    Serial.println("Status - WEB-Client connected YES/NO: " + String(webClientActive));

    clientConnectTimer = now;
  }

  if (WiFi.status() == WL_CONNECTED) {

    // Get Time from NTP Server with a refresh of approx. 10 sec (refer to the GET_TIME_INTERVAL Variable)
    if ((now - previousTIMEcall) > GET_TIME_interval) {

      readLocalTime();
      // Connect to NTP Server and get date & time successful
      Serial.print("NTP Server --> timeInfoSuccess = "); Serial.println(timeInfoSuccess);

      if (!timeInfoSuccess){

        waitForTimeSuccess();

      } else {

        runClockPlatt();

      }

      // FIX: "millis()" statt "now" - die Effekte dauern mehrere Sekunden, sonst
      // käme der nächste Aufruf entsprechend zu früh
      previousTIMEcall = millis();

    }

    // AUFGABE 2: Sekundenblinken des Logos (LED 60) komplett auskommentiert.
    // Wieder aktivieren: die folgenden Zeilen einkommentieren (und blinkingLogo() im Header).
    // // Die LED (60) == Holbastelei-LOGO soll im Sekundentakt blinken.
    // // (früher an "savedSECONDS_JN" gekoppelt - diese Checkbox steuert jetzt
    // // die Ein-/Ausblend-Effekte, daher blinkt das Logo nun immer)
    // if ((millis() - previousSECOND) > BLINK_interval) {

      // if (LEDBlinkingToggle) {

        // // der NSCALE - Value = 64 entspricht einer ca. 25%igen Brightness.
        // blinkingLogo(64);
        // LEDBlinkingToggle = false;

      // } else {

        // blinkingLogo(0);
        // LEDBlinkingToggle = true;

      // }

      // previousSECOND = millis();

    // }

  }

  if (!WIFIConnected && webClientActive) {

    // Nicht mehr anzeigen, wenn die Uhr manuell per Klick auf "start Klock" gestartet wurde
    if (!receivedClockStart) {

      // ++++++ Set color to the actual EEPROM stored values - refreshed by new values from WEB-Client +++++
      for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
        LEDstripArray[LEDindicator].setRGB(savedRGB_RED,savedRGB_GREEN,savedRGB_BLUE);
      }

      FastLED.show();

    }

  }

}
