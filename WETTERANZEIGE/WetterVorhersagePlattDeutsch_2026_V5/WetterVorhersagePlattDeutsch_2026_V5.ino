// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WEDDERVORHERSAGE by Michael Singer / 2025-12-23
// V 1.0 
// Containing :
// WIFI-Scan (max. 5 SSID's) and start WEB-UI with a filled listbox of existing networks
// HTTP REQUEST communication (without WEBSocket)
// for ESP32 (only!) using the EEPROM for storing the basic ceredentials (no more ESP8266 MCU's)
// the main LED / Clock steering ode is re-located into a separate "Header-files"
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// New weather-code translations after the feedback of Heinrich Evers (Plattdeutschbeauftragter OstHolstein)
//
// 0         - BANNING VEEL SÜNN  (CLEAR SKY)
// 1         - VEEL SÜNN MIT POOR WULKEN 
// 2         - BETEN SÜNN MIT WULKEN (MAINLY CLEAR)
// 3         - BANNIG VEEL WULKEN (OVERCAST)
// 45        - BETEN NEVEL
// 46,47     - VEEL NEVEL
// 48        - BANNIG VEEL NEVEL (FOG)
// 51        - BETEN FIESELREGEN (LIGHT DRIZZLE)
// 53        - VEEL FIESELREGEN (MODERATE DRIZZLE)
// 55        - BANNIG VEEL FIESELREGEN (HEAVY DRIZZLE)
// 56        - BETEN FRERENDEN FIESELREGEN (LIGHT FREEZING DRIZZLE)
// 57        - BANNIG FRERENDEN FIESELREGEN (DENSE FREEZING DRIZZLE)
// 61        - BETEN REGEN (SLIGHT RAIN)
// 63        - VEEL REGEN (MODERATE RAIN)
// 65        - BANNIG VEEL REGEN (HEAVY RAIN)
// 66        - BETEN FRERENDEN REGEN (LIGHT FREEZING RAIN)
// 67        - BANNIG FRERENDEN REGEN (HEAVY FREEZING RAIN)
// 71        - BETEN SNEE (SLIGHT SNOW)
// 73        - VEEL SNEE (MODERATE SNOW)
// 75        - BANNIG VEEL SNEE (HEAVY SNOW)
// 77        - BETEN SNEEGRIESEL (SNOW GRAINS)
// 80        - AF UN AN BETEN REGEN (SLIGHT RAIN SHOWER)
// 81        - AF UN AN REGEN (MODERATE RAIN SHOWER)
// 82        - AF UN AN BANNIG VEEL REGEN (VIOLENT RAIN SHOWER)
// 85        - BETEN SNEE REGEN (MODERATE RAIN SNOW)
// 86        - BANNIG VEEL SNEE REGEN (HEAVY RAIN SNOW)
// 95        - VEEL WIND MIT DONNERWEDDER (MODERATE HAIL THUNDERSTORM)
// 96-99     - BANNIG STORM MIT DONNERWEDDER (HEAVY HAIL THUNDERSTORM)
//***********************************************************************
// V 2.0  by Michael Singer / 2026-01-03
// Containing :
// NEW Function : Identify  SUNSET from OPEN-METEO (Hour / Minute) 
// Check if SUNSET < CURRENT TIME --> If yes (in case of WeatherCode < 3 only!) 
// ... --> Switch off "Display" because it just makes no sense 
// ... to display "SÜNN" (is shining) after sunset took place"
// May be better --> Display shows sparkling "Stars" (show colored letters randomly)
//***********************************************************************
// V 3.0  by Michael Singer / 2026-04-07
// Containing :
// Changed Function : Geocode is now made via OpenStreetMap
// bacause the GeoCode API of OPEN-METEO is not able to use the ZIP Code (in Germany) as a search-criteria
// which is necessary for being able to find the correct e.g. "Rosengarten" - which exists in Germany multiple times  
//***********************************************************************
// V 5.0  by Michael Singer / 2026-11-25
// Angepasst an die aktuellen Libraries (analog Wortuhr V5):
// - ESP32-Core 3.x/4.x: esp_sntp_servermode_dhcp() statt des veralteten sntp_servermode_dhcp()
// - ArduinoJson 7.x: JsonDocument statt des veralteten StaticJsonDocument<N>
//   (dynamische Größe -> keine "NoMemory"-Fehler mehr bei langen SSID/Passwort/Ortsnamen)
// - handleSave: bei fehlerhaftem JSON wird jetzt "400" an die WEB-UI zurückgemeldet
//   (bisher kam keine Antwort und der Browser wartete bis zum Timeout)
// - Datums-Puffer für die OPEN-METEO URL vergrößert (10 -> 11 Zeichen, snprintf statt sprintf):
//   "JJJJ-MM-TT" + abschließendes '\0' passte nicht in char[10] -> Stack-Überschreibung
// Verbesserungen (Aufgabe 1c):
//   (1)  Tag/Nacht: Stunden nach Mitternacht (Anzeige ab 21 Uhr) werden jetzt als NACHT erkannt,
//        Vergleich mit Sonnenauf-/-untergang minutengenau
//   (2)  Ortssuche (OpenStreetMap) wird bei Fehler alle 10 Minuten wiederholt,
//        nach neuem Ort wird das Wetter sofort neu geladen
//   (3)  Fehlgeschlagener Wetterabruf: alte Werte bleiben erhalten, neuer Versuch nach 1 Minute;
//        solange noch nie Wetterdaten da waren, blinkt die grüne Warte-Anzeige
//   (4)  Suche in der WeatherTable liest nicht mehr über das Tabellenende hinaus
//   (5)  "Morgen" wird über die Systemzeit berechnet (Schaltjahr korrekt)
//   (6)  Zeitabfrage wartet max. 200 ms statt 5 s
//   (7)  WiFi.persistent(false) - WLAN-Daten werden nicht bei jedem Verbinden in den Flash geschrieben
//   (8)  WEB-UI bleibt während Ein-/Ausblendungen, Countdown und Verbindungsaufbau erreichbar
//   (9)  Passwort an neuer EEPROM-Adresse 400 (max. 64 Zeichen, einmalige Übernahme von Adresse 200);
//        alle Texte werden längenbegrenzt gespeichert
//   (10) Passwort im seriellen Monitor nur maskiert, in der WEB-UI als Passwortfeld (mit "wiesen")
//   (11) WEB-UI sendet Zahlen als Zahlen (Number()), ESP32 prüft/begrenzt die Werte
//   (12) WLAN-Abbruch-Historie (Preferences) im seriellen Monitor und in der WEB-UI
//***********************************************************************

