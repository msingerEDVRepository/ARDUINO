#ifndef LEDCLOCK_ENGLISCH_H
#define LEDCLOCK_ENGLISCH_H

#include <Arduino.h>

#define FASTLED_INTERNAL
#define FASTLED_ALLOW_INTERRUPTS 0

#include <FastLED.h>        // Used library FastLED (by Daniel Garcia / Version 3.10.3) - used for programming the LED stripes

#define DATA_PIN            14 // (for ESP32 )
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
const int NumberOfMinute = 5; // Original: 4;
const int NumberOfItIs = 2;
const int NumberOfToPast = 2;
const int NumberOfHalfFullMinutes = 3;


//Adressen Es/Ist
const int ItIsAddr[2][2] = { {2, 3},    //IT
                              {5, 6} };  //IS

//Adressen Vor/Nach
const int ToPastAddr[2][2] = { {49, 52},              //PAST
                               {53, 54} };            //TO

//Adressen Halb/Voll
const int HalfFullMinutesAddr[3][2] = { {44, 47},     //HALF
                                     {115, 120},      //O'CLOCK
                                     {33, 39} };      //MINUTES

//Adressen der Stunden
const int HourAddr[12][2] = { {88, 90},           //ONE
                              {107, 109},         //TWO
                              {61, 65},           //THREE
                              {91, 94},           //FOUR
                              {66, 69},           //FIVE
                              {55, 57},           //SIX
                              {72, 76},           //SEVEN
                              {77, 81},           //EIGHT
                              {95, 98},           //NINE
                              {58, 60},           //TEN
                              {100, 105},         //ELEVEN
                              {82, 87} };         //TWELVE       

// Adressen der Minuten
const int MinuteAddr[5][2] = {  {12, 15},         //FIVE
                                {41, 43},         //TEN
                                {23, 31},         //A QUARTER (ACHTUNG: Pixel 24 ausblenden weil BLANK)
                                {16, 21},         //TWENTY
                                {12, 21} };       //TWENTYFIVE

// Zeittabelle mit Takt +2/-2 Minuten
// ++++++++++++++++++++++++++++++++++
// Array Element0 = 0 - 59ste Minute
// Array Element1 = Element aus Array "MinuteAddr" (example = 1 = "FIVE" , 2 = "TEN", 3 = "QUARTER", 4 = "TWENTY", 5 = "TWENTYFIVE")
// Array Element2 = Element aus Array "ToPastAddr" (example = 1 = "PAST", 2 = "TO")
// Array Element3 = Element aus Array "HalfFullMinutesAddr" (example = 1 = "HALF", 2 = "O'CLOCK", 3 = "MINUTES")
// ++++++++++++++++++++++++++++++++++
// ++++ (Alle Elemente sind m.E. um "1" verschoben, da der erste Eintrag = "0" == "keine Anzeige" bedeutet) +++++

const int TimeTable[60][4] = { {0,0,0,2},{1,0,0,2},{2,0,0,2},{3,1,1,3},{4,1,1,3},{5,1,1,3},{6,1,1,3},{7,1,1,3},{8,2,1,3},{9,2,1,3},
                              {10,2,1,3},{11,2,1,3},{12,2,1,3},{13,3,1,0},{14,3,1,0},{15,3,1,0},{16,3,1,0},{17,3,1,0},{18,4,1,3},{19,4,1,3},
                              {20,4,1,3},{21,4,1,3},{22,4,1,3},{23,5,1,3},{24,5,1,3},{25,5,1,3},{26,5,1,3},{27,5,1,3},{28,0,1,1},{29,0,1,1},
                              {30,0,1,1},{31,0,1,1},{32,0,1,1},{33,5,2,3},{34,5,2,3},{35,5,2,3},{36,5,2,3},{37,5,2,3},{38,4,2,3},{39,4,2,3},
                              {40,4,2,3},{41,4,2,3},{42,4,2,3},{43,3,2,0},{44,3,2,0},{45,3,2,0},{46,3,2,0},{47,3,2,0},{48,2,2,3},{49,2,2,3},
                              {50,2,2,3},{51,2,2,3},{52,2,2,3},{53,1,2,3},{54,1,2,3},{55,1,2,3},{56,1,2,3},{57,1,2,3},{58,0,0,2},{59,0,0,2} };

// +++++++++++++ NTP Server for getting DATE & TIME

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
//const long  gmtOffset_sec = 3600;
// int   daylightOffset_sec = 3600;

int timeInfoSuccess = 0;
const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

// time_t enthält die Anzahl der Sekunden seit dem 1.1.1970 0 Uhr
// time_t aktuelleZeit;

/* 
  Struktur tm
  tm_hour -> Stunde: 0 bis 23
  tm_min -> Minuten: 0 bis 59
  tm_sec -> Sekunden 0 bis 59
  tm_mday -> Tag 1 bis 31
  tm_wday -> Wochentag (0 = Sonntag, 6 = Samstag)
  tm_mon -> Monat: 0 (Januar) bis 11 (Dezember)
  tm_year -> Jahre seit 1900
  tm_yday -> vergangene Tage seit 1. Januar des Jahres
  tm_isdst -> Wert > 0 = Sommerzeit (dst = daylight saving time)
*/

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

    // IT, IS
    for (LEDindicator = ItIsAddr[0][0]; LEDindicator <= ItIsAddr[0][1]; LEDindicator++){ 
      LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
    }

    for (LEDindicator = ItIsAddr[1][0]; LEDindicator <= ItIsAddr[1][1]; LEDindicator++){ 
      LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
    }

    // MINUTEADDRESS
    long minuteAddressColumn = 99;
    minuteAddressColumn = random(0,4);
    for (LEDindicator = MinuteAddr[minuteAddressColumn][0]; LEDindicator <= MinuteAddr[minuteAddressColumn][1]; LEDindicator++){ 
      LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
    }

    // TO / PAST
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


