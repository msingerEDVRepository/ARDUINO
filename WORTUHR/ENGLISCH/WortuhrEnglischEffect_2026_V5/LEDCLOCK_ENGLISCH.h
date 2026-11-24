#ifndef LEDCLOCK_ENGLISCH_H
#define LEDCLOCK_ENGLISCH_H

#include <Arduino.h>

#define FASTLED_INTERNAL
#define FASTLED_ALLOW_INTERRUPTS 0

#include <FastLED.h>        // Used library FastLED (by Daniel Garcia / Version 3.10.10) - used for programming the LED stripes

#define DATA_PIN            21 // (for ESP32 - GPIO 21)
#define NUMPIXELS           121
#define MAX_POWER_MILLIAMPS 500
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
#define BRIGHTNESS          150

#include <EEPROM.h>

// Zugriff auf das im .ino deklarierte WebServer-Objekt, damit lange
// blockierende Effekte (Countdown, Rain-In, Fly-Out, Letter-Hide) den
// Web-Server zwischendurch bedienen können und die UI nicht für Sekunden einfriert.
#include <WebServer.h>
extern WebServer server;

#define EEPROM_SIZE 512
// EEPROM Adressen
#define ADDR_LED_RED 230
#define ADDR_LED_GREEN 240
#define ADDR_LED_BLUE 250
#define ADDR_NIGHTJN 260
#define ADDR_NIGHT_START 270
#define ADDR_NIGHT_END 290
#define ADDR_EFFECTSJN 310    // AN/AUS für die Ein-/Ausblend-Effekte (Rain-In / Fly-Out / Letter-Hide)

// ++++ for NTP Server Connection and readtime method
#include "time.h"
#include "esp_sntp.h"         // ESP32-Core ab 3.x: "sntp.h" gibt es nicht mehr

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

// +++++++++++ WORDCLOCK Array for displaying time in "ENGLISH" +++++++++++

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

bool  savedEFFECTSJN = false;   // AN = Rain-In/Fly-Out/Letter-Hide Effekte, AUS = einfaches Fade

bool nightSwitchOff = false;

unsigned int pointer = 0;

unsigned int newletter[NUMPIXELS];
unsigned int shownletter[NUMPIXELS];

// Schaltet bei jedem tatsächlichen Anzeigewechsel (also i.d.R. alle 5 Minuten)
// zwischen den beiden festen Ein-/Ausblend-Effekt-Tupeln um:
// true  -> Fly-Out (Ausblenden) + Fly-In (Einblenden)
// false -> Letter-by-Letter-Hide (Ausblenden) + Rain-In (Einblenden)
bool effectPairToggle = false;

// Forward-Deklarationen der Ein-/Ausblend-Effekte (vollständige Definitionen weiter
// unten in dieser Datei), damit sie bereits in runCountdownPlatt() genutzt werden können.
void matrixFlyOutHide(unsigned int *sourceArray);
void matrixFlyIn(unsigned int *targetArray);
void matrixLetterByLetterHide(unsigned int *sourceArray);
void matrixRainReveal(unsigned int *targetArray);

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


// +++++++++++++ NTP Server for getting DATE & TIME ++++++++++++

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";

int timeInfoSuccess = 0;
const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Berlin including daylight adjustment rules

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

  // Timeout 200 ms statt Standard 5000 ms: solange noch keine NTP-Zeit da ist,
  // blockierte getLocalTime() sonst bei jedem Aufruf 5 Sekunden lang die Loop.
  if(!getLocalTime(&timeinfo, 200)){
    Serial.println("No time available (yet)");
    return;
  } else {
    Serial.println("NTP Time available (yet)");
    timeInfoSuccess = 1;
  }

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
// ++++++++++++++ NTP Server end ++++++++++++++++++++++++++

// ++++++++ Write LED ARRAY  ++++ START +++++++++++

void WriteArray(int Pointer, const int Addr[][2], unsigned int *Array, int Value){

  int c1 = 0;

  //Array-Bereich beschreiben
  for(c1 = Addr[Pointer][0]; c1 <= Addr[Pointer][1]; c1++) {
    // Bounds-Check: schützt vor Speicherkorruption, falls eine Adresstabelle
    // versehentlich einen Index außerhalb von 0..NUMPIXELS-1 enthält
    if (c1 >= 0 && c1 < NUMPIXELS) {
      Array[c1] = Value;
    }
  }
}

// ++++++++ Write LED ARRAY  ++++ END +++++++++++++

// ++++++++ Check Nachabschaltung ++++ START +++++++++++

