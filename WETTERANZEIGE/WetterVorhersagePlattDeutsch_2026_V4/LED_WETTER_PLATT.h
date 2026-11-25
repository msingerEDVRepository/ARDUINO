#ifndef LED_WETTER_PLATT_H
#define LED_WETTER_PLATT_H

#include <Arduino.h>

#define FASTLED_INTERNAL
#define FASTLED_ALLOW_INTERRUPTS 0

#include <FastLED.h>        // Used library FastLED (by Daniel Garcia / Version 3.10.10) - used for programming the LED stripes

#define DATA_PIN            21 // (for ESP32 only!)
#define NUMPIXELS           169
// Strombegrenzung für den gesamten LED-Streifen (wird in initLED() an FastLED übergeben).
// Richtwerte: ESP32 am USB-Port  -> ca. 300 (USB liefert nur 500 mA, der ESP32 braucht mit WLAN selbst ~150-250 mA)
//             externes 5V-Netzteil -> z.B. 2000 (bei 3 A Netzteil) - ESP32 UND Streifen am selben Netzteil, GND verbinden!
// Hinweis: FastLED rechnet ca. 1 mA pro LED Ruhestrom mit (169 LEDs = ~170 mA), auch wenn alle LEDs "aus" sind.
#define MAX_POWER_VOLTS     5
#define MAX_POWER_MILLIAMPS 500
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
#define BRIGHTNESS          150

#include <EEPROM.h>

#define EEPROM_SIZE 512
// EEPROM Adressen
#define ADDR_LED_RED 230
#define ADDR_LED_GREEN 240
#define ADDR_LED_BLUE 250
#define ADDR_NIGHTJN 260
#define ADDR_NIGHT_START 270
#define ADDR_NIGHT_END 290

int savedRGBRED = 0;
int savedRGBGREEN = 0;
int savedRGBBLUE = 0;

bool savedNIGHTJN = false;
int savedNIGHTSTART = 0;
int savedNIGHTEND = 0;

bool nightSwitchOff = false;

// ++++ for NTP Server Connection and readtime method

#include "time.h"
#include "esp_sntp.h"
//#include "stdlib.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>   // needed for HTTPS (OpenStreetMap / Nominatim)
#include <memory>

// ++++++++++++ Init LED Strip ++++++++++++++++++++++++++++

// Define the LED Strip (Array) based on the FAST-LED Library
CRGB LEDstripArray[NUMPIXELS];

int RGB_RED = 0;
int RGB_GREEN = 0;
int RGB_BLUE = 0;

int oldRED = 0;
int oldGREEN = 0;
int oldBLUE = 0;

int   brightnessLED = BRIGHTNESS;
bool  letterchanged = false;

// +++++++++++++++ Ende LED Strip +++++++++++++++++++++++++++

// ++++++ OPEN METEO Section START +++++++++++++

bool isToday = true;
String previousLOCATION_PLZ = "";
String previousLOCATION_NAME = "";

String MeteoAPI;
float Latitude = 0;
float Longitude = 0;

bool noLocationFound = false;

unsigned int aktuellerTAG = 0;
unsigned int morgigerTAG = 0;

unsigned int aktuellerMONAT = 0;
unsigned int naechsterMONAT = 0;

unsigned int aktuellesJAHR = 0;
unsigned int naechstesJAHR = 0;

// String METEOLocationName = "";
String json_array_METEO;

int todayWeather[24];    //Array für die stündlichen WeatherCodes von Heute
int tomorrowWeather[24];    //Array für die stündlichen WeatherCodes von Morgen (wegen Datumswechsel für 3h Forecast nach 21 Uhr)

unsigned int todaySunriseHour = 0;
unsigned int todaySunriseMinute = 0;

unsigned int todaySunsetHour = 0;
unsigned int todaySunsetMinute = 0;     

unsigned int WetterCodeStunde1 = 0;    // OPEN METEO Stunde ist 0h - 23h  (in Een Stünd Display)
unsigned int WetterCodeStunde2 = 0;    // OPEN METEO Stunde ist 0h - 23h  (in Twee Stünd Display)
unsigned int WetterCodeStunde3 = 0;    // OPEN METEO Stunde ist 0h - 23h  (in Dree Stünd Display)

unsigned int WetterCode1 = 10;         // 10, because this is 
unsigned int WetterCode2 = 10;
unsigned int WetterCode3 = 10;

unsigned int WetterCodeDisplayedOld = 10;
unsigned int WetterCodeDisplayed = 10;
unsigned int weatherCodeLED = 10;


// ++++++ OPEN METEO Section ENDE +++++++++++++

// +++++++++++++ START WORDCLOCK Variables section ++++++++++++
int randomWeatherTableIndex = 0;
int randomHour = 0;

unsigned int aktuelleStunde12h = 0;
unsigned int aktuelleStunde24h = 0;

unsigned int alteStunde12h = 0;
unsigned int alteStunde24h = 0;

unsigned int aktuelleMinute = 0;
unsigned int alteMinute = 0;

unsigned int summerTimeActive = 0;                // summerTimeActive = 0 == "Winterzeit"

// +++++++++++ WEDDERCLOCK Array for displaying time in "PLATT-Deutsch" +++++++++++

//Adresse der ersten und letzten LED
const int ersteLED = 0;
const int letzteLED = NUMPIXELS - 1;

unsigned int LEDindicator = 0;
unsigned int weatherIndicator = 0;

unsigned int newletter[NUMPIXELS];
unsigned int shownletter[NUMPIXELS];

unsigned int pointer = 0;

bool newdisplay = LOW;
bool setDisplayOff = LOW;

// +++++ Adressen für die Wetter-Vorhersage via OPEN METEO ++++ START ++++++++++

const int NumberInStunde  = 8;
const int NumberOfWord1   = 4;
const int NumberOfWord2   = 4;
const int NumberOfWord3   = 11;
const int NumberOfWord4   = 3;
const int NumberOfWord5   = 4;
const int NumberOfWord6   = 2;
const int NumberOfWord7   = 1;
const int NumberOfWord8   = 2;
const int NumberOfWord9   = 1;

// Adressen in xx Stünd
const int InStundeAddr[8][2] = { {0, 1},            //  IN
                                {5, 7},             //  EEN
                                {3, 6},             //  TWEE
                                {8, 11},            //  DREE
                                {21, 25},           //  STÜNN
                                {19, 25},           //  STÜNNEN
                                {13, 17},           //  GIFFT
                                {27, 29} };         //  DAT           

//Adressen der Bannig/Beeten
const int word1Addr[4][2] = { {39, 43},             //  BETEN
                              {46, 51},             //  BANNIG
                              {31, 32},             //  AF
                              {44, 45} };           //  NE       

//Adressen veel
const int word2Addr[4][2] = { {52, 55},             //  VEEL
                              {110, 116},           //  IESIGEN
                              {34, 35},             //  UN
                              {72, 76} };           //  KLORE

//Adressen Wetterart1
const int word3Addr[11][2] = { {78, 81},            //  SÜNN
                              {117, 122},           //  WULKEN
                              {104, 108},           //  NEVEL
                              {130, 141},           //  DRÖPPELREGEN
                              {137, 141},           //  REGEN                         
                              {123, 126},           //  SNEE
                              {91, 101},            //  SNEEGRIESEL
                              {37, 38},             //  AN
                              {61, 64},             //  WIND                              
                              {56, 60},             //  STORM
                              {66, 70} };           //  NACHT      

//Adressen Mit/Un
const int word4Addr[3][2] = { {83, 85},             //  MIT
                              {46, 51},             //  BANNIG
                              {128, 129} };         //  UN

 //Adressen BOBEN
const int word5Addr[4][2] = { {87, 90},             //  POOR
                              {39, 43},             //  BETEN
                              {52, 55},             //  VEEL                              
                              {150, 155} };         //  DONNER                                                                         
//Adressen Wetterart2
const int word6Addr[2][2] = { {117, 122},           //  WULKEN
                              {137, 141} };         //  REGEN      

//Adressen BLIEV
const int word7Addr[1][2] = { {144, 148} };         //  BLIEV

//Adressen BUTEN/TOHUUS
const int word8Addr[2][2] = { {156, 157},           //  TO
                              {164, 168} };         //  BUTEN