#include <Arduino.h>

#include <EEPROM.h>

#include <WiFi.h>                 // needed to connect to WiFi (for ESP32)
#include <WebServer.h>            // needed to create a simple webserver (make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error)

#include <ArduinoJson.h>          // used library "ArduinoJSON" by Benoit Blanchon / Version 7.4.3 - needed for JSON encapsulation (send multiple variables with one string)

#include <Preferences.h>          // (12) ESP32 Preferences - für die WLAN-Abbruch-Historie
Preferences wifiPrefs;

// INCLUDE - language specific Header files (for the LED CLOCK Steering and the WEB-UI)
// +++++++++ include Start ++++++++++++++++++++++++++++++++
#include "HTML_WETTER_PLATT.h"           // include  WEB-UI
#include "LED_WETTER_PLATT.h"            // include LED CLOCK run
// +++++++++ include Ende ++++++++++++++++++++++++++++++++

// ACCESS POINT -  Konfiguration
const char* ssidAP = "MIENWEDDER";
const char* passwordAP = "mienwedder";         // Only simple  password required (7 digits is minimum)

unsigned long autoConnectStartTime = 120000;   // Start intervall = 2 Minuten
unsigned long now = 0;

// We want to periodically send values to the clients, so we need to define an "interval"
// and remember the last time we sent data to the client (with "previousMillis")
unsigned long GET_TIME_interval         = 15000;                //  NETWORT TIME + OPEN METEO         --> every 15 seconds
unsigned long GET_WEATHER_interval      = 600000;               //  every 10 minutes (10 * 1000 * 60 = 600.000 ms) - free license is maximized to 10.000 calls per day // and sketch has 3 API calls per loop.                                                                // resulting in 6 requests per hour * 24h == 144 API calls per day
unsigned long WEATHER_RETRY_interval    = 60000;                //  (3) nach einem fehlgeschlagenen Wetterabruf: neuer Versuch nach 1 Minute
unsigned long DISPLAY_WEATHER_interval  = 10000;                //  Show weather forecast (een, twee, dree stünd'n) --> every 10 seconds

