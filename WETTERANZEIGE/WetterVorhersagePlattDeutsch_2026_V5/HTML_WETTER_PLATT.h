#ifndef HTML_WETTER_PLATT_H
#define HTML_WETTER_PLATT_H

#include <Arduino.h>

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>MIEN WEDDER</title>
      <style>
      html {
        font-family: Arial, Helvetica, sans-serif;
        text-align: center;
      }
      h1 {
        font-size: 1.8rem;
        color: white;
      }
      h2 {
        font-size: 1.5rem;
        font-weight: bold;
        color: #143642;
        margin-top: 10px;
        margin-bottom: 1px;
        padding: 5px;
      }
      .topnav {
        overflow: hidden;
        background-color: #143642;
      }
      body {
        margin: 0;
      }
      .inhalt {
        padding: 15px;
        max-width: 500px;
        margin: 0 auto;
      }
      .card {
        background-color: #f8f7f9;
        box-shadow: 2px 2px 12px 1px rgba(140, 140, 140, 0.5);
        padding-top: 5px;
        padding-bottom: 15px;
      }

      input[type='text'],
      [type='password'],
      [type='ssid'] {
        height: 30px;
        width: 300px;
        border-radius: 3px;
        border: 1px solid rgb(10, 10, 10);
        margin-top: 5px;
        font-size: 18px;
      }

    input[type='locationPLZ'] {
        height: 30px;
        width: 75px;
        border-radius: 3px;
        border: 1px solid rgb(10, 10, 10);
        /*margin-top: 5px; */
        font-size: 18px;
      }

    input[type='locationNAME'] {
        height: 30px;
        width: 275px;
        border-radius: 3px;
        border: 1px solid rgb(10, 10, 10);
        margin-top: 5px;
        font-size: 18px;
      }

      select {
        height: 25px;
        width: 88px;
        border-radius: 3px;
        border: 1px solid rgb(10, 10, 10);
        margin-top: 5px;
        font-size: 16px;
      }

      select[type='ssid'] {
        height: 30px;
        width: 300px;
        border-radius: 3px;
        border: 1px solid rgb(10, 10, 10);
        margin-top: 5px;
        font-size: 18px;
      }

      .saved {
        height: 20px;
        font-size: 12px;
      }

      .slider {
        height: 35px;
      }
      .passkey {
        margin-top: 5px;
        margin-bottom: 15px;
      }
      /* Abstand zwischen Kennwort-Feld und Checkbox "Kennwort wiesen" -
         die Checkbox ist per transform: scale(1.7) vergrößert und ragt sonst in das Feld */
      .showPasswordContainer {
        margin-top: 18px;
      }

      input[type='checkbox'] {
        transform: scale(1.7);
        padding-top: 10px;
        font-size: 1.2rem;
        font-weight: bold;
        color: #000000;
      }

      label[for='rgbred'] {
        border-radius: 3px;
        border: 1px solid #000;
        background-color: rgb(255, 0, 0);
        padding: 4px 18px 4px 18px;
        font-size: medium;
      }

      label[for='rgbgreen'] {
        border-radius: 3px;
        border: 1px solid #000;
        background-color: rgb(0, 255, 0);
        padding: 4px 7px 4px 7px;
        font-size: medium;
      }

      label[for='rgbblue'] {
        border-radius: 3px;
        border: 1px solid #000;
        background-color: rgb(0, 0, 255);
        padding: 4px 14px 4px 14px;
        font-size: medium;
        color: white;
      }
      label {
        border-radius: 3px;
        color: rgb(0, 0, 0);
        padding: 4px 4px 4px 4px;
        font-size: 16px;
      }

      .colorPreviewStyle {
        display: inline-block;
        width: 253px;
        height: 20px;
        margin-bottom: 10px;
        padding: 0px 0px 8px 0px;
        border-radius: 3px;
        border: 1px solid #000;
      }

      .button {
        padding: 15px 50px;
        font-size: 18px;
        text-align: center;
        outline: none;
        color: #fff;
        background-color: #072c7b;
        border: unset;
        border-radius: 10px;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0, 0, 0, 0);
      }
      button:hover {
        background-color: #0955ee;
      }
      .button:active {
        background-color: #0955ee;
        box-shadow: 2 2px 5px #cdcdcd;
        transform: translateY(2px);
      }
      .state {
        font-size: 0.8em;
        color: #8c8c8c;
        font-weight: bold;
        display: none;
      }
    </style>