//Adressen BUTEN/TOHUUS
const int word9Addr[1][2] = { {159, 162} };         //  HUUS    


// Zeittabelle mit Takt +2/-2 Minuten
// ++++++++++++++++++++++++++++++++++
// Array Element0 = WeatherCode from OPEN METEO           (refer to inline - Comments)
// Array Element1 = Element aus Array "word1Addr"         (example = 1="BETEN", 2="BANNIG", 3 = "AF", 4 = "NE")
// Array Element2 = Element aus Array "word2Addr"         (example = 1="VEEL", 2="IESIGEN", 3 = "UN", 4 = "KLORE")
// Array Element3 = Element aus Array "word3Addr"         (example = 1="SÜNN", 2="WULKEN", 3 = "NEVEL", 4 = "DRÖPPELREGEN", 5 = "REGEN", 6 = "SNEE", 7 = "SNEEGRIESEL", 8 = "AN, 9 = "WIND", 10="STORM", 11= "NACHT")
// Array Element4 = Element aus Array "word4Addr"         (example = 1="MIT", 2="BANNIG" , 3 = "UN")
// Array Element5 = Element aus Array "word5Addr"         (example = 1="POOR", 2="BETEN", 3 = "VEEL", 4 = "DONNER")
// Array Element6 = Element aus Array "word6Addr"         (example = 1="WULKEN", 2="REGEN")
// Array Element7 = Element aus Array "word7Addr"         (example = 1="BLIEV")
// Array Element8 = Element aus Array "word8Addr"         (example = 1="TO", 2 ="BUTEN")
// Array Element9 = Element aus Array "word9Addr"         (example = 1="HUUS")
//
// ACHTUNG - Für die Unterscheidung zwischen TAG = (WetterCode + 100) und NACHT = (WetterCode + 200) 
//
// ++++++++++++++++++++++++++++++++++
// ++++ (Alle Elemente sind m.E. um "1" verschoben, da der erste Eintrag = "0" == "keine Anzeige" bedeutet) +++++

const int WeatherTable[35][10] = { 
                                  {100,2,1,1,0,0,0,1,2,0},
                                  {200,4,4,11,0,0,0,0,0,0},
                                  {101,0,1,1,1,1,1,1,2,0},
                                  {201,4,4,11,1,1,1,0,0,0},
                                  {102,1,0,1,1,0,1,1,2,0},
                                  {202,4,0,11,1,0,1,0,0,0},
                                  {103,2,1,2,0,0,0,0,0,0},
                                  {145,1,0,3,0,0,0,0,0,0},
                                  {146,0,1,3,0,0,0,0,0,0},
                                  {147,0,1,3,0,0,0,0,0,0},
                                  {148,2,1,3,0,0,0,1,1,1},
                                  {151,1,0,4,0,0,0,0,0,0},
                                  {153,0,1,4,0,0,0,0,0,0},
                                  {155,2,1,4,0,0,0,1,1,1},
                                  {156,1,2,4,0,0,0,0,0,0},
                                  {157,2,2,4,0,0,0,1,1,1},
                                  {161,1,0,5,0,0,0,0,0,0},
                                  {163,0,1,5,0,0,0,1,1,1},
                                  {165,2,1,5,0,0,0,1,1,1},
                                  {166,1,2,5,0,0,0,0,0,0},
                                  {167,2,2,5,0,0,0,1,1,1},
                                  {171,1,0,6,0,0,0,0,0,0},
                                  {173,0,1,6,0,0,0,0,0,0},
                                  {175,2,1,6,0,0,0,1,1,1},
                                  {177,1,0,7,0,0,0,0,0,0},
                                  {180,3,3,8,0,2,2,0,0,0},
                                  {181,3,3,8,0,0,2,0,0,0},
                                  {182,3,3,8,2,3,2,1,1,1},
                                  {185,1,0,6,0,0,2,0,0,0},
                                  {186,2,1,6,0,0,2,1,1,1},
                                  {195,0,1,9,3,4,0,1,1,1},
                                  {196,2,1,9,3,4,0,1,1,1},
                                  {197,0,0,10,3,4,0,1,1,1},
                                  {198,2,0,10,3,4,0,1,1,1},
                                  {199,2,0,10,3,4,0,1,1,1} 
                                  };

// +++++ Adressen für die Wettervorhersage ++++ ENDE ++++++++++++


// +++++++++++ Init LED-Strip +++++++++++++++++++++++++++++++++++
void initLED(int savedRGBRED, int savedRGBGREEN, int savedRGBBLUE){

  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(LEDstripArray, NUMPIXELS);
  FastLED.setMaxPowerInVoltsAndMilliamps(MAX_POWER_VOLTS, MAX_POWER_MILLIAMPS);   // FastLED dimmt automatisch herunter, wenn zu viel Strom nötig wäre
  FastLED.setBrightness(BRIGHTNESS);
  randomSeed(analogRead(0));

    // ++++++++++ INIT of WORD-Clock Array ++++++++++++++++++
 
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    newletter[LEDindicator] = 0;   
  }

  LEDindicator = 0;

  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    shownletter[LEDindicator] = 0;   
  }

  LEDindicator = 0;

  // +++++++++++ End of INIT Word-Clock LED Array ++++++++++++

  // alle LED einschalten, um die LED Color & Brightness zu sehen (die aus dem EEPROM übernommen und dann angepasst wird...)     
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
    LEDstripArray[LEDindicator].setRGB( savedRGBRED, savedRGBGREEN, savedRGBBLUE);
    FastLED.show();
    oldRED = savedRGBRED;
    oldGREEN = savedRGBGREEN;
    oldBLUE = savedRGBBLUE;
  }

}

// +++++++++++++ init der LED und LED Array +++ Ende +++++++++++


// +++++++++++++ NTP Server for getting DATE & TIME ++++++++++++

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
//const long  gmtOffset_sec = 3600;
// int   daylightOffset_sec = 3600;

int timeInfoSuccess = 0;
const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)


void readLocalTime()
{
  struct tm timeinfo;

  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  } else {
    Serial.println("NTP Time available (yet)");
    timeInfoSuccess = 1;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println("");
  aktuelleStunde24h = timeinfo.tm_hour;

  aktuellerTAG = timeinfo.tm_mday;
  aktuellerMONAT = timeinfo.tm_mon + 1;
  aktuellesJAHR = timeinfo.tm_year + 1900;

  morgigerTAG = 0;
  naechsterMONAT = 0;
  naechstesJAHR = 0;

  switch (aktuellerMONAT){
      case 1:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;
        if (morgigerTAG > 31){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 2: // Schaltjahr wird ingnoriert (im Moment)
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 28){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 3:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 31){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 4:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 30){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 5:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 31){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 6:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 30){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 7:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 31){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 8:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 31){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 9:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 30){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 10:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 31){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 11:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 30){
            morgigerTAG = 1;
            naechsterMONAT = aktuellerMONAT + 1;
        }
      break;
      case 12:
        morgigerTAG = aktuellerTAG + 1;
        naechsterMONAT = aktuellerMONAT;
        naechstesJAHR = aktuellesJAHR;        
        if (morgigerTAG > 31){
            morgigerTAG = 1;
            naechsterMONAT = 1;
            naechstesJAHR = aktuellesJAHR + 1;
        }
      break;

  }

  Serial.println ("Die aktuelle Stunde (24h) ist : " + String(aktuelleStunde24h));

  Serial.println ("Der aktuelle TAG ist : " + String(aktuellerTAG)); 
  Serial.println ("Der aktuelle MONAT ist : " + String(aktuellerMONAT));
  Serial.println ("Das aktuelle JAHR ist : " + String(aktuellesJAHR));

  Serial.println ("Der morgige TAG ist : " + String(morgigerTAG)); 
  Serial.println ("Der nächste MONAT ist : " + String(naechsterMONAT));
  Serial.println ("Das nächste JAHR ist : " + String(naechstesJAHR));
  
  aktuelleMinute = timeinfo.tm_min;
  Serial.println ("Die aktuelle Minute ist : " + String(aktuelleMinute));
  summerTimeActive = timeinfo.tm_isdst;
  Serial.println ("Sommerzeit aktiv (J/N) : " + String(summerTimeActive));

}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t) 
{
  Serial.println("Got time adjustment from NTP!");
  readLocalTime();
}
// ++++++++++++++ NTP Server end ++++++++++++++++++++++++++

