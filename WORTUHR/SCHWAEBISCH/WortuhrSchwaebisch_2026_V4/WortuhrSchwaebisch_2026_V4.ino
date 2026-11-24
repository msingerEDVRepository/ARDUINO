// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WORDCLOCK by Michael Singer / 2025-12-20
//
// V 1.0 
// WIFI-Scan (max. 5 SSID's) and start WEB-UI with a filled listbox of existing networks
// HTTP REQUEST communication (without WEBSocket)
// for ESP32 (only!) using the EEPROM for storing the basic ceredentials (no more ESP8266 MCU's)
// the main LED / Clock steering is re-located into a separate "Header-file" (LEDCLOCK_HOCHDEUTSCH.H)
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
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#include <Arduino.h>

#include <EEPROM.h>

#include <WiFi.h>                 // needed to connect to WiFi (for ESP32)
#include <WebServer.h>            // needed to create a simple webserver (make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error)

#include <ArduinoJson.h>          // used library "ArduinoJSON" by Benoit Blanchon / Version 7.4.2 - needed for JSON encapsulation (send multiple variables with one string)

#include <Preferences.h>          // New Store logic of an ESP32 MCU (better than EEPROM)

Preferences wifiPrefs;            // New Instance for being able to store the WIFI-Connection Errors

// INCLUDE - language specific Header files (for the LED CLOCK Steering and the WEB-UI)
// +++++++++ include Start ++++++++++++++++++++++++++++++++
#include "HTML_SCHWAEBISCH.h"           // include  WEB-UI
#include "LEDCLOCK_SCHWAEBISCH.h"       // include LED CLOCK run
// +++++++++ include Ende ++++++++++++++++++++++++++++++++

// ACCESS POINT -  Konfiguration
const char* ssidAP = "MEIWORTUHR";
const char* passwordAP = "meiwortuhr";         // Only simple  password required (7 digits is minimum)

int autoConnectStartTime = 120000;            // Start intervall = 2 Minuten

int GET_TIME_interval = 10000;                 //  every 10. sec // Network time protocol     --> 6 times a minute
int BLINK_interval = 1000;                 // every 5 sec. the LOGO should Blink

unsigned long previousTIMEcall  = 0;          // we use the "millis()" command for time reference and this will output an unsigned long
unsigned long previousSECOND  = 0;            // we use the "millis()" command for time reference and this will output an unsigned long

bool receivedClockStart = false;
bool countdownDone = false;

String jsArray = "";

bool LEDBlinkingToggle = true;

// +++++++++++++++++++ WEB-Server objects +++++++++++++++
WebServer server(80);
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++

String foundNetworkSSID[5];
int foundNetworkRSSI[5];
int networkCount = 0;

// +++++++++++++++ START EEPROM Vakues +++++++++++++++++++++++++++

const char*  receivedSSID = "";
const char*  receivedPASSWORD = "";
int receivedRED = 50;
int receivedGREEN = 50;
int receivedBLUE = 50;
bool receivedNIGHT_JN = false;
int receivedNIGHTSTART = 0;
int receivedNIGHTEND = 0;
bool receivedSECONDS_JN = false;

bool invalidCharacterExist = false;

String savedSSID = "";
String savedPASSWORD = "";
int savedRGB_RED = 50;
int savedRGB_GREEN = 50;
int savedRGB_BLUE = 50;
bool savedNIGHT_JN = false;
int savedNIGHT_START = 0;
int savedNIGHT_END = 0;
bool savedSECONDS_JN = false;

#define EEPROM_SIZE 512
// EEPROM Adressen
#define ADDR_SSID 0
#define ADDR_PASSWORD 200
#define ADDR_LED_RED 230
#define ADDR_LED_GREEN 240
#define ADDR_LED_BLUE 250
#define ADDR_NIGHTJN 260
#define ADDR_NIGHT_START 270
#define ADDR_NIGHT_END 290
#define ADDR_SECONDSJN 310

// +++++++++++++++++++ Ende EEPROM Values +++++++++++++++++++++++

unsigned long autoConnectStart = 0;
unsigned long clientConnectTimer = 0;

bool autoConnectStarted = false;
bool webClientActive = false;

// --- Konfiguration WIFI incl. Error-Codes ( in Preferences) ----------------------------------

