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

#include <Arduino.h>

#include <EEPROM.h>

#include <WiFi.h>                 // needed to connect to WiFi (for ESP32)
#include <WebServer.h>            // needed to create a simple webserver (make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error)

#include <ArduinoJson.h>          // used library "ArduinoJSON" by Benoit Blanchon / Version 7.4.2 - needed for JSON encapsulation (send multiple variables with one string)

// INCLUDE - language specific Header files (for the LED CLOCK Steering and the WEB-UI)
// +++++++++ include Start ++++++++++++++++++++++++++++++++
#include "HTML_WETTER_PLATT.h"           // include  WEB-UI
#include "LED_WETTER_PLATT.h"            // include LED CLOCK run
// +++++++++ include Ende ++++++++++++++++++++++++++++++++

// ACCESS POINT -  Konfiguration
const char* ssidAP = "MIENWEDDER";
const char* passwordAP = "mienwedder";         // Only simple  password required (7 digits is minimum)

int autoConnectStartTime = 120000; //120000; //zu Testzwecken nur halbe Zeit --> // 120000;            // Start intervall = 2 Minuten
unsigned long now = 0;

// We want to periodically send values to the clients, so we need to define an "interval" 
// and remember the last time we sent data to the client (with "previousMillis")
int GET_TIME_interval                   = 15000;                //  NETWORT TIME + OPEN METEO         --> every 15 seconds
int GET_WEATHER_interval                = 600000;               //  every 10 minutes (10 * 1000 * 60 = 600.000 ms) - free license is maximized to 10.000 calls per day // and sketch has 3 API calls per loop.                                                                // resulting in 6 requests per hour * 24h == 144 API calls per day
int DISPLAY_WEATHER_interval            = 10000;                //  Show weather forecast (een, twee, dree stünd'n) --> every 10 seconds

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

const char*  receivedSSID = "";
const char*  receivedPASSWORD = "";

const char*  receivedLOCATION_PLZ = "";
const char*  receivedLOCATION_NAME = "";


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

String savedLATITUDE = "";
String savedLONGITUDE = "";

int savedRGB_RED = 50;
int savedRGB_GREEN = 50;
int savedRGB_BLUE = 50;

bool savedNIGHT_JN = false;
int savedNIGHT_START = 0;
int savedNIGHT_END = 0;

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

#define ADDR_LOCATION_PLZ 320
#define ADDR_LOCATION_NAME 330

// +++++++++++++++++++ Ende EEPROM Values +++++++++++++++++++++++

unsigned long autoConnectStart = 0;
unsigned long clientConnectTimer = 0;

bool autoConnectStarted = false;
bool webClientActive = false;
bool WIFIConnected = false;

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