unsigned long previousTIMEcall          = 0;                    // we use the "millis()" command for time reference and this will output an unsigned long
unsigned long previousWeatherCall       = 0;                    // we use the "millis()" command for time reference and this will output an unsigned long
unsigned long previousWeatherDisplay    = 0;                    // we use the "millis()" command for time reference and this will output an unsigned long

bool firstGetWeatherLoop = true;                                // the first GetWeather has to run immediatly after getTime (and not after reaching the GetWeather-Intervall ...)
unsigned int WetterInStund = 1;
bool countdownDone = false;

bool receivedClockStart = false;
String jsArray = "";

// +++++++++++++++++++ WEB-Server objects +++++++++++++++
WebServer server(80);
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++

String foundNetworkSSID[5];
int foundNetworkRSSI[5];
int networkCount = 0;

// +++++++++++++++ START EEPROM Vakues +++++++++++++++++++++++++++

String receivedSSID = "";
String receivedPASSWORD = "";

String receivedLOCATION_PLZ = "";
String receivedLOCATION_NAME = "";

int receivedRED = 50;
int receivedGREEN = 50;
int receivedBLUE = 50;
bool receivedNIGHTJN = false;
int receivedNIGHTSTART = 0;
int receivedNIGHTEND = 0;

bool invalidCharacterExist = false;

String savedSSID = "";
String savedPASSWORD = "";

String savedLOCATION_PLZ = "";
String savedLOCATION_NAME = "";

int savedRGB_RED = 50;
int savedRGB_GREEN = 50;
int savedRGB_BLUE = 50;

bool savedNIGHT_JN = false;
int savedNIGHT_START = 0;
int savedNIGHT_END = 0;

// EEPROM_SIZE und die Adressen für Farbe/Nachtabschaltung stehen in LED_WETTER_PLATT.h
// EEPROM Adressen
#define ADDR_SSID 0                 // 1 Byte Länge + max. 32 Zeichen  (0 - 32)
#define ADDR_PASSWORD_V4 200        // alte Adresse bis V4 - nur noch zum einmaligen Übernehmen gelesen
// ADDR_LED_RED 230, ADDR_LED_GREEN 240, ADDR_LED_BLUE 250
// ADDR_NIGHTJN 260, ADDR_NIGHT_START 270, ADDR_NIGHT_END 290
#define ADDR_LOCATION_PLZ 320       // 1 Byte Länge + max. 8 Zeichen   (320 - 328)
#define ADDR_LOCATION_NAME 330      // 1 Byte Länge + max. 60 Zeichen  (330 - 390)
#define ADDR_PASSWORD 400           // (9) NEU in V5: 1 Byte Länge + max. 64 Zeichen (400 - 464)
#define ADDR_EEPROM_LAYOUT 470      // (9) Kennung des EEPROM-Aufbaus (5 = ab V5, Passwort liegt auf 400)

#define EEPROM_LAYOUT_V5 5

#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 64
#define MAX_PASSWORD_LEN_V4 29      // an Adresse 200 war nur Platz bis 229 (danach folgen die Farben)
#define MAX_LOCATION_PLZ_LEN 8
#define MAX_LOCATION_NAME_LEN 60

// +++++++++++++++++++ Ende EEPROM Values +++++++++++++++++++++++

unsigned long autoConnectStart = 0;
unsigned long clientConnectTimer = 0;

bool autoConnectStarted = false;
bool webClientActive = false;

// "volatile", weil der WiFi-Event-Handler in einem eigenen Task läuft und das Flag
// von dort aus verändert
volatile bool WIFIConnected = false;

// --- (12) Konfiguration WLAN-Abbruch-Historie (in Preferences) ---------------------------
#define WIFI_LOG_SIZE   10            // Anzahl gespeicherter Ereignisse
#define WIFI_LOG_NS     "wifi"        // Namespace in den Preferences

// --- Hilfsfunktion: Wifi Disconnects - Reason-Code -> Text -------------------------------
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