// +++++++++++++++ NTP Server section +++++++++

// set notification call-back function
//sntp_set_time_sync_notification_cb( timeavailable );

/**
  * NTP server address could be aquired via DHCP,
  *
  * NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
  * otherwise SNTP option 42 would be rejected by default.
  * NOTE: configTime() function call if made AFTER DHCP-client run
  * will OVERRIDE aquired NTP server address
  */
//sntp_servermode_dhcp(1);    // (optional)

  /**
  * This will set configured ntp servers and constant TimeZone/daylightOffset
  * should be OK if your time zone does not need to adjust daylightOffset twice a year,
  * in such a case time adjustment won't be handled automagicaly.
  */
//configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  /**
  * A more convenient approach to handle TimeZones with daylightOffset 
  * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
  * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
  */
//configTzTime(time_zone, ntpServer1, ntpServer2);

// +++++++++++++++ NTP Server section end +++++++++

// ++++++++ RESET -  LED ARRAY  ++++ START +++++++++++
void ResetArray(int ArraySize, const int Addr[][2], unsigned int *Array){

  int c1 = 0;
  int c2 = 0;
  
  //Array durchlaufen
  for(c1 = 0; c1 <= ArraySize - 1; c1++) {

    //Bereich auf Null setzen
    for(c2 = Addr[c1][0]; c2 <= Addr[c1][1]; c2++) {
      Array[c2] = 0;
    }
  }
}

// ++++++++ RESET LED ARRAY  ++++ END +++++++++++++

// ++++++++ Write LED ARRAY  ++++ START +++++++++++

void WriteArray(int Pointer, const int Addr[][2], unsigned int *Array, int Value){

  int c1 = 0;

  //Array-Bereich beschreiben
  for(c1 = Addr[Pointer][0]; c1 <= Addr[Pointer][1]; c1++) {
    Array[c1] = Value;
  }
}

// ++++++++ Write LED ARRAY  ++++ END +++++++++++++

//  +++++++++ START - OPEN METEO Weather Info ++++++++++++++++++++

void JsonCONVlocation() {

    JsonDocument doc;

    // The filter: it contains "true" for each value we want to keep
    JsonDocument filter;
    filter[0]["lat"] = true;
    filter[0]["lon"] = true;
  
    DeserializationError error = deserializeJson(doc, json_array_METEO, DeserializationOption::Filter(filter));

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    // TEST - Ausgabe des gefilterten JSON Strings an das SERIAL Object
    serializeJsonPretty(doc, Serial);

    String METEOlatitude = doc[0]["lat"];
    Serial.println("METEOlatitude hat folgenden Wert:" + METEOlatitude);

    String METEOlongitude = doc[0]["lon"];
    Serial.println("METEOlongitude hat folgenden Wert:" + METEOlongitude);

    Latitude = METEOlatitude.toFloat();
    Longitude = METEOlongitude.toFloat();
    
}

void JsonCONVWeatherCodes(bool isToday) {

    StaticJsonDocument<780> doc;

    // The filter: it contains "true" for each value we want to keep
    JsonDocument filter;
    filter["hourly"]["weather_code"] = true;
  
    DeserializationError error = deserializeJson(doc, json_array_METEO, DeserializationOption::Filter(filter));

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    // TEST - Ausgabe des gefilterten JSON Strings an das SERIAL Object
    //serializeJsonPretty(doc, Serial);

    if (isToday){
      for (int i=0; i<=23; i++){
        todayWeather[i] = doc["hourly"]["weather_code"][i];
      }      
    } else{
      for (int i=0; i<=23; i++){
        tomorrowWeather[i] = doc["hourly"]["weather_code"][i];
      }
    }

}

void JsonCONVSunRise(bool isToday) {

  StaticJsonDocument<100> doc;

  // The filter: it contains "true" for each value we want to keep
  JsonDocument filter;
  filter["daily"]["sunrise"] = true;

  DeserializationError error = deserializeJson(doc, json_array_METEO, DeserializationOption::Filter(filter));

  if (error) {
      Serial.print(F("deserializeJson() SUNRISE failed: "));
      Serial.println(error.f_str());
      return;
  }

  // TEST - Ausgabe des gefilterten JSON Strings an das SERIAL Object
  //serializeJsonPretty(doc, Serial);

  if (isToday){

    String todaySunrise = doc["daily"]["sunrise"];
    
    Serial.println("Sunrise Output = " + String(todaySunrise));

    int indexofTime = todaySunrise.indexOf("T");

    todaySunriseHour = (todaySunrise.substring((indexofTime + 1),(indexofTime + 3) ).toInt());
    todaySunriseMinute = (todaySunrise.substring((indexofTime + 4),(indexofTime + 6) ).toInt());

  }
}

void JsonCONVSunSet(bool isToday) {

  StaticJsonDocument<100> doc;

  // The filter: it contains "true" for each value we want to keep
  JsonDocument filter;
  filter["daily"]["sunset"] = true;

  DeserializationError error = deserializeJson(doc, json_array_METEO, DeserializationOption::Filter(filter));

  if (error) {
      Serial.print(F("deserializeJson() SUNSET failed: "));
      Serial.println(error.f_str());
      return;
  }

  // TEST - Ausgabe des gefilterten JSON Strings an das SERIAL Object
  //serializeJsonPretty(doc, Serial);

  if (isToday){

    String todaySunset = doc["daily"]["sunset"];
    
    Serial.println("Sunset Output = " + String(todaySunset));

    int indexofTime = todaySunset.indexOf("T");

    todaySunsetHour = (todaySunset.substring((indexofTime + 1),(indexofTime + 3) ).toInt());
    todaySunsetMinute = (todaySunset.substring((indexofTime + 4),(indexofTime + 6) ).toInt());

  }
}

String SetupLocationApi(String locationPLZ, String locationName) {

// Get GeoCode by Location-Name:
//  OPEN METEO: (hier am Beispiel (DE) "Kakenstorf")
// http://geocoding-api.open-meteo.com/v1/search?name=Kakenstorf&count=1&language=de&format=json
// https://geocoding-api.open-meteo.com/v1/search?name=Hamburg&countryCode=DE&count=10&language=en&format=json
// NEU (2026-04-08) für OPENSTREETMAP: https://nominatim.openstreetmap.org/search?postalcode=21224&city=Rosengarten&country=Germany&format=json

  String locationAPI = "";

    // BISHER für OPEN-METEO Geocode API
    // locationAPI = "https://geocoding-api.open-meteo.com/v1/search?name=(LOCATION)&countryCode=DE&count=1&language=en&format=json";

    // NEU (2026-04-08) für OPENSTREETMAP
    locationAPI = "https://nominatim.openstreetmap.org/search?postalcode=(PLZ)&city=(NAME)&country=Germany&format=json&limit=1";

    String locationplz = String(locationPLZ);
    locationplz.trim();
    String locationname = String(locationName);
    locationname.trim();
    Serial.print("LOCATION (PLZ & Name): ");Serial.println( (locationplz) + " - " + (locationname) );

    locationAPI.replace("(PLZ)",locationplz);
    locationAPI.replace("(NAME)",locationname);

  return locationAPI;
    
}