bool nightSwitchOffCheck() {


  // +++++++++GET EEPROM stored values
  savedNIGHTJN = EEPROM.read(ADDR_NIGHTJN);
  savedNIGHTSTART = EEPROM.read(ADDR_NIGHT_START);
  savedNIGHTEND = EEPROM.read(ADDR_NIGHT_END);

  // FIX: RGB-Werte hier ebenfalls IMMER frisch aus dem EEPROM lesen - vorher wurden
  // savedRGBRED/GREEN/BLUE nur EINMALIG in runCountdownPlatt() gesetzt, wodurch eine
  // über die Web-UI geänderte Farbe erst nach einem Neustart sichtbar wurde.
  savedRGBRED   = EEPROM.read(ADDR_LED_RED);
  savedRGBGREEN = EEPROM.read(ADDR_LED_GREEN);
  savedRGBBLUE  = EEPROM.read(ADDR_LED_BLUE);

  // Ein-/Ausblend-Effekte AN/AUS ebenfalls laufend aktuell halten
  savedEFFECTSJN = EEPROM.read(ADDR_EFFECTSJN);

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
  savedEFFECTSJN = EEPROM.read(ADDR_EFFECTSJN);

  // shownletter zu Beginn auf "nichts an" zurücksetzen - der erste Countdown-Schritt
  // blendet dadurch sauber aus dem Dunkeln ein.
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    shownletter[LEDindicator] = 0;
  }

  // eigene, lokale Tupel-Umschaltung für den Countdown-Demolauf (unabhängig von
  // der "echten" Uhr, damit der Countdown bei jedem Start wieder von vorne beginnt)
  bool countdownEffectPairToggle = false;

  // Bei aktivierten Ein-/Ausblend-Effekten (savedEFFECTSJN == true) dauert ein
  // einzelner Countdown-Schritt durch die Animationen spürbar länger - daher dort
  // in 3er-Schritten zählen, damit nur 4 statt 12 Durchläufe stattfinden
  // (11 -> 8 -> 5 -> 2).
  int countdownStep = savedEFFECTSJN ? 3 : 1;

  for (int countdownCounter = 11; countdownCounter >= 0; countdownCounter -= countdownStep) {

    // Ziel-Buchstaben dieses Countdown-Schritts in "newletter[]" aufbauen
    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
      newletter[LEDindicator] = 0;
    }

    // IT, IS
    WriteArray(0, ItIsAddr, newletter, 1);
    WriteArray(1, ItIsAddr, newletter, 1);

    // MINUTEADDRESS
    long minuteAddressColumn = random(0,5);   // 5 Minutenwörter (inkl. TWENTYFIVE)
    WriteArray(minuteAddressColumn, MinuteAddr, newletter, 1);

    // TO / PAST
    if (countdownCounter%2 == 0) {
      WriteArray(1, ToPastAddr, newletter, 1);
    } else {
      WriteArray(0, ToPastAddr, newletter, 1);
    }

    // HOURADDRESS
    WriteArray(countdownCounter, HourAddr, newletter, 1);

    // Sonderfall "ENGLISH": Pixel 24 (Leerstelle in "A QUARTER") bleibt immer aus
    newletter[24] = 0;

    if (savedEFFECTSJN) {

      // ++++ Effekte AN: dieselben zwei festen Tupel wie bei der echten Uhr, im Wechsel ++++
      if (countdownEffectPairToggle) {
        matrixFlyOutHide(shownletter);
        matrixFlyIn(newletter);
      } else {
        matrixLetterByLetterHide(shownletter);
        matrixRainReveal(newletter);
      }
      countdownEffectPairToggle = !countdownEffectPairToggle;

      // kurze "Lesepause", bevor der nächste Countdown-Schritt zu blenden beginnt
      for (int w = 0; w < 8; w++) {
        server.handleClient();
        delay(100);
      }

    } else {

      // ++++ Effekte AUS: einfaches Direkt-Schreiben wie bisher ++++
      for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
        LEDstripArray[LEDindicator] = CRGB::Black;
      }
      for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
        if (newletter[LEDindicator] == 1) {
          LEDstripArray[LEDindicator].setRGB(savedRGBRED, savedRGBGREEN, savedRGBBLUE);
        }
      }
      FastLED.show();
      // in kleinere Schritte aufgeteilt, damit die Web-UI während des ~14s
      // langen Countdowns weiterhin erreichbar bleibt
      for (int w = 0; w < 10; w++) {
        server.handleClient();
        delay(100);
      }

      // alle LED ausschalten
      for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++){
        LEDstripArray[LEDindicator] = CRGB::Black;
      }
      FastLED.show();
    }

    // shownletter für den nächsten Durchlauf übernehmen
    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
      shownletter[LEDindicator] = newletter[LEDindicator];
    }

    Serial.println("Countdown-Step : " + String(countdownCounter));

    server.handleClient();
    delay(200);

  }

  // Nach dem Countdown alles zurücksetzen, damit die echte Uhr später sauber
  // (mit korrektem shownletter/newletter-Ausgangszustand) starten kann.
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    LEDstripArray[LEDindicator] = CRGB::Black;
    newletter[LEDindicator] = 0;
    shownletter[LEDindicator] = 0;
  }
  FastLED.show();
}