// Webserver
void handleRoot() {
  
  webClientActive = true;  // WEB-Client hat Seite geöffnet → AutoConnect stoppen

  if (!invalidCharacterExist) {
    jsArray = "[\"" + savedSSID + "\",";
  } else {
    jsArray = "[\"... dien Netzwark ... \",";
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
    html.replace("%%LOCATION_PLZ%%", "\"" + savedLOCATION_PLZ + "\"");    
    html.replace("%%LOCATION_NAME%%", "\"" + savedLOCATION_NAME + "\"");    
    html.replace("%%RED%%", String(savedRGB_RED));
    html.replace("%%GREEN%%", String(savedRGB_GREEN));
    html.replace("%%BLUE%%", String(savedRGB_BLUE));
    html.replace("%%NIGHTJN%%", String(savedNIGHT_JN));
    html.replace("%%NIGHTSTART%%", String(savedNIGHT_START));
    html.replace("%%NIGHTEND%%", String(savedNIGHT_END));

  } else {

    html.replace("%%PASSWORD%%", "\"... dien Kennwort ingaven ...\"");
    html.replace("%%LOCATION_PLZ%%", "\" PLZ \"");    
    html.replace("%%LOCATION_NAME%%", "\"... dien Urtschaft ...\"");    
    html.replace("%%RED%%", "100");
    html.replace("%%GREEN%%", "100");
    html.replace("%%BLUE%%", "100");
    html.replace("%%NIGHTJN%%", "true");
    html.replace("%%NIGHTSTART%%", "0");
    html.replace("%%NIGHTEND%%", "6");
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

    receivedLOCATION_PLZ = jsonReceive["locationPLZ"];   
    receivedLOCATION_NAME = jsonReceive["locationName"];     

    receivedNIGHTJN = jsonReceive["nightJN"];
    receivedNIGHTSTART = jsonReceive["nightStart"];
    receivedNIGHTEND = jsonReceive["nightEnd"];
    receivedClockStart = jsonReceive["clockStart"];

    EEPROM.write(ADDR_LED_RED, receivedRED);
    EEPROM.write(ADDR_LED_GREEN, receivedGREEN);
    EEPROM.write(ADDR_LED_BLUE, receivedBLUE);

    writeStringToEEPROM (ADDR_SSID, receivedSSID);
    writeStringToEEPROM (ADDR_PASSWORD, receivedPASSWORD);

    writeStringToEEPROM (ADDR_LOCATION_PLZ, receivedLOCATION_PLZ);
    writeStringToEEPROM (ADDR_LOCATION_NAME, receivedLOCATION_NAME);

    EEPROM.write(ADDR_NIGHTJN, receivedNIGHTJN);
    EEPROM.write(ADDR_NIGHT_START, receivedNIGHTSTART);
    EEPROM.write(ADDR_NIGHT_END, receivedNIGHTEND);

    EEPROM.commit();
  }

  savedRGB_RED = EEPROM.read(ADDR_LED_RED);
  savedRGB_GREEN = EEPROM.read(ADDR_LED_GREEN);
  savedRGB_BLUE = EEPROM.read(ADDR_LED_BLUE);

  savedSSID = readStringFromEEPROM(ADDR_SSID);
  savedPASSWORD = readStringFromEEPROM(ADDR_PASSWORD);
  
  savedLOCATION_PLZ = readStringFromEEPROM(ADDR_LOCATION_PLZ);  
  savedLOCATION_NAME = readStringFromEEPROM(ADDR_LOCATION_NAME);

  savedNIGHT_JN = EEPROM.read(ADDR_NIGHTJN);
  savedNIGHT_START = EEPROM.read(ADDR_NIGHT_START);
  savedNIGHT_END = EEPROM.read(ADDR_NIGHT_END);

  Serial.println("Gelesene Werte SSID: " + savedSSID + " PW: " + savedPASSWORD + " im HandleSave.");

  // ++++++++++++++++++++++++++++

  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert RED_RGB_VALUE: " + String(savedRGB_RED));
  Serial.println("Gespeichert RED_RGB_VALUE: " + String(savedRGB_GREEN));
  Serial.println("Gespeichert RED_RGB_VALUE: " + String(savedRGB_BLUE));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");  
  Serial.println("Gespeichert NightMode(JN): " + String(savedNIGHT_JN));
  Serial.println("Gespeichert NightStart: " + String(savedNIGHT_START));
  Serial.println("Gespeichert NightEnd: " + String(savedNIGHT_END));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert SSID: " + String(receivedSSID));
  Serial.println("Gespeichert PASSWORT: " + String(receivedPASSWORD));  
  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert LOCATION: " + String(savedLOCATION_PLZ) + " - " + String(savedLOCATION_NAME));  
  Serial.println("++++++++++++++++++++++++++++++++++++++++");  
  Serial.println("Starte Uhr received (JN): " + String(receivedClockStart));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");

  server.send(200,"text/plain","OK");

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

  savedLOCATION_PLZ = readStringFromEEPROM(ADDR_LOCATION_PLZ);  
  savedLOCATION_NAME = readStringFromEEPROM(ADDR_LOCATION_NAME);

  savedNIGHT_JN = EEPROM.read(ADDR_NIGHTJN);
  savedNIGHT_START = EEPROM.read(ADDR_NIGHT_START);
  savedNIGHT_END = EEPROM.read(ADDR_NIGHT_END);

  Serial.println("Gelesene Werte SSID: " + savedSSID + " PW: " + savedPASSWORD + " im setup.");

  server.on("/",handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  // +++++ init LED +++++++++++
  initLED(savedRGB_RED, savedRGB_GREEN, savedRGB_BLUE);

    // +++++++ New EVENT Handler zum Check auf bestehenden WLAN connection ++++++++++
  // Event-Handler registrieren, wirkt automatisch sobald STA aktiv ist

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    switch(event){
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.println("WiFi got IP: " + WiFi.localIP().toString());
        WIFIConnected = true;
        break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.printf("WiFi disconnect, reason: %d\n",
                      info.wifi_sta_disconnected.reason);
        WIFIConnected = false;
        break;
      default: break;
    }
  });

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

  now = millis();         // read out the current "time" ("millis()" gives the time in ms since the Arduino started)

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

    //Serial.println("invalidCharacterExist vor AUTOCONNECT = " + String(invalidCharacterExist));
     
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

    checkNewLocation(savedLOCATION_PLZ, savedLOCATION_NAME);

    // Get Time from NTP Server with a refresh of approx. 15 sec (refer to the GET_TIME_INTERVAL Variable)
    if ((now - previousTIMEcall) > GET_TIME_interval) {     

      readLocalTime();
      // Connect to NTP Server and get date & time successful
      Serial.print("NTP Server --> timeInfoSuccess = "); Serial.println(timeInfoSuccess);

      previousTIMEcall = now;
    
    }

    if (timeInfoSuccess){

      // ++++ GET WEATHER-CODE from OPEN-METEO with a refresh of approx. 60 Minutes (refer to the GET_WEATHER_INTERVAL Variable)+++++
      if ((now - previousWeatherCall) > GET_WEATHER_interval || firstGetWeatherLoop ) { 

        Serial.print("Start GET-WEATHERCODE by microsecond : " + String(previousWeatherCall));
        
        getWeatherCodePlatt();

        previousWeatherCall = now;
        firstGetWeatherLoop = false;

      }

      // ++++ DISPLAY WEATHER-CODE with a refresh of approx. 10 sec. (refer to the DISPLAY_WEATHER_INTERVAL Variable)+++++
      if ((now - previousWeatherDisplay) > DISPLAY_WEATHER_interval ) {

        if (noLocationFound){

          noValidGeoCodes();

        } else {

          displayWeatherCodePlatt(savedNIGHT_JN, savedNIGHT_START, savedNIGHT_END, WetterInStund);
        
        }

        previousWeatherDisplay = now;
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
    
    // Nicht mehr anzeigen, wenn die Uhr manuell per Klich auf "Start wedder" gestartet wurde
    if (!receivedClockStart) {

      // ++++++ Set color to the actual EEPROM stored values - refreshed by new values from WEB-Client +++++
      for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
        // LEDstripArray[LEDindicator].setRGB( receivedRED, receivedGREEN, receivedBLUE);
        LEDstripArray[LEDindicator].setRGB(savedRGB_RED,savedRGB_GREEN,savedRGB_BLUE);    
      }

      FastLED.show();

    }
  
    // ++++ Refresh - Color from WEB-Client +++++ END +++++++++++++++++++++++++++++++++

  }

}