#define WIFI_LOG_SIZE   10            // Anzahl gespeicherter Ereignisse
#define WIFI_LOG_NS     "wifi"        // Namespace in den Preferences

extern bool WIFIConnected = false;            // dein bestehendes Flag

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

void writeStringToEEPROM(int addrOffset, const String &strToWrite) {
  byte len = strToWrite.length();

  EEPROM.write(addrOffset, len);
  //Serial.print("write EEPROM-SSID-Length: ");Serial.println(len);
  //Serial.print("write Offset-Addresse: "); Serial.println(addrOffset);

  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
    //Serial.print("write EEPROM-SSID-Charakter: ");Serial.println(strToWrite[i]);
    //Serial.print("write Offset-Addresse: "); Serial.println(addrOffset + 1 + i);
  }
}

String readStringFromEEPROM(int addrOffset) {
  int newStrLen = EEPROM.read(addrOffset);

   Serial.print("read EEPROM Offset Address: "); Serial.println(addrOffset);
   Serial.print("read EEPROM Storage Length: ");Serial.println(newStrLen);

  char data[newStrLen + 1];

  int  asciiValue;

  invalidCharacterExist = false;

  for (int i = 0; i < newStrLen; i++) {

    data[i] = EEPROM.read(addrOffset + 1 + i);
    
    asciiValue = (int)data[i];
    Serial.print("EEPROM-Charakter Nr: " + String(i) + " ");
    Serial.println("DataValue: " + String(data[i]) + " AsciiValue: " + String(asciiValue));
  
    // Prüfung auf ungültige Zeichen
    // https://www.asciitable.com
    if (asciiValue < 32 || asciiValue >= 255) {
      invalidCharacterExist = true;
      Serial.println("EEPROM-Charakter INVALID!");
      break;
    }

  }

  data[newStrLen] = '\0'; // NOTE: Remove the space between the backslash "\" and "0" (I've added a space because otherwise there is a display bug)

  return String(data);
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
      // Ausgabe der gefundenen Netzwerke in der Reihenfolge der Signalstaerke
      // Serial.println("SSID-Nr. mit dem Namen : " + String(tempSSID[i]) + " in der Signalsträte (RSSI): " + String(tempRSSI[i]) );
    }

    // Sort FoundNetworks decreasing (best Network Signal strength is on top) and eliminate duplicates
    for(int i=0;i<netWorks && networkCount<5;i++){
      if(!isDuplicate(tempSSID[i], foundNetworkSSID, networkCount)){

        foundNetworkSSID[networkCount] = tempSSID[i];
        foundNetworkRSSI[networkCount] = tempRSSI[i];
        
        // Ausgabe der gefundenen Netzwerke in der Reihenfolge der Signalstaerke
        Serial.println("foundNetworkSSID : " + String(foundNetworkSSID[networkCount]) + " foundNetworkRSSI: " + String(foundNetworkRSSI[networkCount]) );
        networkCount++;
      }
    }
  }
  WiFi.mode(WIFI_OFF);
}

// ++++++  Ausgabe der WIFI Disconnect Information in eine HTML +++++++++
String getDisconnectLog() {
  wifiPrefs.begin(WIFI_LOG_NS, true);
  uint8_t idx = wifiPrefs.getUChar("idx", 0);

  String out =
    "<table border='1' cellspacing='0' cellpadding='4'>"
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
    jsArray = "[\"" + savedSSID + "\",";
  } else {
    jsArray = "[\"... bitte Netzwerk wählen... \",";
  }

  for(int i=0;i<networkCount;i++){
    jsArray += "\"";
    for(unsigned int j=0;j<foundNetworkSSID[i].length();j++){
      char c = foundNetworkSSID[i][j];
      if(c=='\\' || c=='\"') jsArray += '\\';
      jsArray += c;
    }
    jsArray += "\"";
    if(i<networkCount-1) jsArray+=",";
  }

  jsArray += "]";

  String html = String(htmlPage);
  html.replace("%%NETWORKS%%", jsArray);

  if (!invalidCharacterExist) {
    html.replace("%%PASSWORD%%", "\"" + savedPASSWORD + "\"");
    html.replace("%%RED%%", String(savedRGB_RED));
    html.replace("%%GREEN%%", String(savedRGB_GREEN));
    html.replace("%%BLUE%%", String(savedRGB_BLUE));
    html.replace("%%NIGHTJN%%", String(savedNIGHT_JN));
    html.replace("%%NIGHTSTART%%", String(savedNIGHT_START));
    html.replace("%%NIGHTEND%%", String(savedNIGHT_END));
    html.replace("%%SECONDSJN%%", String(savedSECONDS_JN));
    html.replace("%%WIFILOG%%", getDisconnectLog());
  } else {
    html.replace("%%PASSWORD%%", "\"... bitte Passwort eingeben ...\"");
    html.replace("%%RED%%", "100");
    html.replace("%%GREEN%%", "100");
    html.replace("%%BLUE%%", "100");
    html.replace("%%NIGHTJN%%", "true");
    html.replace("%%NIGHTSTART%%", "0");
    html.replace("%%NIGHTEND%%", "6");
    html.replace("%%SECONDSJN%%", "false");
    html.replace("%%WIFILOG%%", "Disconnect-Log nicht vorhanden.");     
  }    
    
  server.send(200,"text/html", html);

}