// +++++++++ LED COUNTDOWN - END ++++++++++++++++

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
    if (k % 10 == 0) server.handleClient();
  }

  server.handleClient();
  delay(250);
  
  // AUSBLENDEN
  for (int k = brightnessLED; k >= 0; k--) {
    FastLED.setBrightness(k);
    FastLED.delay(10);
    FastLED.show();
    if (k % 10 == 0) server.handleClient();
  }

  server.handleClient();
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
    if (k % 10 == 0) server.handleClient();
  }

  server.handleClient();
  delay(250);
  
  // AUSBLENDEN
  for (int k = brightnessLED; k >= 0; k--) {
    FastLED.setBrightness(k);
    FastLED.delay(10);
    FastLED.show();
    if (k % 10 == 0) server.handleClient();
  }

  server.handleClient();
  delay(100);

}

// ++++++ END +++++ LED Wait  +++++++++++++++++++++++++++++++


// ++++++++++ MATRIX-RAIN REVEAL Effekt - START +++++++++++++++
// Klassischer "Matrix-Regen": in JEDER Spalte läuft - zeitlich zufällig
// versetzt startend - kontinuierlich ein Lichttropfen von oben nach unten
// durch (und beginnt danach wieder von vorne = nächster Durchlauf).
// Erst NACH zwei kompletten Durchläufen "rastet" eine Spalte ein:
// - besitzt die Spalte Ziel-Buchstaben (newletter[] == 1), bleiben genau
//   diese LED in der finalen Farbe stehen,
// - besitzt die Spalte keinen Ziel-Buchstaben, rieselt sie einfach leer
//   nach unten aus dem Display heraus (wird danach einfach nicht mehr
//   gezeichnet).
// Da jede Spalte ihren eigenen zufälligen Start-Zeitpunkt hat, "rasten"
// die Buchstaben eines Wortes ungeordnet/zeitlich versetzt ein.
// Es wird bewusst KEIN zusätzliches Koordinatensystem eingeführt - Zeile
// und Spalte werden aus dem bereits vorhandenen physikalischen LED-Index
// (0-120, Serpentinen-Verkabelung) zurückgerechnet.

#define MATRIX_WIDTH               11
#define MATRIX_HEIGHT               11
#define RAIN_TRAIL_LENGTH            2     // Länge der Lichtspur hinter dem "Kopf" des Tropfens (ganz wenig Nachglimmen)
#define RAIN_STEP_DELAY_MS          35     // Framerate der Animation
#define RAIN_FALL_DURATION_MS     1700     // Dauer für EINEN kompletten Durchlauf einer Spalte (langsamer)
#define RAIN_REQUIRED_CYCLES          2     // Anzahl kompletter Durchläufe, bevor eine Spalte "einrastet"
#define RAIN_COLUMN_START_JITTER_MS 1400     // zufälliger Start-Versatz je Spalte (größerer Abstand zwischen den Bahnen)
#define RAIN_TARGETS_PER_COLUMN       6     // Reserve: max. Anzahl Ziel-Buchstaben in derselben Spalte

// Zeile (0-10) aus dem physikalischen LED-Index ermitteln
int rainRowFromIndex(int ledIndex) {
  return ledIndex / MATRIX_WIDTH;
}

// Spalte (0-10) aus dem physikalischen LED-Index ermitteln - unter
// Berücksichtigung der Serpentinen-Verkabelung (jede 2. Zeile läuft rückwärts)
int rainColFromIndex(int ledIndex) {
  int row = rainRowFromIndex(ledIndex);
  int offset = ledIndex % MATRIX_WIDTH;
  if (row % 2 == 0) {
    return offset;
  } else {
    return (MATRIX_WIDTH - 1) - offset;
  }
}

// wandelt (Zeile, Spalte) zurück in den physikalischen LED-Index
int rainIndexFromRowCol(int row, int col) {
  if (row % 2 == 0) {
    return row * MATRIX_WIDTH + col;
  } else {
    return row * MATRIX_WIDTH + ((MATRIX_WIDTH - 1) - col);
  }
}

