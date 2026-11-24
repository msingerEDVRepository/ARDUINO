// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// WORDCLOCK by Michael Singer / 2025-12-20
// V 1.0 
// Containing :
// WIFI-Scan (max. 5 SSID's) and start WEB-UI with a filled listbox of existing networks
// HTTP REQUEST communication (without WEBSocket)
// for ESP32 (only!) using the EEPROM for storing the basic ceredentials (no more ESP8266 MCU's)
// the main LED / Clock steering ode is re-located into a separate "Header-file" (LEDCLOCK_PLATT.H)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#include <Arduino.h>

#include <EEPROM.h>

#include <WiFi.h>                 // needed to connect to WiFi (for ESP32)
#include <WebServer.h>            // needed to create a simple webserver (make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error)

#include <ArduinoJson.h>          // used library "ArduinoJSON" by Benoit Blanchon / Version 7.4.2 - needed for JSON encapsulation (send multiple variables with one string)

// INCLUDE - language specific Header files (for the LED CLOCK Steering and the WEB-UI)
// +++++++++ include Start ++++++++++++++++++++++++++++++++
#include "HTML_ENGLISCH.h"           // include  WEB-UI
#include "LEDCLOCK_ENGLISCH.h"       // include LED CLOCK run
// +++++++++ include Ende ++++++++++++++++++++++++++++++++

// ACCESS POINT -  Konfiguration
const char* ssidAP = "MYWORDCLOCK";
const char* passwordAP = "mywordclock";         // Only simple  password required (7 digits is minimum)

int autoConnectStartTime = 120000;            // Start intervall = 2 Minuten

int GET_TIME_interval = 10000;                 //  every 10. sec // Network time protocol     --> 6 times a minute
unsigned long previousTIMEcall  = 0;          // we use the "millis()" command for time reference and this will output an unsigned long

bool receivedClockStart = false;
String jsArray = "";

// +++++++++++++++++++ WEB-Server objects +++++++++++++++
WebServer server(80);
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++

String foundNetworks[5];
int networkCount = 0;

// +++++++++++++++ START EEPROM Vakues +++++++++++++++++++++++++++

const char*  receivedSSID = "";
const char*  receivedPASSWORD = "";
int receivedRED = 50;
int receivedGREEN = 50;
int receivedBLUE = 50;
bool receivedNIGHTJN = false;
int receivedNIGHTSTART = 0;
int receivedNIGHTEND = 0;

bool invalidCharacterExist = false;

String savedSSID = "";
String savedPASSWORD = "";
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

// +++++++++++++++++++ Ende EEPROM Values +++++++++++++++++++++++

unsigned long autoConnectStart = 0;
unsigned long clientConnectTimer = 0;

bool autoConnectStarted = false;
bool webClientActive = false;
bool WIFIConnected = false;

// Hilfsfunktion - Check auf doppelte Netzwerknamen (z.B. auf versch. Kanälen)
bool isDuplicate(String ssid, String* arr, int count) {
  for(int i=0;i<count;i++) if(arr[i]==ssid) return true;
  return false;
}

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