String SetupMeteoApi(bool isToday) {

    // für Kakenstorf
    //http://api.open-meteo.com/v1/forecast?latitude=53.3032&longitude=9.7629&hourly=weather_code&timezone=Europe%2FBerlin&start_date=2024-08-22&end_date=2024-08-22

    String API = "";

    // TODAY
    char TodayDate[10];
    // TOMORROW
    char TomorrowDate[10];

    if (isToday == true){
      //API = "http://api.open-meteo.com/v1/forecast?latitude=53.3032&longitude=9.7629&hourly=weather_code&timezone=Europe%2FBerlin&start_date=(TODAY)&end_date=(TODAY)";
      // API (NEW): http://api.open-meteo.com/v1/forecast?latitude=53.3032&longitude=9.7629&daily=sunrise,sunset&hourly=weather_code&timezone=Europe%2FBerlin&start_date=2025-12-25&end_date=2025-12-25
      // mit der DIALY-Angabe zu SUNRISE und SUNSET
      API = "http://api.open-meteo.com/v1/forecast?latitude=(LATITUDE)&longitude=(LONGITUDE)&daily=sunrise,sunset&hourly=weather_code&timezone=Europe%2FBerlin&start_date=(TODAY)&end_date=(TODAY)";
      sprintf(TodayDate,"%04u-%02u-%02u",aktuellesJAHR,aktuellerMONAT,aktuellerTAG);
      Serial.print("TodayDate: ");Serial.println(TodayDate);
      String Today = String(TodayDate);
      Today.trim();
      API.replace("(TODAY)",String(Today));
      API.replace("(LATITUDE)",String(Latitude));
      API.replace("(LONGITUDE)",String(Longitude));
    } else{
      //API = "http://api.open-meteo.com/v1/forecast?latitude=53.3032&longitude=9.7629&hourly=weather_code&timezone=Europe%2FBerlin&start_date=(TOMORROW)&end_date=(TOMORROW)";
      API = "http://api.open-meteo.com/v1/forecast?latitude=(LATITUDE)&longitude=(LONGITUDE)&daily=sunrise,sunset&hourly=weather_code&timezone=Europe%2FBerlin&start_date=(TOMORROW)&end_date=(TOMORROW)";
      sprintf(TomorrowDate,"%04u-%02u-%02u",naechstesJAHR,naechsterMONAT,morgigerTAG);
      Serial.print("TomorrowDate: ");Serial.println(TomorrowDate);
      String Tomorrow = String(TomorrowDate);
      Tomorrow.trim();
      API.replace("(TOMORROW)",String(Tomorrow));
      API.replace("(LATITUDE)",String(Latitude));
      API.replace("(LONGITUDE)",String(Longitude));
    }

    return API;
    
}

String GET_Request(const char* METEO_URL) {

  String payload = "{}";

  if (WiFi.status() == WL_CONNECTED) {

    String url = String(METEO_URL);

    // WICHTIG: der Client muss VOR "http" deklariert werden, damit er NACH "http" zerstört wird.
    // (sonst greift der Destruktor von HTTPClient auf einen schon gelöschten Client zu --> Absturz in lwIP)
    std::unique_ptr<WiFiClient> client;

    if (url.startsWith("https://")) {
      // Nominatim (OpenStreetMap) erlaubt nur HTTPS
      WiFiClientSecure* secureClient = new WiFiClientSecure();
      secureClient->setInsecure();   // keine Zertifikatsprüfung (reicht hier, es werden nur Koordinaten geladen)
      client.reset(secureClient);
    } else {
      client.reset(new WiFiClient());  // OPEN-METEO weiter per HTTP
    }

    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    Serial.println("METEO_URL für den HTTP.BEGIN Aufruf: " + url);

    http.begin(*client, url);

    // Nominatim verlangt einen identifizierenden User-Agent (sonst HTTP 403)
    http.setUserAgent("MienWedder/3.0 (ESP32 Wortuhr)");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(10000);

    Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        payload = http.getString();
      } else {
        Serial.println("[HTTP] Antwort: " + http.getString());
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }

  return payload;

}

//  +++++++++ END - OPEN METEO Weather Info ++++++++++++++++++++

// +++++++++++ START WAIT for WIFI Connect +++++++++++++++++++++
void waitForWLANconnect(){

  LEDstripArray[58].setRGB(100,0,0);
  LEDstripArray[71].setRGB(100,0,0);
  LEDstripArray[84].setRGB(100,0,0);
  LEDstripArray[97].setRGB(100,0,0);
  LEDstripArray[110].setRGB(100,0,0); 

  // EINBLENDEN
  for (int k = 0; k <= brightnessLED; k++) {
    FastLED.setBrightness(k);
    FastLED.delay(20);
    FastLED.show();
  }

  delay(250);
  
  // AUSBLENDEN
  for (int k = brightnessLED; k >= 0; k--) {
    FastLED.setBrightness(k);
    FastLED.delay(10);
    FastLED.show();
  }

  delay(100);

}

// +++++++++++ ENDE WAIT for WIFI Connect +++++++++++++++++++++

// ++++++++++ LED Wait during NTP Server-Connect (after WLAN is connected)
void waitForTimeSuccess(){

  LEDstripArray[58].setRGB(0,100,0);
  LEDstripArray[71].setRGB(0,100,0);
  LEDstripArray[84].setRGB(0,100,0);
  LEDstripArray[97].setRGB(0,100,0);
  LEDstripArray[110].setRGB(0,100,0);

  // EINBLENDEN
  for (int k = 0; k <= brightnessLED; k++) {
    FastLED.setBrightness(k);
    FastLED.delay(20);
    FastLED.show();
  }

  delay(250);
  
  // AUSBLENDEN
  for (int k = brightnessLED; k >= 0; k--) {
    FastLED.setBrightness(k);
    FastLED.delay(10);
    FastLED.show();
  }

  delay(100);

}

// ++++++ END +++++ LED Wait  +++++++++++++++++++++++++++++++

// ++++++++++ LED Wait during NTP Server-Connect (after WLAN is connected)
void noValidGeoCodes(){

  LEDstripArray[58].setRGB(0,0,100);
  LEDstripArray[71].setRGB(0,0,100);
  LEDstripArray[84].setRGB(0,0,100);
  LEDstripArray[97].setRGB(0,0,100);
  LEDstripArray[110].setRGB(0,0,100);

  // EINBLENDEN
  for (int k = 0; k <= brightnessLED; k++) {
    FastLED.setBrightness(k);
    FastLED.delay(20);
    FastLED.show();
  }

  delay(250);
  
  // AUSBLENDEN
  for (int k = brightnessLED; k >= 0; k--) {
    FastLED.setBrightness(k);
    FastLED.delay(10);
    FastLED.show();
  }

  delay(100);

}

// ++++++ END +++++ LED Wait  +++++++++++++++++++++++++++++++

// ++++++++ Check Nachabschaltung ++++ START +++++++++++

bool nightSwitchOffCheck() {


  // +++++++++GET EEPROM stored values
  savedNIGHTJN = EEPROM.read(ADDR_NIGHTJN);
  savedNIGHTSTART = EEPROM.read(ADDR_NIGHT_START);
  savedNIGHTEND = EEPROM.read(ADDR_NIGHT_END);

  // UND Überprüfung des NIGHTMODE (WEnn er nicht(!) aktiviert ist 
  // ODER wenn er aktiviert ist , aber die Zeit nicht(!) zwischen 00:00 Uhr und 05:59 Uhr ist
  // DANN Schalte die LED Anzeige frei (anderfalls schalte alle LED aus.)
  //TEST // ORIGIN:  if(retrievedNightMode == 0 || (retrievedNightMode == 1 && aktuelleStunde24h >= 6) ) {
  //if(retrievedNightMode == 0 || (retrievedNightMode == 1 && (aktuelleStunde24h == 14 || aktuelleStunde24h == 16) ) ) {

  // Check retrievedNightStart und retrievedNightEnd Values auf "Uhrzeit-Stunde" - wegen "leerer" EEPROM Werte.
  if (savedNIGHTSTART > 23) {
    savedNIGHTSTART = 0;      // Standard - Abschaltung um Mitternacht
  }

  if (savedNIGHTEND > 9) {
    savedNIGHTEND = 6;        // Standard Re-Start morgens um 06:00 Uhr
  }

  // Init SwitchOff für NightMode
  nightSwitchOff = false;

  // Ermittle Abschalt-Intervall START für Nightmode bis Mitternacht
  // NightMode - Startwert liegt zwischen 21:00 Uhr und 23:00 Uhr 
  if (aktuelleStunde24h >= 21 && savedNIGHTSTART >= 21) {        
    if (aktuelleStunde24h >= savedNIGHTSTART) {
      nightSwitchOff = true;
    }
  }
  // Ermittle Abschalt-Intervall START & ENDE für Nightmode nach Mitternacht  
  // (Case 1a) ... oder NightMode - Startwert liegt zwischen 00:00 Uhr und 02:00 Uhr 
  // (Case 1b) ... oder NightMode - Startwert liegt zwischen 21:00 Uhr und 23:00 Uhr 
  // (Case 2) ... zwischen 02:01 und 03:59 Uhr ist die Uhr immer AUS (Ausnahme Night-Mode == OFF)
  // (Case 3) ... zwischen 04:00 und 09:00 muss auf der Wert für Nightmode - END geprüft werden.
  if (aktuelleStunde24h < 3 ) {

    // Case (1a)
    if (savedNIGHTSTART <= 2 ) {
      if (aktuelleStunde24h >= savedNIGHTSTART) {
        nightSwitchOff = true;
      }    
    }
    // Case (1b)
    if (savedNIGHTSTART >= 21 ) {
      nightSwitchOff = true; 
    }
  }

  // Case (2)
  if (aktuelleStunde24h == 3 ) {
    nightSwitchOff = true; 
  }
    
    // Case (3)
  if (aktuelleStunde24h > 3  && aktuelleStunde24h < 10) {

    if (aktuelleStunde24h >= savedNIGHTEND) {
      nightSwitchOff = false;
    } else {
      nightSwitchOff = true;
    }

  }

  return nightSwitchOff;

}