// targetArray: z.B. "newletter[]" - alle Einträge mit Wert 1 markieren,
// wo am Ende ein Buchstabe des neuen Wortes stehen bleiben soll.
void matrixRainReveal(unsigned int *targetArray) {

  // Pro Spalte: welche Zeilen sollen am Ende (fest) leuchten?
  int colTargetRows[MATRIX_WIDTH][RAIN_TARGETS_PER_COLUMN];
  int colTargetCount[MATRIX_WIDTH];
  bool anyTarget = false;

  for (int c = 0; c < MATRIX_WIDTH; c++) {
    colTargetCount[c] = 0;
  }

  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    if (targetArray[LEDindicator] == 1) {
      int row = rainRowFromIndex(LEDindicator);
      int col = rainColFromIndex(LEDindicator);
      if (colTargetCount[col] < RAIN_TARGETS_PER_COLUMN) {
        colTargetRows[col][colTargetCount[col]] = row;
        colTargetCount[col]++;
        anyTarget = true;
      }
    }
  }

  if (!anyTarget) return;

  RGB_RED   = savedRGBRED;
  RGB_GREEN = savedRGBGREEN;
  RGB_BLUE  = savedRGBBLUE;

  // Die eingestellte Farbe wird sowohl für die Lichtspur (gedimmt)
  // als auch für die finale Farbe des Buchstabens verwendet.
  CRGB targetColor = CRGB(RGB_RED, RGB_GREEN, RGB_BLUE);
  CRGB trailColor  = targetColor;

  FastLED.setBrightness(brightnessLED);

  // jede Spalte bekommt einen zufälligen Start-Versatz ("randomly startend")
  unsigned long colStartDelay[MATRIX_WIDTH];
  unsigned long maxTotalDuration = 0;
  for (int c = 0; c < MATRIX_WIDTH; c++) {
    colStartDelay[c] = (unsigned long)random(0, RAIN_COLUMN_START_JITTER_MS);
    unsigned long colTotal = colStartDelay[c] + (unsigned long)(RAIN_REQUIRED_CYCLES * RAIN_FALL_DURATION_MS);
    if (colTotal > maxTotalDuration) maxTotalDuration = colTotal;
  }

  // Distanz, die ein Tropfen in EINEM Durchlauf "sichtbar zurücklegt".
  // WICHTIG: hier bewusst "- 1" gegenüber (MATRIX_HEIGHT + RAIN_TRAIL_LENGTH),
  // damit beim Zyklus-Wrap (Ende Durchlauf 1 -> Start Durchlauf 2) KEINE
  // Lücke entsteht, in der die Spalte kurz komplett dunkel wäre. Mit dem
  // alten Wert gab es einen kurzen "Blackout" genau am Übergang zwischen
  // zwei Durchläufen, weil der letzte Rest des alten Tropfens bereits
  // unsichtbar war, bevor der Kopf des neuen Tropfens am oberen Rand
  // wieder sichtbar wurde. Mit "-1" schließt der neue Kopf (Zeile 0)
  // nahtlos an das letzte sichtbare Pixel (Zeile 10) des alten Tropfens an.
  const float cycleDistance = (float)(MATRIX_HEIGHT + RAIN_TRAIL_LENGTH - 1);
  const float rowsPerMs = cycleDistance / (float)RAIN_FALL_DURATION_MS;

  unsigned long startTime = millis();

  while (true) {

    unsigned long elapsed = millis() - startTime;

    // Frame löschen, danach nur die aktiven Spalten neu zeichnen
    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
      LEDstripArray[LEDindicator] = CRGB::Black;
    }

    for (int c = 0; c < MATRIX_WIDTH; c++) {

      // Spalte wartet noch auf ihren zufälligen Start
      if (elapsed < colStartDelay[c]) {
        continue;
      }

      unsigned long colElapsed = elapsed - colStartDelay[c];
      float totalRows = (float)colElapsed * rowsPerMs;
      int cyclesCompleted = (int)(totalRows / cycleDistance);

      if (cyclesCompleted >= RAIN_REQUIRED_CYCLES) {
        // Spalte ist "eingerastet": nur noch die Ziel-Buchstaben (falls vorhanden)
        // in finaler Farbe zeigen - alles andere ist bereits unten herausgerieselt.
        for (int t = 0; t < colTargetCount[c]; t++) {
          int idx = rainIndexFromRowCol(colTargetRows[c][t], c);
          LEDstripArray[idx] = targetColor;
        }
        continue;
      }

      // Spalte rieselt noch: Position des Tropfen-"Kopfes" innerhalb des aktuellen Durchlaufs.
      // KEIN "- RAIN_TRAIL_LENGTH" Offset mehr (siehe Kommentar bei cycleDistance) -
      // der Kopf beginnt jeden Durchlauf direkt sichtbar bei Zeile 0.
      float posInCycle = fmod(totalRows, cycleDistance);
      int headRow = (int)floor(posInCycle);

      for (int t = 0; t < RAIN_TRAIL_LENGTH; t++) {
        int r = headRow - t;
        if (r < 0 || r > (MATRIX_HEIGHT - 1)) continue;

        int idx = rainIndexFromRowCol(r, c);
        uint8_t fade = 255 - (t * (255 / RAIN_TRAIL_LENGTH));

        CRGB col = trailColor;
        col.nscale8(fade);
        LEDstripArray[idx] = col;
      }
    }

    FastLED.show();
    server.handleClient();   // Web-UI während der mehrsekündigen Animation erreichbar halten

    if (elapsed >= maxTotalDuration) break;
    delay(RAIN_STEP_DELAY_MS);
  }

  // Sicherheitshalber am Ende alle Ziel-LED garantiert in der finalen Farbe
  for (int c = 0; c < MATRIX_WIDTH; c++) {
    for (int t = 0; t < colTargetCount[c]; t++) {
      int idx = rainIndexFromRowCol(colTargetRows[c][t], c);
      LEDstripArray[idx] = targetColor;
    }
  }
  FastLED.show();
}
// ++++++++++ MATRIX-RAIN REVEAL Effekt - ENDE +++++++++++++++


