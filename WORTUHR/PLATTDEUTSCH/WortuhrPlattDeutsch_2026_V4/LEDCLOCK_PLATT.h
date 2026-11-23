#ifndef LEDCLOCK_PLATT_H
#define LEDCLOCK_PLATT_H

#include <Arduino.h>

#define FASTLED_INTERNAL
#define FASTLED_ALLOW_INTERRUPTS 0

#include <FastLED.h>        // Used library FastLED (by Daniel Garcia / Version 3.10.3) - used for programming the LED stripes

#define DATA_PIN            21 // (for ESP32 only - GPIO 21, 25 or GPIO 27)
#define NUMPIXELS           121
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

// ++++ for NTP Server Connection and readtime method
#include "time.h"
#include "sntp.h"

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

// Dieses ist nur für ESP32 !!!
// SET_LOOP_TASK_STACK_SIZE(16 * 1024);  // Test mit 32kB da es bei 16KB eine "späte" Fehlfunktion gibt (bei 8k Loop Stack Size kann man nur 75 LED addressieren ?!?! / Dann gibt es eine harten Prozessor Fehler!)

// +++++++++++++++ Ende LED Strip +++++++++++++++++++++++++++

// +++++++++++++ START WORDCLOCK Variables section ++++++++++++

unsigned int aktuelleStunde12h = 0;
unsigned int aktuelleStunde24h = 0;

unsigned int alteStunde12h = 0;
unsigned int alteStunde24h = 0;

unsigned int aktuelleMinute = 0;
unsigned int alteMinute = 0;

unsigned int summerTimeActive = 0;                // summerTimeActive = 0 == "Winterzeit"

// +++++++++++ WORDCLOCK Array for displaying time in "PLATT-Deutsch" +++++++++++

//Adresse der ersten und letzten LED
const int ersteLED = 0;
const int letzteLED = NUMPIXELS - 1;

unsigned int LEDindicator = 0;

int   savedRGBRED = 0;
int   savedRGBGREEN = 0;
int   savedRGBBLUE = 0;

bool  savedNIGHTJN = false;
int   savedNIGHTSTART = 0;
int   savedNIGHTEND = 0;

bool nightSwitchOff = false;

unsigned int pointer = 0;

unsigned int newletter[NUMPIXELS];
unsigned int shownletter[NUMPIXELS];

//Größe der Adressbereiche für eine 11 x 11 LED Matrix (--> ersteLED = 0 - letzteLED = 120)
const int NumberOfHours = 12;
const int NumberOfMinute = 4;
const int NumberOfSpecialChar = 2;

//Adressen für die WORDCLOCK
// Adressen Es/Ist
const int EsIstAddr[2][2] = { {0, 2},             //DAT
                              {4, 5} };           //IS                   

//Adressen der AL / EERST Anzeige
const int AlreadyAddr[1][2] = { {7, 8} };      //AL

//Adressen Vor/Nach
const int ToPastAddr[2][2] = { {33, 34},          //NA
                               {35, 37} };        //VÖR

//Adressen Halb/Voll
const int HalfFullHourAddr[2][2] = { {44, 47},    //HALF
                                     {49, 53} };  //KLOCK

//Adressen der Stunden
const int HourAddr[12][2] = { {88, 90},           //EEN
                              {105, 108},         //TWEE
                              {61, 64},           //DREE
                              {99, 102},          //VEER
                              {66, 69},           //FIEF
                              {110, 113},         //SÖSS
                              {71, 75},           //SÖVEN
                              {77, 80},           //ACHT
                              {93, 97},           //NEGEN
                              {55, 59},           //TEIHN
                              {115, 119},         //ÖLVEN
                              {82, 86} };         //TWÖLF       

// Adressen der Minuten
const int MinuteAddr[4][2] = {  {11, 14},         //FIEF
                                {39, 43},         //TEIHN
                                {26, 32},         //VEERTEL
                                {15, 21} };       //TWINTIG