// ++++++++ Check Nachabschaltung  ++++ END +++++++++++++

// +++++++++ LED WETTERCODE - COUNTDOWN - START +++++++++++++++++++++++++++++

void runCountdownPlatt() {

  savedRGBRED = EEPROM.read(ADDR_LED_RED);
  savedRGBGREEN = EEPROM.read(ADDR_LED_GREEN);
  savedRGBBLUE = EEPROM.read(ADDR_LED_BLUE);

  // alle LED zunächst einmal ausschalten
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
    LEDstripArray[LEDindicator] = CRGB::Black;
  }
  FastLED.show();
  delay(25);

  // DEMO - MODE: for  (randomWeatherTableIndex = 31; randomWeatherTableIndex >= 0; randomWeatherTableIndex = randomWeatherTableIndex - 1 ) {
  // NORMAL - MODE : der Countdown ist 3x schneller durch die WetterCodes, weil er Werte überspringt
  // NORMAL - MODE: for  (randomWeatherTableIndex = 31; randomWeatherTableIndex >= 0; randomWeatherTableIndex = randomWeatherTableIndex - 3 ) {
  for  (randomWeatherTableIndex = 34; randomWeatherTableIndex >= 0; randomWeatherTableIndex = randomWeatherTableIndex - 3 ) {

    Serial.println("Start - Countdown:" + String(randomWeatherTableIndex));

    // ++++++++ Neuer Countdown START ++++++++++++++++++++++

    //Adressbereiche im Array initialisieren
    ResetArray(NumberInStunde, InStundeAddr, newletter);

    ResetArray(NumberOfWord1, word1Addr, newletter);
    ResetArray(NumberOfWord2, word2Addr, newletter);
    ResetArray(NumberOfWord3, word3Addr, newletter);
    ResetArray(NumberOfWord4, word4Addr, newletter);
    ResetArray(NumberOfWord5, word5Addr, newletter);
    ResetArray(NumberOfWord6, word6Addr, newletter);
    ResetArray(NumberOfWord7, word7Addr, newletter);
    ResetArray(NumberOfWord8, word8Addr, newletter);
    ResetArray(NumberOfWord9, word9Addr, newletter);
    randomHour = 0;
    randomHour = random(1,4);

    // ++++++ 1. Wort "IN" ausgeben (immer) ++++++++++
    WriteArray(0, InStundeAddr, newletter, 1);

    // ++++++ 2. Wort "EEN", "TWEE" oder "DREE" ausgeben (Uhrzeit gesteuert) +++++++++++

    WriteArray(randomHour, InStundeAddr, newletter, 1);                //EEN , TWEE oder DREE STÜND'

    // ++++++ 3. Wort "STÜNN'" oder "STÜNNEN" ausgeben (immer) ++++++++++
    if (randomHour == 1){
      WriteArray(4, InStundeAddr, newletter, 1);
    } else {
      WriteArray(5, InStundeAddr, newletter, 1);
    }; // 4 = "STÜNN" und 5 = "STÜNNEN" 

    // ++++++ 4. Wort "GIFFT" ausgeben (immer) +++++++++++
    WriteArray(6, InStundeAddr, newletter, 1);

    // ++++++ 6. Wort "DAT" ausgeben (immer) +++++++++++
    WriteArray(7, InStundeAddr, newletter, 1);

    Serial.println("randomWeatherTableIndex : " + String(randomWeatherTableIndex));
    int weatherCode;
    weatherCode = WeatherTable[randomWeatherTableIndex][0];
    Serial.println("weatherCode: " + String(weatherCode));

    //word1
    if(WeatherTable[randomWeatherTableIndex][1] != 0){
      WriteArray(WeatherTable[randomWeatherTableIndex][1] - 1, word1Addr, newletter, 1);
    }

    //word2
    if(WeatherTable[randomWeatherTableIndex][2] != 0){
      WriteArray(WeatherTable[randomWeatherTableIndex][2] - 1, word2Addr, newletter, 1);
    }

    //word3
    if(WeatherTable[randomWeatherTableIndex][3] != 0){
      WriteArray(WeatherTable[randomWeatherTableIndex][3] - 1, word3Addr, newletter, 1);
    }

    //word4
    if(WeatherTable[randomWeatherTableIndex][4] != 0){
      WriteArray(WeatherTable[randomWeatherTableIndex][4] - 1, word4Addr, newletter, 1);
    }

    //word5
    if(WeatherTable[randomWeatherTableIndex][5] != 0){
      WriteArray(WeatherTable[randomWeatherTableIndex][5] - 1, word5Addr, newletter, 1);
    }

    //word6
    if(WeatherTable[randomWeatherTableIndex][6] != 0){
      WriteArray(WeatherTable[randomWeatherTableIndex][6] - 1, word6Addr, newletter, 1);
    }

    //word7
    if(WeatherTable[randomWeatherTableIndex][7] != 0){
      WriteArray(WeatherTable[randomWeatherTableIndex][7] - 1, word7Addr, newletter, 1);
    }

    //word8
    if(WeatherTable[randomWeatherTableIndex][8] != 0){
      WriteArray(WeatherTable[randomWeatherTableIndex][8] - 1, word8Addr, newletter, 1);
    }

    //word9
    if(WeatherTable[randomWeatherTableIndex][9] != 0){
      WriteArray(WeatherTable[randomWeatherTableIndex][9] - 1, word9Addr, newletter, 1);
    }

    // NEWLETTER - EINBLENDEN - Start:
    for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {

      if(newletter[LEDindicator] == 0){
        LEDstripArray[LEDindicator].setRGB( 0, 0, 0);
      }

      // +++++ the DAY colors are defined by the Setup and EEPROM storage
      if (weatherCode < 200) {
        RGB_RED = savedRGBRED;
        RGB_GREEN = savedRGBGREEN;
        RGB_BLUE = savedRGBBLUE;

      // ++++ the Night-Color should be shifted to a "BLUE-Night" value  
      } else {
        RGB_RED = 0;
        RGB_GREEN = 100;
        RGB_BLUE = 250;
      }
      
      if(newletter[LEDindicator] == 1) {
        LEDstripArray[LEDindicator].setRGB( RGB_RED, RGB_GREEN, RGB_BLUE);
      } 

    }

    for (int k = 0; k <= brightnessLED; k++) {
      FastLED.setBrightness(k);
      FastLED.delay(5);
      FastLED.show();

    }  // NEWLETTER einblenden - ENDE

    // ++++++++ Neuer Countdown ENDE +++++++++++++++++++++++

    // DEMO - Mode : delay(3000);
    delay(500);

    // ALLE LED - AUSBLENDEN (während CountDown):
    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
      LEDstripArray[LEDindicator].setRGB( 0, 0, 0);
    }

    for (int k = brightnessLED; k >= 0; k--) {
      FastLED.setBrightness(k);
      FastLED.delay(5);
      FastLED.show();
    }

    LEDindicator = 0;

  }
}

// +++++++++ LED WETTERCODE - COUNTDOWN - END ++++++++++++++++