// ++++++++++ MATRIX FLY-OUT (AUSBLENDEN) Effekt - START +++++++++++++++
// Lässt NUR die Buchstaben, die beim Uhrzeitwechsel wirklich VERSCHWINDEN
// (also in "shownletter[]" an, aber in "newletter[]" aus sind), je Zeile
// seitlich aus dem Display herausfliegen. Buchstaben, die im neuen Wort
// unverändert weiterhin gebraucht werden (z.B. "ES IST"), fliegen NICHT mit
// heraus, sondern bleiben die ganze Zeit über durchgehend an.
// Buchstaben in der rechten Zeilenhälfte fliegen nach LINKS heraus,
// Buchstaben in der linken Zeilenhälfte fliegen nach RECHTS heraus.
// Die Farbe bleibt dabei unverändert (kein Fade, kein Farbwechsel).

#define FLYOUT_DURATION_MS   2200   // Dauer des Herausfliegens (deutlich langsamer/sichtbarer)
#define FLYOUT_STEP_DELAY_MS   20   // Framerate
#define FLYOUT_MAX_LETTERS     40   // Reserve für die längstmögliche Wortkombination

struct FlyOutLetter {
  int   row;
  float startCol;
  float exitCol;      // Ziel-Spalte AUSSERHALB des sichtbaren Displays
};