// WLAN Scan
void scanNetworks() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(500);

  int n = WiFi.scanNetworks();
  networkCount = 0;
  if(n>0){
    String tempSSID[n];
    int tempRSSI[n];
    for(int i=0;i<n;i++){
      tempSSID[i] = WiFi.SSID(i);
      tempRSSI[i] = WiFi.RSSI(i);
    }
    for(int i=0;i<n-1;i++){
      for(int j=i+1;j<n;j++){
        if(tempRSSI[j]>tempRSSI[i]){
          int r=tempRSSI[i]; tempRSSI[i]=tempRSSI[j]; tempRSSI[j]=r;
          String s=tempSSID[i]; tempSSID[i]=tempSSID[j]; tempSSID[j]=s;
        }
      }
    }
    for(int i=0;i<n && networkCount<5;i++){
      if(!isDuplicate(tempSSID[i], foundNetworks, networkCount)){
        foundNetworks[networkCount] = tempSSID[i];
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
    jsArray = "[";
  } else {
    jsArray = "[\"... your networkname ... \",";
  }

  for(int i=0;i<networkCount;i++){
    jsArray += "\"";
    for(unsigned int j=0;j<foundNetworks[i].length();j++){
      char c = foundNetworks[i][j];
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
  } else {
    html.replace("%%PASSWORD%%", "\"... your passwort ...\"");
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
    
    receivedNIGHTJN = jsonReceive["nightJN"];
    receivedNIGHTSTART = jsonReceive["nightStart"];
    receivedNIGHTEND = jsonReceive["nightEnd"];
    receivedClockStart = jsonReceive["clockStart"];

    EEPROM.write(ADDR_LED_RED, receivedRED);
    EEPROM.write(ADDR_LED_GREEN, receivedGREEN);
    EEPROM.write(ADDR_LED_BLUE, receivedBLUE);

    writeStringToEEPROM (ADDR_SSID, receivedSSID);
    writeStringToEEPROM (ADDR_PASSWORD, receivedPASSWORD);

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

  savedNIGHT_JN = EEPROM.read(ADDR_NIGHTJN);
  savedNIGHT_START = EEPROM.read(ADDR_NIGHT_START);
  savedNIGHT_END = EEPROM.read(ADDR_NIGHT_END);

  Serial.println("Gelesene Werte SSID: " + savedSSID + " PW: " + savedPASSWORD + " im HandleSave.");

  // ++++++++++++++++++++++++++++

  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert RED_RGB_VALUE:" + String(savedRGB_RED));
  Serial.println("Gespeichert RED_RGB_VALUE:" + String(savedRGB_GREEN));
  Serial.println("Gespeichert RED_RGB_VALUE:" + String(savedRGB_BLUE));
  Serial.println("NightMode(JN):" + String(savedNIGHT_JN) + " NightStart:" + String(savedNIGHT_START) + " NightEnd:" + String(savedNIGHT_END));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");
  Serial.println("Gespeichert SSID:" + String(receivedSSID) + " PW:" + String(receivedPASSWORD));
  Serial.println("Starte Uhr received (JN):" + String(receivedClockStart));
  Serial.println("++++++++++++++++++++++++++++++++++++++++");

  server.send(200,"text/plain","OK");

}

// +++++++++++++++ Auto-Connect ++++++++++++++++++++++++++++
bool autoConnect() {
  if(savedSSID.length()>0 && savedPASSWORD.length()>0){
    Serial.println("Versuche Verbindung zu "+ savedSSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPASSWORD.c_str());
    int attempts=0;
    while(WiFi.status()!=WL_CONNECTED && attempts<30){
      delay(500);
      Serial.print(".");
      attempts++;
    }
    if(WiFi.status()==WL_CONNECTED){
      Serial.println("\nVerbunden! IP: "+WiFi.localIP().toString());
      WiFi.softAPdisconnect(true); // AP Modus deaktivieren
      Serial.println("AP Modus deaktiviert");
      WIFIConnected = true;
      return true;
    } else {
      Serial.println("\nVerbindung fehlgeschlagen.");
      return false;
    }
  }
  return false;
}

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

  server.on("/",handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  savedRGB_RED = EEPROM.read(ADDR_LED_RED);
  savedRGB_GREEN = EEPROM.read(ADDR_LED_GREEN);
  savedRGB_BLUE = EEPROM.read(ADDR_LED_BLUE);
  savedSSID = readStringFromEEPROM(ADDR_SSID);
  savedPASSWORD = readStringFromEEPROM(ADDR_PASSWORD);
  savedNIGHT_JN = EEPROM.read(ADDR_NIGHTJN);
  savedNIGHT_START = EEPROM.read(ADDR_NIGHT_START);
  savedNIGHT_END = EEPROM.read(ADDR_NIGHT_END);

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

  server.handleClient();

  // Non-blocking Auto-Connect nach 1 Minute
  if (receivedClockStart || (autoConnectStarted && !webClientActive  &&  (millis() - autoConnectStart >= autoConnectStartTime))){
    
    autoConnectStarted = false;

    if(!WIFIConnected){

      runCountdownPlatt();

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

      if (timeInfoSuccess){
        runClockPlatt();

      }

      previousTIMEcall = now;

    }

  }

  if (!WIFIConnected && webClientActive) {
    // ++++++ Set color to the actual EEPROM stored values - refreshed by new values from WEB-Client +++++
    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
      // LEDstripArray[LEDindicator].setRGB( receivedRED, receivedGREEN, receivedBLUE);
      LEDstripArray[LEDindicator].setRGB(savedRGB_RED,savedRGB_GREEN,savedRGB_BLUE);    
    }

    FastLED.show();
    // ++++ Refresh - Color from WEB-Client +++++ END +++++++++++++++++++++++++++++++++

  } 

  if (WIFIConnected && !timeInfoSuccess){

    waitForTimeSuccess();

  }

}