// +++++++++ LED COUNTDOWN - END ++++++++++++++++

// ++++++++++ LED Wait during NTP Server-Connect (after WLAN is connected)
void waitForTimeSuccess(){

  LEDstripArray[38].setRGB(100,0,0);
  LEDstripArray[49].setRGB(100,0,0);
  LEDstripArray[60].setRGB(100,0,0);
  LEDstripArray[71].setRGB(100,0,0);
  LEDstripArray[82].setRGB(100,0,0); 

  // EINBLENDEN
  for (int k = 0; k <= brightnessLED; k++) {
    FastLED.setBrightness(k);
    FastLED.delay(10);
    FastLED.show();
  }
  
  // AUSBLENDEN
  for (int k = brightnessLED; k >= 0; k--) {
    FastLED.setBrightness(k);
    FastLED.delay(5);
    FastLED.show();
  }

}
// +++++++++++ LED Wait  ++++++++++ ENDE ++++++++

// ++++++++++ RUN LED CLOCK - START +++++++++++++
void runClockPlatt(){

  // In jedem LOOP das NEWLETTER Array wieder zurücksetzen und damit für das WRITE-Array initialisieren
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    newletter[LEDindicator] = 0;   
  }

  // ++++++ 1. Wort "DAT" ausgeben (immer) ++++++++++
  WriteArray(0, ItIsAddr, newletter, 1);

  // ++++++ 2. Wort "IS" ausgeben (immer) +++++++++++
  WriteArray(1, ItIsAddr, newletter, 1);

  // ++++++ 3. Ausgabe der Stunde (in Abhängigkeit der MEZ "Stunde") +++++++++

  // Diese Werte werden in dem Wert "aktuelleStunde" übergeben.
  // für 00 Uhr - 12 Uhr --> wird 00 Uhr  - 12 Uhr (keine Umsetzung)
  // aus 13 Uhr - 23 Uhr --> wird 01 Uhr  - 11 Uhr 

  // eine Stunde - vor(!) Erreichen der "vollen Stunde" addieren;
  // Beispiel: da 20:30 Uhr 
  // muss in Halb NEUN gewandelt werden muss und nicht in halb ACHT
  // aber 12:58 Uhr muss "Klock Een" werden und nich 13:58 Uhr 

  if (aktuelleMinute >= 33) {
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

  Serial.println ("Die vorherige Ausgabe-Stunde (12h) ist : " + String(alteStunde12h));
  Serial.println ("Die aktuelle Ausgabe-Stunde (12h) ist : " + String(aktuelleStunde12h));
  Serial.println ("Die aktuelle Ausgabe-Minute (12h) ist : " + String(aktuelleMinute));

  //Anzeige der Stunden (Hier wird 1 von der aktuell ermittelten Stunde abgezogen, da das Array bei 0 (und nicht bei 1) beginnt.)
  WriteArray(aktuelleStunde12h - 1, HourAddr, newletter, 1);

  // ++++++ 4. Ausgabe der Minuten (in Abhängigkeit der MEZ "Minuten") ++++++++++++ 
              
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
        WriteArray(pointer, HalfFullMinutesAddr, newletter, 1);
      }
      break;
    }  
  }

  // ++++++++++ Dieser Sonderfall wird  bei "ENGLISH" for "A" - QUARTER gebraucht +++++++++++++
  //Beispiel: IT IS "A" QUARTER PAST FIVE //  {23, 31}, (ACHTUNG: Pixel 24 ausblenden weil BLANK)
  newletter[24] = 0;
  // ++++++++ Sonderfall "ENGLISH" Ende ++++++++++++++++

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

  // Ermittle Abschalt-Intervall START für Nightmode
  if ( savedNIGHTSTART >= 21 || savedNIGHTSTART <= 2 ) {        // NightMode - Startwert liegt zwischen 21:00 Uhr und 02:00 Uhr 
    if (aktuelleStunde24h >= savedNIGHTSTART) {
      nightSwitchOff = true;
    }
  }

  // Ermittle Abschalt-Intervall END für Nightmode
  if (aktuelleStunde24h >= savedNIGHTEND) {                         // Erneutes Anschalten liegt zwischen 04:00 und 09:00 Uhr
    nightSwitchOff = false;
  } else {
    nightSwitchOff = true;
  }

  Serial.println ("aktuelleStunde24h = " + String(aktuelleStunde24h));
  Serial.println ("savedNIGHTJN = " + String(savedNIGHTJN));
  Serial.println ("savedNIGHTSTART = " + String(savedNIGHTSTART));
  Serial.println ("savedNIGHTEND = " + String(savedNIGHTEND));
  Serial.println ("nightSwitchOff = " + String(nightSwitchOff));

  // Vergleich, ob sich die Anzeige ändern muss, weil sich mindestens ein Zeichen geändert hat.
  letterchanged = false;

  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {       

    if (shownletter[LEDindicator] != newletter[LEDindicator]) {
      letterchanged = true;
    }

  }

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