// +++++++START +++++ Check for NEW-Location +++++++++++++++++++
void checkNewLocation(String savedLOCATION_PLZ, String savedLOCATION_NAME){

  if (savedLOCATION_PLZ != previousLOCATION_PLZ || savedLOCATION_NAME != previousLOCATION_NAME) {

    String converted_LOCATION_NAME= savedLOCATION_NAME;

    converted_LOCATION_NAME.trim();

    // unsigned int locationNamelength = convertString.length();

    // Array of Characters being substitued for die OPEN-METEO API
    String substituteChar[8][2] = { {"ß", "%C3%9F"}, 
                                    {"Ä", "%C3%84"}, 
                                    {"ä", "%C3%A4"}, 
                                    {"Ö", "%C3%96"}, 
                                    {"ö", "%C3%B6"}, 
                                    {"Ü", "%C3%9C"}, 
                                    {"ü", "%C3%BC"}, 
                                    {" ", "+"} };
    

    Serial.println("converted_LOCATION_NAME (before Conversion)= : " + String(converted_LOCATION_NAME));

    // Loop CONVERSION by using REPLACE() 
    // - for all (0-7) possible substitions (refering to the array above)
    for (int i=0; i < 8; i++) {
      converted_LOCATION_NAME.replace(substituteChar[i][0] , substituteChar[i][1]);
      // Serial.println("CONVERT-STRING (during conversion)= : " + String(convertString));
    }

    Serial.println("converted_LOCATION_NAME (after Conversion)= : " + String(converted_LOCATION_NAME));

    // +++++++++ END new CONVERSION +++++++++

    // TEST RUN
    //METEOLocationName = converted_LOCATION_NAME;

    // ++++++++++ ENDE ++++ Location Name conversion function ++++++++++++        

    // Start ++++++++++Ermittlung LATITUDE & LONGITUDE aus der LOCATION aus dem EEPROM / WEB UI
    String MeteoAPILocation = "";

    // Bisher für OPEN-METEO
    // MeteoAPILocation = SetupLocationApi(METEOLocationName);
    // NEU (2026-04-08) für OPENSTREETMAP
    MeteoAPILocation = SetupLocationApi(savedLOCATION_PLZ, converted_LOCATION_NAME);
    Serial.println("--------------------------------");
    Serial.println("API Location call to OPEN-METEO.COM : ");
    Serial.println(MeteoAPILocation);
    Serial.println("--------------------------------");

    json_array_METEO = "";

    json_array_METEO = GET_Request(MeteoAPILocation.c_str());
    Serial.println("--------------------------------");
    Serial.println("JSON result from OPEN-METEO.COM");
    Serial.println(json_array_METEO);
    Serial.println("--------------------------------");

    delay(250);

    noLocationFound = false;

    JsonCONVlocation();
    Serial.println("--------------------------------");
    Serial.print("Breitengrad : "); Serial.print(Latitude, 4); Serial.print(" und Laengengrad : ");
    Serial.print(Longitude, 4); Serial.print(" für die Lokation (PLZ - Name): "); Serial.println(savedLOCATION_PLZ + " - " + converted_LOCATION_NAME);

    if (Latitude == 0 && Longitude == 0){
      noLocationFound = true;
    }
    
    /*
    //write LATITUDE / LONGITUDE to EEPROM (as String)
    // convert double (Float) to string using the library stdlib.h
    char stringbufferLAT[10];
    String myLatitude = "";
    dtostrf(Latitude, 3, 4, stringbufferLAT);
    myLatitude = stringbufferLAT;
    writeStringToEEPROM (adrOffsetLAT, myLatitude);
    EEPROM.commit();
    retrievedLATITUDE = readStringFromEEPROM(adrOffsetLAT);
    Serial.print("The LATITUDE - read from EEPROM:"); Serial.println(retrievedLATITUDE);

    char stringbufferLONG[10];
    String myLongitude = "";
    dtostrf(Longitude, 3, 4, stringbufferLONG);
    myLongitude = stringbufferLONG;
    writeStringToEEPROM (adrOffsetLONG, myLongitude);
    EEPROM.commit();
    retrievedLONGITUDE = readStringFromEEPROM(adrOffsetLONG);
    Serial.print("The LONGITUDE - read from EEPROM:"); Serial.println(retrievedLONGITUDE);
    */

    // store the current LOCATION into the PREVIOUS LOCATION for the above comparison
    // the whole API request isn't needed as long as the location didn't change.
    previousLOCATION_PLZ = savedLOCATION_PLZ;
    previousLOCATION_NAME = savedLOCATION_NAME;

  }
}
// +++++++ END +++++ Check for NEW-Location +++++++++++++++++++

// ++++++++++ RUN GET WEATHER CODE - START +++++++++++++
void getWeatherCodePlatt() {

  String MeteoAPIToday = "";
  String MeteoAPITomorrow = "";

  // Wetterdaten für HEUTE:
  isToday = true;
  MeteoAPIToday = SetupMeteoApi(isToday);
  Serial.println("--------------------------------");
  Serial.println("API call to OPEN-METEO.COM : ");
  Serial.println(MeteoAPIToday);
  Serial.println("--------------------------------");

  json_array_METEO = "";          

  json_array_METEO = GET_Request(MeteoAPIToday.c_str());
  Serial.println("--------------------------------");
  Serial.println("JSON result from OPEN-METEO.COM");
  Serial.println(json_array_METEO);
  Serial.println("--------------------------------");

  delay(250);

  JsonCONVWeatherCodes(isToday);
  Serial.println("--------------------------------");
  Serial.println("TODAY: Hourly WeatherCodes extracted from the JSON result");

  for (int i=0; i<=23; i++){
    Serial.print("WetterCode um "); Serial.print(i); Serial.print(".00 Uhr : ");Serial.println(todayWeather[i]);  
  }
  Serial.println("--------------------------------");

  // Get SUNRISE and SUNSET (of TODAY Only!)
  JsonCONVSunRise(isToday);
  Serial.println("--------SUNRISE START ------------------------");
  Serial.println("SUNRISE IS at Hour : " + String(todaySunriseHour));
  Serial.println("SUNRISE IS at Minute :" + String(todaySunriseMinute));  
  Serial.println("--------SUNRISE END.  ------------------------");

  JsonCONVSunSet(isToday);
  Serial.println("--------SUNSET START ------------------------");
  Serial.println("SUNSET IS at Hour : " + String(todaySunsetHour));
  Serial.println("SUNSET IS at Minute :" + String(todaySunsetMinute));  
  Serial.println("--------SUNSET END.  ------------------------");

  // Wetterdaten für Morgen (== isToday = false):
  isToday = false;         

  MeteoAPITomorrow = SetupMeteoApi(isToday);
  Serial.println("--------------------------------");
  Serial.println("API call to OPEN-METEO.COM : ");
  Serial.println(MeteoAPITomorrow);
  Serial.println("--------------------------------");

  json_array_METEO = GET_Request(MeteoAPITomorrow.c_str());
  Serial.println("--------------------------------");
  Serial.println("JSON result from OPEN-METEO.COM");
  Serial.println(json_array_METEO);
  Serial.println("--------------------------------");

  JsonCONVWeatherCodes(isToday);
  Serial.println("--------------------------------");
  Serial.println("TOMORROW: Hourly WeatherCodes extracted from the JSON result");

  for (int i=0; i<=23; i++){
    Serial.print("WetterCode um "); Serial.print(i); Serial.print(".00 Uhr : ");Serial.println(tomorrowWeather[i]);  
  }
  Serial.println("--------------------------------");

}         

// / ++++++++++ RUN GET WEATHER CODE - ENDE +++++++++++++