// sourceArray: z.B. "shownletter[]". Vergleicht intern gegen das globale
// "newletter[]", um nur die tatsächlich verschwindenden Buchstaben zu animieren -
// unveränderte (weiterhin benötigte) Buchstaben bleiben unangetastet an.
void matrixFlyOutHide(unsigned int *sourceArray) {

  FlyOutLetter letters[FLYOUT_MAX_LETTERS];
  int letterCount = 0;

  int persistIndexList[FLYOUT_MAX_LETTERS];
  int persistCount = 0;

  // aus dem Quell-Array nur die LED einsammeln, die wirklich verschwinden;
  // LED, die im neuen Wort ebenfalls an sind, gelten als "unverändert"
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {

    if (sourceArray[LEDindicator] != 1) continue;

    if (newletter[LEDindicator] == 1) {
      // unverändert -> bleibt einfach an, keine Animation
      if (persistCount < FLYOUT_MAX_LETTERS) {
        persistIndexList[persistCount] = LEDindicator;
        persistCount++;
      }
      continue;
    }

    // verschwindet wirklich -> fliegt raus
    if (letterCount < FLYOUT_MAX_LETTERS) {

      int row = rainRowFromIndex(LEDindicator);
      int col = rainColFromIndex(LEDindicator);

      letters[letterCount].row      = row;
      letters[letterCount].startCol = (float)col;

      if (col < (MATRIX_WIDTH / 2)) {
        // linke Zeilenhälfte -> fliegt nach RECHTS heraus
        letters[letterCount].exitCol = (float)(MATRIX_WIDTH + 3);
      } else {
        // rechte Zeilenhälfte -> fliegt nach LINKS heraus
        letters[letterCount].exitCol = -3.0;
      }

      letterCount++;
    }
  }

  if (letterCount == 0) return;   // nichts verschwindet -> nichts zu tun (unveränderte LED bleiben unangetastet)

  RGB_RED   = savedRGBRED;
  RGB_GREEN = savedRGBGREEN;
  RGB_BLUE  = savedRGBBLUE;

  // Farbe bleibt beim Herausfliegen UNVERÄNDERT (kein Fade, kein Farbwechsel)
  CRGB currentColor = CRGB(RGB_RED, RGB_GREEN, RGB_BLUE);

  FastLED.setBrightness(brightnessLED);

  unsigned long startTime = millis();

  while (true) {

    unsigned long elapsed = millis() - startTime;
    float progress = (float)elapsed / FLYOUT_DURATION_MS;
    if (progress > 1.0) progress = 1.0;

    float eased = progress * progress;   // Ease-In -> beschleunigtes "Wegfliegen"

    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
      LEDstripArray[LEDindicator] = CRGB::Black;
    }

    // unveränderte Buchstaben bleiben die ganze Zeit über durchgehend an
    for (int i = 0; i < persistCount; i++) {
      LEDstripArray[persistIndexList[i]] = currentColor;
    }

    for (int i = 0; i < letterCount; i++) {

      float colF = letters[i].startCol + eased * (letters[i].exitCol - letters[i].startCol);
      int col = (int)round(colF);

      if (col < 0 || col > (MATRIX_WIDTH - 1)) continue;   // schon außerhalb -> nicht mehr zeichnen

      int idx = rainIndexFromRowCol(letters[i].row, col);
      LEDstripArray[idx] = currentColor;   // volle, unveränderte Farbe
    }

    FastLED.show();
    server.handleClient();   // Web-UI während der Animation erreichbar halten

    if (progress >= 1.0) break;
    delay(FLYOUT_STEP_DELAY_MS);
  }

  // Am Ende: nur die verschwundenen Buchstaben sind weg, die unveränderten bleiben an
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    LEDstripArray[LEDindicator] = CRGB::Black;
  }
  for (int i = 0; i < persistCount; i++) {
    LEDstripArray[persistIndexList[i]] = currentColor;
  }
  FastLED.show();
}
// ++++++++++ MATRIX FLY-OUT (AUSBLENDEN) Effekt - ENDE +++++++++++++++


// ++++++++++ MATRIX FLY-IN (EINBLENDEN) Effekt - START +++++++++++++++
// Analog zum Fly-Out: lässt NUR die Buchstaben, die beim Uhrzeitwechsel
// wirklich NEU dazukommen (also in "newletter[]" an, aber in "shownletter[]"
// aus sind), je Zeile seitlich ins Display hereinfliegen. Buchstaben, die
// unverändert weiterhin gebraucht werden (z.B. "ES IST"), fliegen NICHT mit
// rein, sondern waren/bleiben einfach durchgehend an.
// Die Flugrichtung ist genau umgekehrt zu Fly-Out: Buchstaben, die in der
// linken Zeilenhälfte landen, kommen von RECHTS hereingeflogen; Buchstaben,
// die in der rechten Zeilenhälfte landen, kommen von LINKS hereingeflogen.
// Die Farbe bleibt dabei unverändert (kein Fade, kein Farbwechsel).

#define FLYIN_DURATION_MS   2200   // Dauer des Hereinfliegens
#define FLYIN_STEP_DELAY_MS   20   // Framerate
#define FLYIN_MAX_LETTERS     40   // Reserve für die längstmögliche Wortkombination

struct FlyInLetter {
  int   row;
  float startCol;    // Startposition AUSSERHALB des sichtbaren Displays
  float targetCol;
};