</head>
<body>
  <div class="topnav">
    <h1>MIEN WEDDER</h1>
  </div>
     <div class="inhalt">
      <div class="card">
        <h2>Mien Farv:</h2>
        <div id="colorPreview" class="colorPreviewStyle"></div>
        <div class="slider">
          <label for="rgbred">ROT</label>
          <input
            type="range"
            min="0"
            max="255"
            name="rgbred"
            id="rgbred"
            step="1"
            value="%%RED%%"
            oninput="updateInputValue('rgbred');updateColorPreview()"
          />
          <input
            type="number"
            id="rgbredInput"
            min="0"
            max="255"
            value="%%RED%%"
            oninput="updateSliderValue('rgbred');updateColorPreview()"
          />
        </div>

        <div class="slider">
          <label for="rgbgreen">GREUN</label>
          <input
            type="range"
            min="0"
            max="255"
            name="rgbgreen"
            id="rgbgreen"
            step="1"
            value="%%GREEN%%"
            oninput="updateInputValue('rgbgreen');updateColorPreview()"
          />
          <input
            type="number"
            id="rgbgreenInput"
            min="0"
            max="255"
            value="%%GREEN%%"
            oninput="updateSliderValue('rgbgreen');updateColorPreview()"
          />
        </div>

        <div class="slider">
          <label for="rgbblue">BLAU</label>
          <input
            type="range"
            min="0"
            max="255"
            name="rgbblue"
            id="rgbblue"
            step="1"
            value="%%BLUE%%"
            oninput="updateInputValue('rgbblue');updateColorPreview()"
          />
          <input
            type="number"
            id="rgbblueInput"
            min="0"
            max="255"
            value="%%BLUE%%"
            oninput="updateSliderValue('rgbblue');updateColorPreview()"
          />
        </div>
        <h2>Mien Netzwark + Kennwort:</h2>
        <div class="networks">
          <select 
            type="ssid"
            name="ssid"
            id="ssid">
          </select>
        </div>
        <div class="passkey">
          <input
            type="password"
            name="password"
            id="password"
            maxlength="64"
            placeholder=" ... dien Kennwort ingaven ..."
          />
          <div class="showPasswordContainer">
            <input type="checkbox" id="showPassword" onclick="togglePassword()" />
            <label for="showPassword"> Kennwort wiesen</label>
          </div>
        </div>
        <h2>Mien Urtschaft (PLZ + Name):</h2>
        <div class="locationPLZ">
            <input
            type="locationPLZ"
            name="locationPLZ"
            id="locationPLZ"
            maxlength="8"
            placeholder=" PLZ "
          />
          <input
            type="locationName"
            name="locationName"
            id="locationName"
            maxlength="60"
            placeholder=" ... dien Urtschaft ..."
          />
        </div>
        <div class="nightModeContainer">
          <input type="checkbox" id="nightModeJN" name="nightModeJN" />
          <label for="nightModeJN"> Klock ut - vun: </label>
          <select id="nightModeStart">
            <option value="21">21:00</option>
            <option value="22">22:00</option>
            <option value="23">23:00</option>
            <option value="0" selected>00:00</option>
            <option value="1">01:00</option>
            <option value="2">02:00</option>
          </select>
          <label for="nightModeJN"> bis: </label>
          <select id="nightModeEnd">
            <option value="4">04:00</option>
            <option value="5">05:00</option>
            <option value="6" selected>06:00</option>
            <option value="7">07:00</option>
            <option value="8">08:00</option>
            <option value="9">09:00</option>
          </select>
        </div>
          <p><button id="updateClock" class="button" onclick='saveSettings()'>mok update</button></p>
          <label id="saved">...</label>
      </div>
  <div class="inhalt">
    <div class="card">
      <p><button id="startClock" class="button" onclick='startClock()'>start wedder</button></p>
      <label id="startet">...</label>
    </div>
    <div>
      <h3>WLAN-Historie (Verbindungsabbrüche)</h3>
      %%WIFILOG%%
    </div>
  </div>
  
  <script>

    var networks = %%NETWORKS%%;            // JSON Array vom ESP32

    function generateOptions(networkArray) {
      var select = document.getElementById('ssid');
      select.innerHTML = "";
      networkArray.forEach(function(ssid) {
        var option = document.createElement('option');
        option.value = ssid;
        option.textContent = ssid;
        select.appendChild(option);
      });
    }

    generateOptions(networks);

    // alle weiteren Werte aus EEPROM einsetzen:
    var savedPASSWORD = %%PASSWORD%%;
    var savedLOCATION_PLZ = %%LOCATION_PLZ%%;
    var savedLOCATION_NAME = %%LOCATION_NAME%%;
    var savedRGB_RED = %%RED%%;
    var savedRGB_GREEN = %%GREEN%%;
    var savedRGB_BLUE = %%BLUE%%;
    var savedNIGHT_JN = %%NIGHTJN%%;
    var savedNIGHT_START = %%NIGHTSTART%%;
    var savedNIGHT_END = %%NIGHTEND%%;

    document.getElementById('password').value = savedPASSWORD;

    document.getElementById('locationPLZ').value = savedLOCATION_PLZ;
    document.getElementById('locationName').value = savedLOCATION_NAME;

    document.getElementById('rgbred').value = savedRGB_RED;
    document.getElementById('rgbgreen').value = savedRGB_GREEN;
    document.getElementById('rgbblue').value = savedRGB_BLUE;

    document.getElementById('nightModeJN').checked = savedNIGHT_JN;
    document.getElementById('nightModeStart').value = savedNIGHT_START;
    document.getElementById('nightModeEnd').value = savedNIGHT_END;

    // Farbvorschau und Zahlenfelder gleich beim Laden mit den gespeicherten Werten füllen
    updateColorPreview();

    // (10) Kennwort ein-/ausblenden
    function togglePassword() {
      var pw = document.getElementById('password');
      pw.type = document.getElementById('showPassword').checked ? 'text' : 'password';
    }


    // Aktualisiert den Wert des Eingabefelds, wenn der Slider bewegt wird
    function updateInputValue(colorId) {
      const slider = document.getElementById(colorId);
      const input = document.getElementById(colorId + 'Input');
      input.value = slider.value;
    }

    // Aktualisiert den Slider-Wert, wenn das Eingabefeld geändert wird
    function updateSliderValue(colorId) {
      const input = document.getElementById(colorId + 'Input');
      const slider = document.getElementById(colorId);

      // Begrenzung auf gültige Werte (0–255)
      if (input.value < 0) input.value = 0;
      if (input.value > 255) input.value = 255;

      slider.value = input.value;
    }

    function updateColorPreview() {
      // Werte von den Slidern lesen
      const red = document.getElementById('rgbred').value;
      const green = document.getElementById('rgbgreen').value;
      const blue = document.getElementById('rgbblue').value;

      // Vorschau aktualisieren
      const colorPreview = document.getElementById('colorPreview');
      colorPreview.style.backgroundColor = `rgb(${red}, ${green}, ${blue})`;

      // Synchronisiere die Eingabefelder
      document.getElementById('rgbredInput').value = red;
      document.getElementById('rgbgreenInput').value = green;
      document.getElementById('rgbblueInput').value = blue;
    }

    // ++++++++++ SAVE all settings +++++++++


    function saveSettings()  {

      var saveObjectJSON = {
        rgb_red: Number(document.getElementById('rgbredInput').value),
        rgb_green: Number(document.getElementById('rgbgreenInput').value),
        rgb_blue: Number(document.getElementById('rgbblueInput').value),
        ssid: document.getElementById('ssid').value,
        password: document.getElementById('password').value,
        locationPLZ: document.getElementById('locationPLZ').value,
        locationName: document.getElementById('locationName').value,        
        nightJN: document.getElementById('nightModeJN').checked,
        nightStart: Number(document.getElementById('nightModeStart').value),
        nightEnd: Number(document.getElementById('nightModeEnd').value),
        clockStart: false,
      };

      var postJSON = new XMLHttpRequest();
      postJSON.open("POST", "/save", true);
      postJSON.setRequestHeader("Content-Type", "application/json;charset=UTF-8");

      postJSON.onreadystatechange = function() {
        if(postJSON.readyState === XMLHttpRequest.DONE) {
          document.getElementById('saved').innerText = (postJSON.status === 200)
            ? "Diene Instellingen weer nu spiekert."
            : "Fehler beim Spiekern (" + postJSON.status + ")";
        }
      };

      postJSON.send(JSON.stringify(saveObjectJSON));
           
    }

    // ++++++++++ START Clock +++++++++
    function startClock() {

      var startObjectJSON = {
        rgb_red: Number(document.getElementById('rgbredInput').value),
        rgb_green: Number(document.getElementById('rgbgreenInput').value),
        rgb_blue: Number(document.getElementById('rgbblueInput').value),
        ssid: document.getElementById('ssid').value,
        password: document.getElementById('password').value,
        locationPLZ: document.getElementById('locationPLZ').value,
        locationName: document.getElementById('locationName').value,            
        nightJN: document.getElementById('nightModeJN').checked,
        nightStart: Number(document.getElementById('nightModeStart').value),
        nightEnd: Number(document.getElementById('nightModeEnd').value),
        clockStart: true,
      };

      var postJSON = new XMLHttpRequest();
      postJSON.open("POST", "/save", true);
      postJSON.setRequestHeader("Content-Type", "application/json;charset=UTF-8");

      postJSON.onreadystatechange = function() {
        if(postJSON.readyState === XMLHttpRequest.DONE) {
          document.getElementById('startet').innerText = (postJSON.status === 200)
            ? "Diene Uhr geiht nu los ..."
            : "Fehler beim Starten (" + postJSON.status + ")";
        }
      };

      postJSON.send(JSON.stringify(startObjectJSON));

    } 

  </script>
</body>
</html>
)rawliteral";

#endif