// ++++++++++ RUN DISPLAY WEATHER CODE - START +++++++++++++
void displayWeatherCodePlatt(bool savedNIGHTJN, int  savedNIGHTSTART, int savedNIGHTEND, int WetterInStund){

  LEDindicator = 0;  
  
  // Serial.print("runWedderPlatt för STÜND nr. = "); Serial.println(WetterInStund);

  // In jedem LOOP das NEWLETTER Array wieder zurücksetzen und damit für das WRITE-Array initialisieren
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    newletter[LEDindicator] = 0;   
  }

  // +++++++START +++ Berechne LED Anzeige fuer WetterCode bei wechselnder Anzeige "IN EEN/TWEE/DREE STÜND" +++++++

  // init Weather variables (in every loop)
  WetterCodeStunde1 = 0;    // OPEN METEO Stunde ist 0h - 23h  (in Een Stünd Display)
  WetterCodeStunde2 = 0;    // OPEN METEO Stunde ist 0h - 23h  (in Twee Stünd Display)
  WetterCodeStunde3 = 0;    // OPEN METEO Stunde ist 0h - 23h  (in Dree Stünd Display)

  WetterCode1 = 10;
  WetterCode2 = 10;
  WetterCode3 = 10;

  if (aktuelleStunde24h > 20) {

    switch (aktuelleStunde24h){
      case 21:
        WetterCodeStunde1 = aktuelleStunde24h + 1;
        WetterCode1 = todayWeather[WetterCodeStunde1];

        WetterCodeStunde2 = aktuelleStunde24h + 2;
        WetterCode2 = todayWeather[WetterCodeStunde2];

        WetterCodeStunde3 = 0;
        WetterCode3 = tomorrowWeather[WetterCodeStunde3];
      break;

      case 22:
        WetterCodeStunde1 = aktuelleStunde24h + 1;
        WetterCode1 = todayWeather[WetterCodeStunde1];

        WetterCodeStunde2 = 0;
        WetterCode2 = tomorrowWeather[WetterCodeStunde2];

        WetterCodeStunde3 = 1;
        WetterCode3 = tomorrowWeather[WetterCodeStunde3];
      break;

      case 23:
        WetterCodeStunde1 = 0;
        WetterCode1 = tomorrowWeather[WetterCodeStunde1];

        WetterCodeStunde2 = 1;             
        WetterCode2 = tomorrowWeather[WetterCodeStunde2];

        WetterCodeStunde3 = 2;          
        WetterCode3 = tomorrowWeather[WetterCodeStunde3];
      break;

      }

  } else {
    WetterCodeStunde1 = aktuelleStunde24h + 1;
    WetterCode1 = todayWeather[WetterCodeStunde1];            

    WetterCodeStunde2 = aktuelleStunde24h + 2;
    WetterCode2 = todayWeather[WetterCodeStunde2];

    WetterCodeStunde3 = aktuelleStunde24h + 3;
    WetterCode3 = todayWeather[WetterCodeStunde3];
  }

  // Init / zurücksetzen Werte (wobei 10 ein ungenutzer WetterCode ist)
  WetterCodeDisplayedOld = 10;
  WetterCodeDisplayed = 10;
  weatherCodeLED = 10;

  // Serial.println("WetterInStund von aktuelle Minute : " + String(aktuelleMinute) + " hat hier den Wert : " + String(WetterInStund));

  if(WetterInStund != 0) {

    //Adressbereiche im Array initialisieren
    ResetArray(NumberInStunde, InStundeAddr, newletter);

    // ++++++ 1. Wort "IN" ausgeben (immer) ++++++++++
    WriteArray(0, InStundeAddr, newletter, 1);

    // ++++++ 2. Wort "EEN", "TWEE" oder "DREE" ausgeben (Uhrzeit gesteuert) +++++++++++
    // mit einem Anzeigewechsel 3x pro Stunde

    WriteArray(WetterInStund, InStundeAddr, newletter, 1);                //EEN , TWEE oder DREE STÜND'

    // ++++++ 3. Wort "STÜNN'" oder "STÜNNEN" ausgeben (immer) ++++++++++
    if (WetterInStund == 1){
      WriteArray(4, InStundeAddr, newletter, 1);
    } else {
      WriteArray(5, InStundeAddr, newletter, 1);
    }; // 4 = "STÜNN" und 5 = "STÜNNEN"

    // ++++++ 4. Wort "GIFFT" ausgeben (immer) +++++++++++
    WriteArray(6, InStundeAddr, newletter, 1);

    // ++++++ 6. Wort "DAT" ausgeben (immer) +++++++++++
    WriteArray(7, InStundeAddr, newletter, 1);

    // ++++++ 7. "Wetter via OPEN METEO Codes" ausgeben +++++++++++

    Serial.print("DISPLAY WetterInStund (vor dem Switch) = "); Serial.println(WetterInStund);

    switch (WetterInStund){

      case 1:
        WetterCodeDisplayed = WetterCode1;
        Serial.print("Die Ausgabe-Stunde (in EEN STÜNN) ist : " + String(WetterCodeStunde1) + " Uhr");Serial.println(" mit dem Wetter-Code : " + String(WetterCode1));                
      break;

      case 2:
        WetterCodeDisplayed = WetterCode2;
        Serial.print("Die Ausgabe-Stunde (in TWEE STÜNNEN)  ist : " + String(WetterCodeStunde2) + " Uhr");Serial.println(" mit dem Wetter-Code : " + String(WetterCode2));
      break;

      case 3:
        WetterCodeDisplayed = WetterCode3;
        Serial.print("Die Ausgabe-Stunde (in DREE STÜNNEN)  ist : " + String(WetterCodeStunde3) + " Uhr");Serial.println(" mit dem Wetter-Code : " + String(WetterCode3));
      break;    
              
    }
  }

  // ++++ NEW +++ Distinction for WetterCode < 3 (SÜNN versus NACHT) between DAY and NIGHT (refering to SUNRISE and SUNSET times )
  Serial.println("WetterCodeDisplayed (VORHER): " + String(WetterCodeDisplayed));
  
  Serial.println("todaySunriseHour : " + String(todaySunriseHour));
  Serial.println("todaySunsetHour : " + String(todaySunsetHour));
  Serial.println("aktuelleStunde24h : " + String(aktuelleStunde24h));

  // Wenn der WetterCode kleiner als "3 = Wulken" ist, dann muss zwischen Tag und Nacht Anzeige unterschieden werden
  if (WetterCodeDisplayed < 3) {

    // WETTERCODESTUNDE 1
    if (WetterInStund == 1) {

      // Wenn die aktuelle Stunde zwischen 16:00 Uhr und 23:59 Uhr ist UND die aktuelle Stunde nach Sonnenuntergang, dann zeige NACHT WetterCodes
      if (aktuelleStunde24h > 15 && todaySunsetHour <= WetterCodeStunde1) {
        WetterCodeDisplayed = WetterCodeDisplayed + 200; // Addition von weiteren 100 == 200 für Wettercode Werte 200, 201 und 202 (für NACHT) --> NACH SUNSET
  
      // Wenn die aktuelle Stunde zwischen 0:00 Uhr und 09:59 Uhr liegt UND die aktuelle Stunde vor Sonnenaufgang, dann zeige NACHT WetterCodes
      } else if (aktuelleStunde24h < 10 && todaySunriseHour >= WetterCodeStunde1) {
        WetterCodeDisplayed = WetterCodeDisplayed + 200; // Addition von weiteren 100 == 200 für Wettercode Werte 200, 201 und 202 (für NACHT) --> VOR SUNRISE
      
      } else {

        WetterCodeDisplayed = WetterCodeDisplayed + 100; // Addition 100 für WetterCode-Werte (TAG)     
      
      }
    
    } else if (WetterInStund == 2) {

      // Wenn die aktuelle Stunde zwischen 16:00 Uhr und 23:59 Uhr ist UND die aktuelle Stunde nach Sonnenuntergang, dann zeige NACHT WetterCodes
      if (aktuelleStunde24h > 15 && todaySunsetHour <= WetterCodeStunde2) {
        WetterCodeDisplayed = WetterCodeDisplayed + 200; // Addition von weiteren 100 == 200 für Wettercode Werte 200, 201 und 202 (für NACHT) --> NACH SUNSET
  
      // Wenn die aktuelle Stunde zwischen 0:00 Uhr und 09:59 Uhr liegt UND die aktuelle Stunde vor Sonnenaufgang, dann zeige NACHT WetterCodes
      } else if (aktuelleStunde24h < 10 && todaySunriseHour >= WetterCodeStunde2) {
        WetterCodeDisplayed = WetterCodeDisplayed + 200; // Addition von weiteren 100 == 200 für Wettercode Werte 200, 201 und 202 (für NACHT) --> VOR SUNRISE
      
      } else {

        WetterCodeDisplayed = WetterCodeDisplayed + 100; // Addition 100 für WetterCode-Werte (TAG)     
      
      }

    } else if (WetterInStund == 3) {

      // Wenn die aktuelle Stunde zwischen 16:00 Uhr und 23:59 Uhr ist UND die aktuelle Stunde nach Sonnenuntergang, dann zeige NACHT WetterCodes
      if (aktuelleStunde24h > 15 && todaySunsetHour <= WetterCodeStunde3) {
        WetterCodeDisplayed = WetterCodeDisplayed + 200; // Addition von weiteren 100 == 200 für Wettercode Werte 200, 201 und 202 (für NACHT) --> NACH SUNSET
  
      // Wenn die aktuelle Stunde zwischen 0:00 Uhr und 09:59 Uhr liegt UND die aktuelle Stunde vor Sonnenaufgang, dann zeige NACHT WetterCodes
      } else if (aktuelleStunde24h < 10 && todaySunriseHour >= WetterCodeStunde3) {
        WetterCodeDisplayed = WetterCodeDisplayed + 200; // Addition von weiteren 100 == 200 für Wettercode Werte 200, 201 und 202 (für NACHT) --> VOR SUNRISE
      
      } else {

        WetterCodeDisplayed = WetterCodeDisplayed + 100; // Addition 100 für WetterCode-Werte (TAG)     
      
      }

    } 
  
  } else {

    WetterCodeDisplayed = WetterCodeDisplayed + 100; // Addition 100 für WetterCode-Werte (TAG)
  
  }

  Serial.println("WetterCodeDisplayed (NACHHER): " + String(WetterCodeDisplayed));
  
  // ++++ Sicherung des WetterCodes für die LED Anzeige weiter unten (im Break wird der Wert auf NUll gesetzt!)
  weatherCodeLED = WetterCodeDisplayed;

  // ++++ NEW +++ Distinction +++++ ENDE +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  if(WetterCodeDisplayed != WetterCodeDisplayedOld) {

    //Minutenzahl für den nächsten Vergleich speichern
    WetterCodeDisplayedOld = WetterCodeDisplayed;

    //Adressbereiche im Array initialisieren
    ResetArray(NumberOfWord1, word1Addr, newletter);
    ResetArray(NumberOfWord2, word2Addr, newletter);
    ResetArray(NumberOfWord3, word3Addr, newletter);
    ResetArray(NumberOfWord4, word4Addr, newletter);
    ResetArray(NumberOfWord5, word5Addr, newletter);
    ResetArray(NumberOfWord6, word6Addr, newletter);
    ResetArray(NumberOfWord7, word7Addr, newletter);
    ResetArray(NumberOfWord8, word8Addr, newletter);
    ResetArray(NumberOfWord9, word9Addr, newletter);  

    //WeatherTable[32][9]
    //Wert '0' bedeutet, dass das Array übersprungen wird und die Adresse nicht angezeigt wird
    //Anzeige der Wetterdaten
    for(LEDindicator = 0; LEDindicator <= 35; LEDindicator++){

      //Aktuellen WetterCode in der WeatherTable suchen
      if(WetterCodeDisplayed == WeatherTable[LEDindicator][0]){
        
        //word1
        if(WeatherTable[LEDindicator][1] != 0){
          WetterCodeDisplayed = WeatherTable[LEDindicator][1] - 1;
          WriteArray(WetterCodeDisplayed, word1Addr, newletter, 1);
        }

        //word2
        if(WeatherTable[LEDindicator][2] != 0){
          WetterCodeDisplayed = WeatherTable[LEDindicator][2] - 1;
          WriteArray(WetterCodeDisplayed, word2Addr, newletter, 1);
        }

        //word3
        if(WeatherTable[LEDindicator][3] != 0){
          WetterCodeDisplayed = WeatherTable[LEDindicator][3] - 1;
          WriteArray(WetterCodeDisplayed, word3Addr, newletter, 1);
        }

        //word4
        if(WeatherTable[LEDindicator][4] != 0){
          WetterCodeDisplayed = WeatherTable[LEDindicator][4] - 1;
          WriteArray(WetterCodeDisplayed, word4Addr, newletter, 1);
        }

        //word5
        if(WeatherTable[LEDindicator][5] != 0){
          WetterCodeDisplayed = WeatherTable[LEDindicator][5] - 1;
          WriteArray(WetterCodeDisplayed, word5Addr, newletter, 1);
        }

        //word6
        if(WeatherTable[LEDindicator][6] != 0){
          WetterCodeDisplayed = WeatherTable[LEDindicator][6] - 1;
          WriteArray(WetterCodeDisplayed, word6Addr, newletter, 1);
        }

        //word7
        if(WeatherTable[LEDindicator][7] != 0){
          WetterCodeDisplayed = WeatherTable[LEDindicator][7] - 1;
          WriteArray(WetterCodeDisplayed, word7Addr, newletter, 1);
        }

        //word8
        if(WeatherTable[LEDindicator][8] != 0){
          WetterCodeDisplayed = WeatherTable[LEDindicator][8] - 1;
          WriteArray(WetterCodeDisplayed, word8Addr, newletter, 1);
        }

        //word9
        if(WeatherTable[LEDindicator][9] != 0){
          WetterCodeDisplayed = WeatherTable[LEDindicator][9] - 1;
          WriteArray(WetterCodeDisplayed, word9Addr, newletter, 1);
        }

        break;

      }  
    }

  }

  // Vergleich, ob sich die Anzeige ändern muss, weil sich mindestens ein Zeichen geändert hat.
  letterchanged = false;

  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {       

    if (shownletter[LEDindicator] != newletter[LEDindicator]) {
      letterchanged = true;
    }

  }

  // Check if Night-Switch-Off is active...
  nightSwitchOffCheck();

  
  Serial.println ("aktuelleStunde24h = " + String(aktuelleStunde24h));
  Serial.println ("savedNIGHTJN = " + String(savedNIGHTJN));
  Serial.println ("savedNIGHTSTART = " + String(savedNIGHTSTART));
  Serial.println ("savedNIGHTEND = " + String(savedNIGHTEND));
  Serial.println ("nightSwitchOff = " + String(nightSwitchOff));


  if(savedNIGHTJN == false || (savedNIGHTJN == true && nightSwitchOff == false) ) {

    // Anzeige (FADE OUT & FADE IN) nur wenn sich mindestens ein Zeichen geändert hat.
    if (letterchanged) {
      
      // SHOWNLETTER - AUSBLENDEN - Start:
      for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {

        if(shownletter[LEDindicator] == 0){
          LEDstripArray[LEDindicator].setRGB( 0, 0, 0);
        }

        if(shownletter[LEDindicator] == 1) {
          LEDstripArray[LEDindicator].setRGB( RGB_RED, RGB_GREEN, RGB_BLUE);
        } 

      }

      for (int k = brightnessLED; k >= 0; k--) {
        FastLED.setBrightness(k);
        FastLED.delay(15);
        FastLED.show();

      } // SHOWNLETTER ausblenden - ENDE

      // NEWLETTER - EINBLENDEN - Start:
      for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {

        if(newletter[LEDindicator] == 0){
          LEDstripArray[LEDindicator].setRGB( 0, 0, 0);
        }

        // +++++ the DAY colors are defined by the Setup and EEPROM storage
        if (weatherCodeLED < 200) {
          RGB_RED = savedRGBRED;
          RGB_GREEN = savedRGBGREEN;
          RGB_BLUE = savedRGBBLUE;

        // ++++ the Night-Color should be shifted to a "BLUE-Night" value  
        } else {
          RGB_RED = 0;
          RGB_GREEN = 100;
          RGB_BLUE = 250;
        }
        
        if(newletter[LEDindicator] == 1) {
          LEDstripArray[LEDindicator].setRGB( RGB_RED, RGB_GREEN, RGB_BLUE);
        } 

      }

      for (int k = 0; k <= brightnessLED; k++) {
        FastLED.setBrightness(k);
        FastLED.delay(15);
        FastLED.show();

      }  // NEWLLETTER einblenden - ENDE

    }

  } else { // Schalte alle LED im STRIP auf den RGB Wert = 0,0,0 

    for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {

      LEDstripArray[LEDindicator].setRGB( 0, 0, 0);

    }

    FastLED.show();

  } // Ende Abfrage NIGHTMODE

  LEDindicator = 0;

  //Arrays kopieren für den nächsten Vergleich ... 
  // ... wichtig für das FADE OUT / FADE IN --> NUR(!) bei Änderung mindestens eines Zeichens
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    shownletter[LEDindicator] = newletter[LEDindicator];
  } 

}

#endif

// ++++++++++ RUN LED CLOCK - END +++++++++++++++++++++++++++++++++++++++++++++