void handleSave(){

  if(!server.hasArg("plain")){
    server.send(400, "text/plain", "No body");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<200> jsonReceive;                                    // create a JSON container
  DeserializationError error = deserializeJson(jsonReceive, body);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  else {

    // JSON string was received correctly, so information can be retrieved:
    receivedRED = jsonReceive["rgb_red"];
    receivedGREEN = jsonReceive["rgb_green"];
    receivedBLUE = jsonReceive["rgb_blue"];

    receivedSSID = jsonReceive["ssid"];
    receivedPASSWORD = jsonReceive["password"];
    
    receivedNIGHT_JN = jsonReceive["nightJN"];
    receivedNIGHTSTART = jsonReceive["nightStart"];
    receivedNIGHTEND = jsonReceive["nightEnd"];
    receivedSECONDS_JN = jsonReceive["secondsJN"];

    receivedClockStart = jsonReceive["clockStart"];

    EEPROM.write(ADDR_LED_RED, receivedRED);
    EEPROM.write(ADDR_LED_GREEN, receivedGREEN);
    EEPROM.write(ADDR_LED_BLUE, receivedBLUE);

    writeStringToEEPROM (ADDR_SSID, receivedSSID);
    writeStringToEEPROM (ADDR_PASSWORD, receivedPASSWORD);

    EEPROM.write(ADDR_NIGHTJN, receivedNIGHT_JN);
    EEPROM.write(ADDR_NIGHT_START, receivedNIGHTSTART);
    EEPROM.write(ADDR_NIGHT_END, receivedNIGHTEND);
    EEPROM.write(ADDR_SECONDSJN, receivedSECONDS_JN);

    EEPROM.commit();
  }

  savedRGB_RED = EEPROM.read(ADDR_LED_RED);
  savedRGB_GREEN = EEPROM.read(ADDR_LED_GREEN);
  savedRGB_BLUE = EEPROM.read(ADDR_LED_BLUE);

  savedSSID = readStringFromEEPROM(ADDR_SSID);
  savedPASSWORD = readStringFromEEPROM(ADDR_PASSWORD);

  savedNIGHT_JN = EEPROM.read(ADDR_NIGHTJN);
  savedNIGHT_START = EEPROM.read(ADDR_NIGHT_START);
  savedNIGHT_END = EEPROM.read(ADDR_NIGHT_END);

  savedSECONDS_JN = EEPROM.read(ADDR_SECONDSJN);

  Serial.println("Gelesene Werte SSID: " + savedSSID + " PW: " + savedPASSWORD + " im HandleSave.");

  // ++++++++++++++++++++++++++++

  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert RED_RGB_VALUE:" + String(savedRGB_RED));
  Serial.println("Gespeichert RED_RGB_VALUE:" + String(savedRGB_GREEN));
  Serial.println("Gespeichert RED_RGB_VALUE:" + String(savedRGB_BLUE));
  Serial.println("Show Blinking SECONDS (JN):" + String(savedSECONDS_JN));
  Serial.println("NightMode(JN):" + String(savedNIGHT_JN) + " NightStart:" + String(savedNIGHT_START) + " NightEnd:" + String(savedNIGHT_END));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert SSID:" + String(receivedSSID) + " PW:" + String(receivedPASSWORD));
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
    WiFi.persistent(true);

    WiFi.begin(savedSSID.c_str(), savedPASSWORD.c_str());

    int attempts=0;
    while(WiFi.status()!=WL_CONNECTED && attempts < 20){  // 10s sind oft zu kurz
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if(WiFi.status()==WL_CONNECTED){
      Serial.println("\nVerbunden! IP: "+WiFi.localIP().toString());
      WiFi.softAPdisconnect(true);
      // WIFIConnected wird jetzt vom Event-Handler gesetzt — hier nicht mehr nötig
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

  savedRGB_RED = EEPROM.read(ADDR_LED_RED);
  savedRGB_GREEN = EEPROM.read(ADDR_LED_GREEN);
  savedRGB_BLUE = EEPROM.read(ADDR_LED_BLUE);
  savedSSID = readStringFromEEPROM(ADDR_SSID);
  savedPASSWORD = readStringFromEEPROM(ADDR_PASSWORD);
  savedNIGHT_JN = EEPROM.read(ADDR_NIGHTJN);
  savedNIGHT_START = EEPROM.read(ADDR_NIGHT_START);
  savedNIGHT_END = EEPROM.read(ADDR_NIGHT_END);
  savedSECONDS_JN = EEPROM.read(ADDR_SECONDSJN);

  server.on("/",handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  Serial.println("Gelesene Werte SSID: " + savedSSID + " PW: " + savedPASSWORD + " im setup.");

  // +++++++++++ Init LED-Strip ++++++++++++++++++++++++++
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(LEDstripArray, NUMPIXELS);
  FastLED.setBrightness(BRIGHTNESS);

    // ++++++++++ INIT of WORD-Clock Array ++++++++
 
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    newletter[LEDindicator] = 0;   
  }

  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    shownletter[LEDindicator] = 0;   
  }

  // +++++++++++ End of INIT Word-Clock LED Array +++

  // alle LED einschalten, um die LED Color & Brightness zu sehen (die aus dem EEPROM übernommen und dann angepasst wird...)     
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
    LEDstripArray[LEDindicator].setRGB( savedRGB_RED, savedRGB_GREEN, savedRGB_BLUE);
    FastLED.show();
    oldRED = savedRGB_RED;
    oldGREEN = savedRGB_GREEN;
    oldBLUE = savedRGB_BLUE;
  }

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
    Serial.println("AutoConnect startet in 1 Minute wenn die WEB-Site nicht vorher aufgerufen wird ...");
  }
  // ++++++ Collect time information from NTP +++++++++
  sntp_servermode_dhcp(1); 
  configTzTime(time_zone, ntpServer1, ntpServer2);

}