// (9) Länge begrenzen, damit ein zu langer Text nie in die nachfolgenden
// EEPROM-Bereiche (Farben, Nachtabschaltung, Ort ...) hineinschreibt
void writeStringToEEPROM(int addrOffset, const String &strToWrite, unsigned int maxLen) {
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
  // Board = 0xFF = 255): Länge begrenzen, BEVOR das Stack-Array angelegt wird.
  if (newStrLen > maxLen) {
    newStrLen = maxLen;
    invalidCharacterExist = true;
  }

  Serial.print("read EEPROM Offset Address: "); Serial.print(addrOffset);
  Serial.print(" / Storage Length: "); Serial.println(newStrLen);

  char data[newStrLen + 1];

  for (unsigned int i = 0; i < newStrLen; i++) {

    data[i] = EEPROM.read(addrOffset + 1 + i);

    // als "unsigned" auswerten - sonst werden Umlaute (UTF-8-Bytes > 127)
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

// (9) Liest SSID + Passwort aus dem EEPROM und setzt "invalidCharacterExist".
// Bis V4 setzte jedes Lesen das Flag neu - entscheidend war damit nur der zuletzt
// gelesene Ortsname, eine kaputte SSID wurde nicht erkannt.
void loadCredentials() {
  invalidCharacterExist = false;
  savedSSID = readStringFromEEPROM(ADDR_SSID, MAX_SSID_LEN);
  bool ssidInvalid = invalidCharacterExist;

  // Einmalige Übernahme des Passworts von der alten V4-Adresse (200) nach 400
  if (EEPROM.read(ADDR_EEPROM_LAYOUT) != EEPROM_LAYOUT_V5) {
    invalidCharacterExist = false;
    String oldPassword = readStringFromEEPROM(ADDR_PASSWORD_V4, MAX_PASSWORD_LEN_V4);
    if (invalidCharacterExist) oldPassword = "";

    writeStringToEEPROM(ADDR_PASSWORD, oldPassword, MAX_PASSWORD_LEN);
    EEPROM.write(ADDR_EEPROM_LAYOUT, EEPROM_LAYOUT_V5);
    EEPROM.commit();
    Serial.println("EEPROM auf V5 umgestellt - Passwort von Adresse 200 nach 400 übernommen: " + String(oldPassword.length() > 0 ? "ja" : "nein (leer/ungültig)"));
  }

  invalidCharacterExist = false;
  savedPASSWORD = readStringFromEEPROM(ADDR_PASSWORD, MAX_PASSWORD_LEN);
  bool passwordInvalid = invalidCharacterExist;

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

  // ungültiger Ort (z.B. leeres EEPROM) wird als "kein Ort" behandelt - beeinflusst
  // aber NICHT mehr die Prüfung von SSID/Passwort
  bool keepInvalid = invalidCharacterExist;
  invalidCharacterExist = false;
  savedLOCATION_PLZ = readStringFromEEPROM(ADDR_LOCATION_PLZ, MAX_LOCATION_PLZ_LEN);
  if (invalidCharacterExist) savedLOCATION_PLZ = "";
  invalidCharacterExist = false;
  savedLOCATION_NAME = readStringFromEEPROM(ADDR_LOCATION_NAME, MAX_LOCATION_NAME_LEN);
  if (invalidCharacterExist) savedLOCATION_NAME = "";
  invalidCharacterExist = keepInvalid;
}

// Text sicher in einen JavaScript-String ("...") einbetten. Ein Anführungszeichen
// oder Backslash im Passwort/Ortsnamen würde sonst die komplette Web-Seite zerstören.
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

// (10) Passwort für die Ausgabe im seriellen Monitor maskieren: statt Klartext nur
// Sternchen + Länge, z.B. "******** (20 Zeichen)"
String maskPassword(const String &pw) {
  if (pw.length() == 0) return "(leer)";
  return "******** (" + String(pw.length()) + " Zeichen)";
}

// (11) Zahl aus dem JSON lesen - egal ob als Zahl oder als Text ("123") gesendet
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

    // Sortierung nach Signalstärke (stärkstes Netz zuerst)
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

    // max. 5 Netzwerke übernehmen - ohne Duplikate und ohne versteckte Netzwerke (leere SSID)
    for(int i=0;i<netWorks && networkCount<5;i++){
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

// ++++++ (12) Ausgabe der WLAN-Abbruch-Historie als HTML-Tabelle +++++++++
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

// Webserver
void handleRoot() {

  webClientActive = true;  // WEB-Client hat Seite geöffnet → AutoConnect stoppen

  // Das gespeicherte WLAN steht an erster Stelle (vorausgewählt) und wird
  // in der Liste der gefundenen Netzwerke nicht noch einmal aufgeführt
  jsArray = "[";
  bool firstEntry = true;

  if (invalidCharacterExist) {
    jsArray += "\"... dien Netzwark ... \"";
    firstEntry = false;
  } else if (savedSSID.length() > 0) {
    jsArray += "\"" + jsEscape(savedSSID) + "\"";
    firstEntry = false;
  }

  for(int i=0;i<networkCount;i++){
    if (!invalidCharacterExist && foundNetworkSSID[i] == savedSSID) continue;   // nicht doppelt aufführen
    if (!firstEntry) jsArray += ",";
    jsArray += "\"" + jsEscape(foundNetworkSSID[i]) + "\"";
    firstEntry = false;
  }

  jsArray += "]";

  String html = String(htmlPage);
  html.replace("%%NETWORKS%%", jsArray);

  if (!invalidCharacterExist) {
    html.replace("%%PASSWORD%%", "\"" + jsEscape(savedPASSWORD) + "\"");
    html.replace("%%LOCATION_PLZ%%", "\"" + jsEscape(savedLOCATION_PLZ) + "\"");
    html.replace("%%LOCATION_NAME%%", "\"" + jsEscape(savedLOCATION_NAME) + "\"");
    html.replace("%%RED%%", String(savedRGB_RED));
    html.replace("%%GREEN%%", String(savedRGB_GREEN));
    html.replace("%%BLUE%%", String(savedRGB_BLUE));
    html.replace("%%NIGHTJN%%", savedNIGHT_JN ? "true" : "false");
    html.replace("%%NIGHTSTART%%", String(savedNIGHT_START));
    html.replace("%%NIGHTEND%%", String(savedNIGHT_END));
    html.replace("%%WIFILOG%%", getDisconnectLog());

  } else {

    html.replace("%%PASSWORD%%", "\"\"");                 // leer -> Platzhalter "... dien Kennwort ..." im Eingabefeld
    html.replace("%%LOCATION_PLZ%%", "\"\"");
    html.replace("%%LOCATION_NAME%%", "\"\"");
    html.replace("%%RED%%", "100");
    html.replace("%%GREEN%%", "100");
    html.replace("%%BLUE%%", "100");
    html.replace("%%NIGHTJN%%", "true");
    html.replace("%%NIGHTSTART%%", "0");
    html.replace("%%NIGHTEND%%", "6");
    html.replace("%%WIFILOG%%", getDisconnectLog());
  }

  server.send(200,"text/html", html);

}


void handleSave(){

  if(!server.hasArg("plain")){
    server.send(400, "text/plain", "No body");
    return;
  }

  String body = server.arg("plain");
  JsonDocument jsonReceive;                                               // create a JSON container (ArduinoJson 7: Größe wird automatisch verwaltet)
  DeserializationError error = deserializeJson(jsonReceive, body);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    server.send(400, "text/plain", "Invalid JSON");                      // WEB-UI bekommt eine Antwort statt auf den Timeout zu warten
    return;
  }

  // JSON string was received correctly, so information can be retrieved:
  // (11) Werte werden auf gültige Bereiche begrenzt
  receivedRED   = constrain(jsonInt(jsonReceive["rgb_red"], 50), 0, 255);
  receivedGREEN = constrain(jsonInt(jsonReceive["rgb_green"], 50), 0, 255);
  receivedBLUE  = constrain(jsonInt(jsonReceive["rgb_blue"], 50), 0, 255);

  receivedSSID     = jsonReceive["ssid"] | "";
  receivedPASSWORD = jsonReceive["password"] | "";

  receivedLOCATION_PLZ  = jsonReceive["locationPLZ"] | "";
  receivedLOCATION_NAME = jsonReceive["locationName"] | "";
  receivedLOCATION_PLZ.trim();
  receivedLOCATION_NAME.trim();

  receivedNIGHTJN    = jsonReceive["nightJN"] | false;
  receivedNIGHTSTART = constrain(jsonInt(jsonReceive["nightStart"], 0), 0, 23);
  receivedNIGHTEND   = constrain(jsonInt(jsonReceive["nightEnd"], 6), 0, 23);
  receivedClockStart = jsonReceive["clockStart"] | false;

  EEPROM.write(ADDR_LED_RED, receivedRED);
  EEPROM.write(ADDR_LED_GREEN, receivedGREEN);
  EEPROM.write(ADDR_LED_BLUE, receivedBLUE);

  // (9) alle Texte längenbegrenzt speichern
  writeStringToEEPROM(ADDR_SSID, receivedSSID, MAX_SSID_LEN);
  writeStringToEEPROM(ADDR_PASSWORD, receivedPASSWORD, MAX_PASSWORD_LEN);

  writeStringToEEPROM(ADDR_LOCATION_PLZ, receivedLOCATION_PLZ, MAX_LOCATION_PLZ_LEN);
  writeStringToEEPROM(ADDR_LOCATION_NAME, receivedLOCATION_NAME, MAX_LOCATION_NAME_LEN);

  EEPROM.write(ADDR_NIGHTJN, receivedNIGHTJN);
  EEPROM.write(ADDR_NIGHT_START, receivedNIGHTSTART);
  EEPROM.write(ADDR_NIGHT_END, receivedNIGHTEND);

  EEPROM.write(ADDR_EEPROM_LAYOUT, EEPROM_LAYOUT_V5);

  EEPROM.commit();

  loadCredentials();
  loadSettings();

  Serial.println("Gelesene Werte SSID: " + savedSSID + " PW: " + maskPassword(savedPASSWORD) + " im HandleSave.");

  // ++++++++++++++++++++++++++++

  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert RED_RGB_VALUE: " + String(savedRGB_RED));
  Serial.println("Gespeichert GREEN_RGB_VALUE: " + String(savedRGB_GREEN));
  Serial.println("Gespeichert BLUE_RGB_VALUE: " + String(savedRGB_BLUE));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert NightMode(JN): " + String(savedNIGHT_JN));
  Serial.println("Gespeichert NightStart: " + String(savedNIGHT_START));
  Serial.println("Gespeichert NightEnd: " + String(savedNIGHT_END));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert SSID: " + receivedSSID);
  Serial.println("Gespeichert PASSWORT: " + maskPassword(receivedPASSWORD));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert LOCATION: " + savedLOCATION_PLZ + " - " + savedLOCATION_NAME);
  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Starte Uhr received (JN): " + String(receivedClockStart));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");

  server.send(200,"text/plain","OK");

}

// --- (12) Ringpuffer-Eintrag in Preferences schreiben --------------------
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

// --- (12) Gespeicherte Historie auf Serial ausgeben -----------------------
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

// --- (12) Optional: Log komplett löschen (wird aktuell nicht aufgerufen) ---
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
    // (7) false: die Zugangsdaten liegen ohnehin im EEPROM. Mit "true" schriebe jedes
    // WiFi.begin() (auch die des Watchdogs alle 30 s) in den Flash.
    WiFi.persistent(false);

    WiFi.begin(savedSSID.c_str(), savedPASSWORD.c_str());

    int attempts=0;
    while(WiFi.status()!=WL_CONNECTED && attempts < 20){  // 10s sind oft zu kurz
      server.handleClient();   // (8) Web-UI während des Verbindungsaufbaus erreichbar halten
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

// --- WIFI - Event-Handler ----------------------------------
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("WiFi got IP: " + WiFi.localIP().toString());
      WIFIConnected = true;
      break;

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println(F("[WiFi] IP verloren"));
      WIFIConnected = false;
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      uint8_t r = info.wifi_sta_disconnected.reason;
      Serial.printf("[WiFi] Disconnect, reason %u (%s)\n", r, wifiReasonToText(r));
      WIFIConnected = false;
      logDisconnect(r);       // (12) in der Historie festhalten
      break;
    }

    default:
      break;
  }
}
// ++++++++++++++++ WIFI Event Handler ENDE +++++++++++++