// Zeittabelle mit Takt +2/-2 Minuten
// ++++++++++++++++++++++++++++++++++
// Array Element0 = 0 - 59ste Minute
// Array Element1 = Element aus Array "MinuteAddr"        (example = 1="FIEF", 2="TEIHN", 3=VEERTEL, 4="TWINTIG")
// Array Element2 = Element aus Array "ToPastAddr"        (example = 1 = "NA", 2 = "VÖR")
// Array Element3 = Element aus Array "HalfFullHourAddr"  (example = 1 = "HALF", 2 = "KLOCK")
// ++++++++++++++++++++++++++++++++++
// ++++ (Alle Elemente sind m.E. um "1" verschoben, da der erste Eintrag = "0" == "keine Anzeige" bedeutet) +++++

const int TimeTable[60][4] = { {0,0,0,2},{1,0,0,2},{2,0,0,2},{3,1,1,0},{4,1,1,0},{5,1,1,0},{6,1,1,0},{7,1,1,0},{8,2,1,0},{9,2,1,0},
                              {10,2,1,0},{11,2,1,0},{12,2,1,0},{13,3,1,0},{14,3,1,0},{15,3,1,0},{16,3,1,0},{17,3,1,0},{18,4,1,0},{19,4,1,0},
                              {20,4,1,0},{21,4,1,0},{22,4,1,0},{23,1,2,1},{24,1,2,1},{25,1,2,1},{26,1,2,1},{27,1,2,1},{28,0,0,1},{29,0,0,1},
                              {30,0,0,1},{31,0,0,1},{32,0,0,1},{33,1,1,1},{34,1,1,1},{35,1,1,1},{36,1,1,1},{37,1,1,1},{38,4,2,0},{39,4,2,0},
                              {40,4,2,0},{41,4,2,0},{42,4,2,0},{43,3,2,0},{44,3,2,0},{45,3,2,0},{46,3,2,0},{47,3,2,0},{48,2,2,0},{49,2,2,0},
                              {50,2,2,0},{51,2,2,0},{52,2,2,0},{53,1,2,0},{54,1,2,0},{55,1,2,0},{56,1,2,0},{57,1,2,0},{58,0,0,2},{59,0,0,2} };

// +++++ Adressen für WORDCLOCK - Ende +++++++++++++++++++++++++

// +++++++++++++ NTP Server for getting DATE & TIME ++++++++++++

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
//const long  gmtOffset_sec = 3600;
// int   daylightOffset_sec = 3600;

int timeInfoSuccess = 0;
const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

void readLocalTime() {
  struct tm timeinfo;

  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  } else {
    Serial.println("NTP Time available (yet)");
    timeInfoSuccess = 1;
  }

  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
 
  aktuelleStunde24h = timeinfo.tm_hour;

  Serial.println ("Die aktuelle Stunde (24h) ist : " + String(aktuelleStunde24h));

  // für 00 Uhr - 12 Uhr --> wird 00 Uhr  - 12 Uhr (keine Umsetzung)
  // aus 13 Uhr - 23 Uhr --> wird 01 Uhr  - 11 Uhr 

  if (aktuelleStunde24h > 12){
    aktuelleStunde12h = aktuelleStunde24h - 12;
  }else{
    aktuelleStunde12h = aktuelleStunde24h;
  }
   Serial.println ("Die aktuelle Stunde (12h) ist : " + String(aktuelleStunde12h));
  
  aktuelleMinute = timeinfo.tm_min;
  Serial.println ("Die aktuelle Minute ist : " + String(aktuelleMinute));
  summerTimeActive = timeinfo.tm_isdst;
  Serial.println ("Sommerzeit aktiv (J/N) : " + String(summerTimeActive));

}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t) {
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

// ++++++++ Write LED ARRAY  ++++ START +++++++++++

void WriteArray(int Pointer, const int Addr[][2], unsigned int *Array, int Value){

  int c1 = 0;

  //Array-Bereich beschreiben
  for(c1 = Addr[Pointer][0]; c1 <= Addr[Pointer][1]; c1++) {
    Array[c1] = Value;
  }
}

// ++++++++ Write LED ARRAY  ++++ END +++++++++++++

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

// +++++++++ LED COUNTDOWN - START ++++++++++++++++
void runCountdownPlatt() {

  savedRGBRED = EEPROM.read(ADDR_LED_RED);
  savedRGBGREEN = EEPROM.read(ADDR_LED_GREEN);
  savedRGBBLUE = EEPROM.read(ADDR_LED_BLUE);

  for (int countdownCounter = 11; countdownCounter >= 0; countdownCounter--) {
    // alle LED zunächst einmal ausschalten
    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
      LEDstripArray[LEDindicator] = CRGB::Black;
    }
    FastLED.show();

    //CountDowm LEDs  einschalten (--> DEMO-Lauf)

    // DAT, IS, AL
    for (LEDindicator = EsIstAddr[0][0]; LEDindicator <= EsIstAddr[0][1]; LEDindicator++){ 
      LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
    }

    for (LEDindicator = EsIstAddr[1][0]; LEDindicator <= EsIstAddr[1][1]; LEDindicator++){ 
      LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
    }

    for (LEDindicator = AlreadyAddr[0][0]; LEDindicator <= AlreadyAddr[0][1]; LEDindicator++){ 
      LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
    }

    // MINUTEADDRESS
    long minuteAddressColumn = 99;
    minuteAddressColumn = random(0,4);

    for (LEDindicator = MinuteAddr[minuteAddressColumn][0]; LEDindicator <= MinuteAddr[minuteAddressColumn][1]; LEDindicator++){ 
      LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
    }

    // VÖR NACH
    if (countdownCounter%2 == 0) {
      for (LEDindicator = ToPastAddr[1][0]; LEDindicator <= ToPastAddr[1][1]; LEDindicator++){ 
        LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
      }
    } else {
      for (LEDindicator = ToPastAddr[0][0]; LEDindicator <= ToPastAddr[0][1]; LEDindicator++){ 
        LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
      }
    }

    // HOURADDRESS
    for (LEDindicator = HourAddr[countdownCounter][0]; LEDindicator <= HourAddr[countdownCounter][1]; LEDindicator++){ 
      LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
    }

    FastLED.show();
    delay(1000);

    // alle LED ausschalten
    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
      LEDstripArray[LEDindicator] = CRGB::Black;
    }

    FastLED.show();
    Serial.println("Countdown-Step : " + String(countdownCounter));

    delay(200);

  }
}