// targetArray: z.B. "newletter[]". Vergleicht intern gegen das globale
// "shownletter[]", um nur die tatsächlich neu hinzukommenden Buchstaben zu
// animieren - unveränderte (bereits vorher gezeigte) Buchstaben bleiben
// unangetastet an.
void matrixFlyIn(unsigned int *targetArray) {

  FlyInLetter letters[FLYIN_MAX_LETTERS];
  int letterCount = 0;

  int persistIndexList[FLYIN_MAX_LETTERS];
  int persistCount = 0;

  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {

    if (targetArray[LEDindicator] != 1) continue;

    if (shownletter[LEDindicator] == 1) {
      // unverändert -> war schon an, bleibt einfach an
      if (persistCount < FLYIN_MAX_LETTERS) {
        persistIndexList[persistCount] = LEDindicator;
        persistCount++;
      }
      continue;
    }

    // wirklich neu -> fliegt rein
    if (letterCount < FLYIN_MAX_LETTERS) {

      int row = rainRowFromIndex(LEDindicator);
      int col = rainColFromIndex(LEDindicator);

      letters[letterCount].row       = row;
      letters[letterCount].targetCol = (float)col;

      if (col < (MATRIX_WIDTH / 2)) {
        // Ziel in linker Zeilenhälfte -> kommt von RECHTS hereingeflogen
        letters[letterCount].startCol = (float)(MATRIX_WIDTH + 3);
      } else {
        // Ziel in rechter Zeilenhälfte -> kommt von LINKS hereingeflogen
        letters[letterCount].startCol = -3.0;
      }

      letterCount++;
    }
  }

  RGB_RED   = savedRGBRED;
  RGB_GREEN = savedRGBGREEN;
  RGB_BLUE  = savedRGBBLUE;

  // Farbe bleibt beim Hereinfliegen UNVERÄNDERT (kein Fade, kein Farbwechsel)
  CRGB currentColor = CRGB(RGB_RED, RGB_GREEN, RGB_BLUE);

  FastLED.setBrightness(brightnessLED);

  if (letterCount == 0) {
    // nichts wirklich Neues -> nur sicherstellen, dass die unveränderten
    // Buchstaben stehen, sonst nichts zu animieren
    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
      LEDstripArray[LEDindicator] = CRGB::Black;
    }
    for (int i = 0; i < persistCount; i++) {
      LEDstripArray[persistIndexList[i]] = currentColor;
    }
    FastLED.show();
    return;
  }

  unsigned long startTime = millis();

  while (true) {

    unsigned long elapsed = millis() - startTime;
    float progress = (float)elapsed / FLYIN_DURATION_MS;
    if (progress > 1.0) progress = 1.0;

    float eased = 1.0 - (1.0 - progress) * (1.0 - progress);   // Ease-Out -> sanftes "Einschweben"

    for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
      LEDstripArray[LEDindicator] = CRGB::Black;
    }

    // unveränderte Buchstaben bleiben die ganze Zeit über durchgehend an
    for (int i = 0; i < persistCount; i++) {
      LEDstripArray[persistIndexList[i]] = currentColor;
    }

    for (int i = 0; i < letterCount; i++) {

      float colF = letters[i].startCol + eased * (letters[i].targetCol - letters[i].startCol);
      int col = (int)round(colF);

      if (col < 0 || col > (MATRIX_WIDTH - 1)) continue;   // noch außerhalb -> nicht zeichnen

      int idx = rainIndexFromRowCol(letters[i].row, col);
      LEDstripArray[idx] = currentColor;
    }

    FastLED.show();
    server.handleClient();   // Web-UI während der Animation erreichbar halten

    if (progress >= 1.0) break;
    delay(FLYIN_STEP_DELAY_MS);
  }

  // Sicherheitshalber am Ende: alle Ziel-Buchstaben (neu + unverändert) final gesetzt
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    LEDstripArray[LEDindicator] = CRGB::Black;
  }
  for (int i = 0; i < persistCount; i++) {
    LEDstripArray[persistIndexList[i]] = currentColor;
  }
  for (int i = 0; i < letterCount; i++) {
    int idx = rainIndexFromRowCol(letters[i].row, (int)letters[i].targetCol);
    LEDstripArray[idx] = currentColor;
  }
  FastLED.show();
}
// ++++++++++ MATRIX FLY-IN (EINBLENDEN) Effekt - ENDE +++++++++++++++


// ++++++++++ MATRIX LETTER-BY-LETTER HIDE (AUSBLENDEN) Effekt - START ++++++
// Zweiter Ausblend-Effekt: schaltet die aktuell angezeigten Buchstaben
// EINZELN aus - allerdings NICHT der Reihe nach (nicht Buchstabe 1, 2, 3...
// eines Wortes), sondern in komplett zufälliger/ungeordneter Reihenfolge
// über ALLE gerade angezeigten Buchstaben hinweg. Die Farbe der jeweils
// noch leuchtenden Buchstaben bleibt dabei unverändert.

#define LETTERHIDE_MAX_LETTERS        40   // Reserve für die längstmögliche Wortkombination
#define LETTERHIDE_STEP_DELAY_MIN_MS   60   // kürzeste Pause zwischen zwei "Abschalt-Schritten"
#define LETTERHIDE_STEP_DELAY_MAX_MS  170   // längste Pause zwischen zwei "Abschalt-Schritten"