// ++++++++++++++++++ START SETUP +++++++++++++++++++++++++
void setup() {

  Serial.begin(115200);

  // +++++++++++++ Scanning Local WLAN SSID +++++++++++++++
  scanNetworks();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP,passwordAP);
  Serial.println("AP gestartet: "+String(ssidAP));

  EEPROM.begin(EEPROM_SIZE);

  loadCredentials();
  loadSettings();

  Serial.println("Gelesene Werte SSID: " + savedSSID + " PW: " + maskPassword(savedPASSWORD) + " im setup.");
  Serial.println("Gelesene LOCATION: " + savedLOCATION_PLZ + " - " + savedLOCATION_NAME);

  server.on("/",handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  // +++++ init LED +++++++++++
  initLED(savedRGB_RED, savedRGB_GREEN, savedRGB_BLUE);

  // +++++++ EVENT Handler zum Check auf bestehende WLAN connection ++++++++++
  // Event-Handler registrieren, wirkt automatisch sobald STA aktiv ist
  WiFi.onEvent(onWiFiEvent);

  // (12) Beim Booten einmal anzeigen, was zuletzt mit dem WLAN los war
  printDisconnectLog();

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

  now = millis();         // read out the current "time" ("millis()" gives the time in ms since the Arduino started)

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

    // (2) prüft auf neuen Ort - und wiederholt eine fehlgeschlagene Ortssuche alle 10 Minuten
    checkNewLocation(savedLOCATION_PLZ, savedLOCATION_NAME);

    // (2) neuer Ort gefunden -> Wetter sofort neu laden (nicht erst nach bis zu 10 Minuten)
    if (locationChanged) {
      locationChanged = false;
      firstGetWeatherLoop = true;
    }

    // Get Time from NTP Server with a refresh of approx. 15 sec (refer to the GET_TIME_INTERVAL Variable)
    if ((now - previousTIMEcall) > GET_TIME_interval) {

      readLocalTime();
      // Connect to NTP Server and get date & time successful
      Serial.print("NTP Server --> timeInfoSuccess = "); Serial.println(timeInfoSuccess);

      previousTIMEcall = now;

    }

    if (timeInfoSuccess){

      // ++++ GET WEATHER-CODE from OPEN-METEO with a refresh of approx. 10 Minutes (refer to the GET_WEATHER_INTERVAL Variable)+++++
      // (ohne gültigen Ort ist ein Wetterabruf sinnlos - er würde für Breitengrad/Längengrad 0/0 erfolgen)
      if (!noLocationFound && ((now - previousWeatherCall) > GET_WEATHER_interval || firstGetWeatherLoop)) {

        Serial.println("Start GET-WEATHERCODE by millisecond : " + String(now));

        bool weatherOK = getWeatherCodePlatt();

        previousWeatherCall = now;
        firstGetWeatherLoop = false;

        // (3) Abruf fehlgeschlagen -> nicht 10 Minuten warten, sondern nach 1 Minute erneut versuchen
        if (!weatherOK) {
          previousWeatherCall = now - GET_WEATHER_interval + WEATHER_RETRY_interval;
          Serial.println("Wetterabruf fehlgeschlagen - neuer Versuch in 1 Minute (bisherige Werte bleiben erhalten).");
        }

      }

      // ++++ DISPLAY WEATHER-CODE with a refresh of approx. 10 sec. (refer to the DISPLAY_WEATHER_INTERVAL Variable)+++++
      if ((now - previousWeatherDisplay) > DISPLAY_WEATHER_interval ) {

        if (noLocationFound){

          noValidGeoCodes();

        } else if (!weatherDataValid) {

          // (3) noch nie erfolgreich Wetterdaten geladen -> grüne Warte-Anzeige statt "Sonne"
          waitForTimeSuccess();

        } else {

          displayWeatherCodePlatt(savedNIGHT_JN, savedNIGHT_START, savedNIGHT_END, WetterInStund);

        }

        previousWeatherDisplay = millis();      // millis(): die Anzeige selbst dauert einige Sekunden
        WetterInStund++; // Wechsel der Wetteranzeige (EEN, TWEE oder DREE STÜND)

        if (WetterInStund > 3){
            WetterInStund = 1;
        }; // zurücksetzen, wenn der Wert > DREE ist
      }

    } else {

      waitForTimeSuccess();

    }

  }

  if (!WIFIConnected && webClientActive) {

    // Nicht mehr anzeigen, wenn die Uhr manuell per Klick auf "Start wedder" gestartet wurde
    if (!receivedClockStart) {

      // ++++++ Set color to the actual EEPROM stored values - refreshed by new values from WEB-Client +++++
      for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
        LEDstripArray[LEDindicator].setRGB(savedRGB_RED,savedRGB_GREEN,savedRGB_BLUE);
      }

      FastLED.show();

    }

    // ++++ Refresh - Color from WEB-Client +++++ END +++++++++++++++++++++++++++++++++

  }

}