// +++++++++ LED COUNTDOWN - END +++++++++++++++++++++++++++++++

// +++++++++++ START WAIT for WIFI Connect +++++++++++++++++++++
void waitForWLANconnect(){

  LEDstripArray[38].setRGB(100,0,0);
  LEDstripArray[49].setRGB(100,0,0);
  LEDstripArray[60].setRGB(100,0,0);
  LEDstripArray[71].setRGB(100,0,0);
  LEDstripArray[82].setRGB(100,0,0); 

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

  LEDstripArray[38].setRGB(0,100,0);
  LEDstripArray[49].setRGB(0,100,0);
  LEDstripArray[60].setRGB(0,100,0);
  LEDstripArray[71].setRGB(0,100,0);
  LEDstripArray[82].setRGB(0,100,0); 

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

// ++++++++++ Every second the logo should blink ++++++++++++
void blinkingLogo(int LEDScaleValue){

  RGB_RED = savedRGBRED;
  RGB_GREEN = savedRGBGREEN;
  RGB_BLUE = savedRGBBLUE;
  
  // Check if Night-Switch-Off is active...
  nightSwitchOffCheck();

  if (!nightSwitchOff) {
    LEDstripArray[60].setRGB( RGB_RED, RGB_GREEN, RGB_BLUE);
    LEDstripArray[60].nscale8(LEDScaleValue);
    FastLED.show();
  }

}

// ++++++ END +++++ LED LOGO Blink  +++++++++++++++++++++++++++++++


// ++++++++++ RUN LED CLOCK - START +++++++++++++++++++++++++
void runClockPlatt(){

  // In jedem LOOP das NEWLETTER Array wieder zurücksetzen und damit für das WRITE-Array initialisieren
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    newletter[LEDindicator] = 0;   
  }

  // ++++++ 1. Wort "DAT" ausgeben (immer) ++++++++++
  WriteArray(0, EsIstAddr, newletter, 1);

  // ++++++ 2. Wort "IS" ausgeben (immer) +++++++++++
  WriteArray(1, EsIstAddr, newletter, 1);

  // ++++++ 3. Ausgabe der Stunde (in Abhängigkeit der MEZ "Stunde") +++++++++

  // Diese Werte werden in dem Wert "aktuelleStunde" übergeben.
  // für 00 Uhr - 12 Uhr --> wird 00 Uhr  - 12 Uhr (keine Umsetzung)
  // aus 13 Uhr - 23 Uhr --> wird 01 Uhr  - 11 Uhr 

  // eine Stunde - vor(!) Erreichen der "vollen Stunde" addieren;
  // Beispiel: da 20:30 Uhr 
  // muss in Halb NEUN gewandelt werden muss und nicht in halb ACHT
  // aber 12:58 Uhr muss "Klock Een" werden und nich 13:58 Uhr 


  if (aktuelleMinute >= 23) {
      if (aktuelleStunde12h == 12) {
          aktuelleStunde12h = 1; // Bei 12 Uhr wird auf 1 Uhr gesetzt
      } else {
          aktuelleStunde12h += 1; // Sonst wird die Stunde um 1 erhöht
      }
  } else {
      if (aktuelleStunde12h == 0) {
          aktuelleStunde12h = 12; // Bei 0 Uhr wird auf 12 Uhr gesetzt
      }
      // Alle anderen Stunden bleiben unverändert
  }

  //Anzeige der Stunden (Hier wird 1 von der aktuell ermittelten Stunde abgezogen, da das Array bei 0 (und nicht bei 1) beginnt.)
  WriteArray(aktuelleStunde12h - 1, HourAddr, newletter, 1);

  // +++++++ 4. Ausgabe "AL" (ja/nein) ++++++++++++++++

  //"AL" einschalten           
  if(aktuelleStunde24h > 18 || aktuelleStunde24h < 3) {
    //Anzeige der AL (.... wenn es nach 18 Uhr oder vor 3 Uhr früh ist.)
    WriteArray(0, AlreadyAddr, newletter, 1);

  }

  // ++++++ 5. Ausgabe der Minuten (in Abhängigkeit der MEZ "Minuten") ++++++++++++ 


  //Pointer zurücksetzen
  pointer = 99;

  //TimeTable[60][4]
  //{0,0,0,0} = Minutenzahl, Pointer_MinuteAddr, Pointer_ToPastAddr, Pointer_HalfFullHourAddr
  //Wert '0' bedeutet, dass das Array übersprungen wird und die Adresse nicht angezeigt wird

  //Anzeige der Minuten / der 1/2 Stunde und das VÖR / NA
  for(LEDindicator = 0; LEDindicator <= 59; LEDindicator++){

    //Aktuelle Minutenzahl mit der Zeittabelle vergleichen
    if(aktuelleMinute == TimeTable[LEDindicator][0]){
      
      //MinuteAddr
      if(TimeTable[LEDindicator][1] != 0){
        pointer = TimeTable[LEDindicator][1] - 1;
        WriteArray(pointer, MinuteAddr, newletter, 1);
      }

      //ToPastAddr
      if(TimeTable[LEDindicator][2] != 0){
        pointer = TimeTable[LEDindicator][2] - 1;
        WriteArray(pointer, ToPastAddr, newletter, 1);
      }

      //HalfFullHourAddr
      if(TimeTable[LEDindicator][3] != 0){
        pointer = TimeTable[LEDindicator][3] - 1;
        WriteArray(pointer, HalfFullHourAddr, newletter, 1);
      }
      break;
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

  Serial.println ("Die vorherige Ausgabe-Stunde (12h) ist : " + String(alteStunde12h));
  Serial.println ("Die aktuelle Ausgabe-Stunde (12h) ist : " + String(aktuelleStunde12h));
  Serial.println ("Die aktuelle Ausgabe-Minute (12h) ist : " + String(aktuelleMinute));

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

        RGB_RED = savedRGBRED;
        RGB_GREEN = savedRGBGREEN;
        RGB_BLUE = savedRGBBLUE;
        
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

        RGB_RED = savedRGBRED;
        RGB_GREEN = savedRGBGREEN;
        RGB_BLUE = savedRGBBLUE;
        
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

  //Arrays kopieren für den nächsten Vergleich ... 
  // ... wichtig für das FADE OUT / FADE IN --> NUR(!) bei Änderung mindestens eines Zeichens
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    shownletter[LEDindicator] = newletter[LEDindicator];
  } 

}   

#endif

// ++++++++++ RUN LED CLOCK - END +++++++++++++++++++++++++++++++++++++++++++++