// sourceArray: z.B. "shownletter[]" - alle Einträge mit Wert 1 werden
// in zufälliger Reihenfolge einzeln ausgeschaltet.
void matrixLetterByLetterHide(unsigned int *sourceArray) {

  int ledIndexList[LETTERHIDE_MAX_LETTERS];
  int letterCount = 0;

  // alle aktiven (== 1) LED aus dem Quell-Array einsammeln
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    if (sourceArray[LEDindicator] == 1 && letterCount < LETTERHIDE_MAX_LETTERS) {
      ledIndexList[letterCount] = LEDindicator;
      letterCount++;
    }
  }

  if (letterCount == 0) return;

  // Reihenfolge zufällig mischen (Fisher-Yates-Shuffle), damit die
  // Buchstaben NICHT der Reihe nach, sondern ungeordnet verschwinden
  for (int i = letterCount - 1; i > 0; i--) {
    int j = random(0, i + 1);
    int tmp = ledIndexList[i];
    ledIndexList[i] = ledIndexList[j];
    ledIndexList[j] = tmp;
  }

  RGB_RED   = savedRGBRED;
  RGB_GREEN = savedRGBGREEN;
  RGB_BLUE  = savedRGBBLUE;

  CRGB currentColor = CRGB(RGB_RED, RGB_GREEN, RGB_BLUE);

  FastLED.setBrightness(brightnessLED);

  // Startbild: alle aktuell angezeigten Buchstaben unverändert anzeigen
  for (int i = 0; i < letterCount; i++) {
    LEDstripArray[ledIndexList[i]] = currentColor;
  }
  FastLED.show();

  // einen nach dem anderen (in der zufälligen Reihenfolge) ausschalten
  for (int i = 0; i < letterCount; i++) {
    LEDstripArray[ledIndexList[i]] = CRGB::Black;
    FastLED.show();
    server.handleClient();   // Web-UI während der Animation erreichbar halten
    delay(random(LETTERHIDE_STEP_DELAY_MIN_MS, LETTERHIDE_STEP_DELAY_MAX_MS));
  }

  // Sicherheitshalber am Ende alles löschen
  for (LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    LEDstripArray[LEDindicator] = CRGB::Black;
  }
  FastLED.show();
}
// ++++++++++ MATRIX LETTER-BY-LETTER HIDE (AUSBLENDEN) Effekt - ENDE ++++++



// ++++++++++ RUN LED CLOCK - START +++++++++++++
void runClockPlatt(){

  // In jedem LOOP das NEWLETTER Array wieder zurücksetzen und damit für das WRITE-Array initialisieren
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    newletter[LEDindicator] = 0;   
  }

  // ++++++ 1. Wort "IT" ausgeben (immer) ++++++++++
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

  // Check if Night-Switch-Off is active...
  // (korrigierte Logik aus Kölsch/Plattdeutsch - liest außerdem Farbe und
  //  Effekte AN/AUS jedes Mal frisch aus dem EEPROM)
  nightSwitchOffCheck();

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

      if (savedEFFECTSJN) {

        // ++++ Ein-/Ausblend-Effekte AN: zwei feste Effekt-Tupel im Wechsel ++++
        // effectPairToggle == true  -> Fly-Out (Ausblenden) + Fly-In (Einblenden)
        // effectPairToggle == false -> Letter-by-Letter-Hide (Ausblenden) + Rain-In (Einblenden)
        // Der Wechsel erfolgt bei jedem tatsächlichen Anzeigewechsel (i.d.R. alle 5 Minuten).

        if (effectPairToggle) {
          matrixFlyOutHide(shownletter);
          matrixFlyIn(newletter);
        } else {
          matrixLetterByLetterHide(shownletter);
          matrixRainReveal(newletter);
        }

        effectPairToggle = !effectPairToggle;

      } else {

      // ++++ Ein-/Ausblend-Effekte AUS: einfaches Brightness-Fade ++++

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

      } // Ende Effekte AN/AUS

    }

  } else { // Schalte alle LED im STRIP auf den RGB Wert = 0,0,0 

    for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {

      LEDstripArray[LEDindicator].setRGB( 0, 0, 0);

    }

    FastLED.show();

    // FIX: Während der Nachtabschaltung "nichts angezeigt" merken. Vorher wurde
    // auch nachts newletter -> shownletter kopiert. War das erste Wort nach der
    // Nacht identisch mit dem letzten davor (z.B. 05:58 und 06:00 = "SIX O'CLOCK"),
    // hielt die Uhr das für "unverändert" und blieb bis 06:03 dunkel.
    for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
      shownletter[LEDindicator] = 0;
    }
    return;

  } // Ende Abfrage NIGHTMODE

  //Arrays kopieren für den nächsten Vergleich ... 
  // ... wichtig für das FADE OUT / FADE IN --> NUR(!) bei Änderung mindestens eines Zeichens
  for(LEDindicator = ersteLED; LEDindicator <= letzteLED; LEDindicator++) {
    shownletter[LEDindicator] = newletter[LEDindicator];
  } 

}   

#endif

// ++++++++++ RUN LED CLOCK - END +++++++++++++++++++++++++++++++++++++++++++++