// ++++++++++++++++++ START LOOP +++++++++++++++++++++++++
void loop() {

  unsigned long now = millis();         // read out the current "time" ("millis()" gives the time in ms since the Arduino started)

  // ++++++++++++++++++++++ Watchdog Start +++++++++++++++++
  // Watchdog für den Fall, dass der WiFi.reconnect() nicht greift

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

  // Non-blocking Auto-Connect nach 1 Minute
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

      previousTIMEcall = now;

    }

    // Die LED (60) == Holbastelei-LOGO soll im Sekundentakt blinken.
    // Get Time from NTP Server with a refresh of approx. 10 sec (refer to the GET_TIME_INTERVAL Variable)
    // ... show only if "Show Blinking Seconds is activated" 
    if (savedSECONDS_JN) {

      if ((now - previousSECOND) > BLINK_interval) {     

        if (LEDBlinkingToggle) {

          // der NSCALE - Value = 64 entspricht einer ca. 25%igen Brightness.
          blinkingLogo(64);
          LEDBlinkingToggle = false;       
        
        } else {

          blinkingLogo(0);
          LEDBlinkingToggle = true;
        
        }

        previousSECOND = now;

      }

    }    

  }

  if (!WIFIConnected && webClientActive) {
    
    // Nicht mehr anzeigen, wenn die Uhr manuell per Klich auf "Start wedder" gestartet wurde
    if (!receivedClockStart) {

      // ++++++ Set color to the actual EEPROM stored values - refreshed by new values from WEB-Client +++++
      for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
        // LEDstripArray[LEDindicator].setRGB( receivedRED, receivedGREEN, receivedBLUE);
        LEDstripArray[LEDindicator].setRGB(savedRGB_RED,savedRGB_GREEN,savedRGB_BLUE);    
      }

      FastLED.show();

    }

  }

}
