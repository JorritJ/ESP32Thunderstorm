// --- file: README.md
# ESP32 + DY-SV5W Voice Module Project

### Licht- en geluidsimulatie met scenario’s (Onweer / Dag)

Dit project combineert een **ESP32** met de **DY-SV5W voice playback module**.
Het systeem speelt geluiden af via UART en bestuurt meerdere **PWM-LED’s** voor dynamische lichteffecten.
Er zijn momenteel twee scenario’s:

* **Onweer** → willekeurige flitsen (met subflitsen, nagloei, en kanaalverschuiving)
* **Daglicht** → langzaam fonkelende lichten

Het geheel is non-blocking, reageert continu op knoppen en kan eenvoudig worden uitgebreid.

---

## Hardwareoverzicht

| Component                            | Beschrijving                                         |
| ------------------------------------ | ---------------------------------------------------- |
| **ESP32 Dev Board**                  | Bestuurt alles via UART en PWM                       |
| **DY-SV5W Module**                   | Speelt MP3’s af (UART-modus, DIP 1-OFF, 2-OFF, 3-ON) |
| **LED’s (max. 4 in huidige config)** | Worden via transistor/MOSFET aangestuurd op 5 V      |
| **Knoppen**                          | NEXT / PREV / VOL+ / VOL–                            |
| **BUSY-pin**                         | Ingang naar ESP32 (laag tijdens afspelen)            |

### Basisverbindingen

| DY-SV5W pin | ESP32 pin (aanpasbaar in `Config.h`) |
| ----------- | ------------------------------------ |
| TXD (IO0)   | GPIO 16 (RX2)                        |
| RXD (IO1)   | GPIO 17 (TX2)                        |
| BUSY        | GPIO 4                               |
| VCC         | 5 V                                  |
| GND         | GND                                  |

---

## Installatie & Build

### 1. PlatformIO setup

**`platformio.ini`** voorbeeld:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags = -DCORE_DEBUG_LEVEL=0
```

### 2. Mappenstructuur

```
/lib
 ├── Button/
 ├── LedPwm/
 ├── LedSet/
 ├── LightProgram/
 ├── SV5W/
 └── Config.h
/src
 └── example_usage.ino
```

### 3. Build & upload

```bash
pio run -t upload
pio device monitor
```

---

## Software-architectuur

| Module                    | Functie                                                                                 |
| ------------------------- | --------------------------------------------------------------------------------------- |
| **Button**                | Debounced knoppen met `consumePressed()`-logica                                         |
| **LedPwmChannel**         | Beheert één PWM-kanaal (frequentie, resolutie, duty)                                    |
| **LedSet**                | Groepeert meerdere `LedPwmChannel`s en weegt per kanaalintensiteit                      |
| **ProgThunder / ProgDay** | Scenario’s die LED’s moduleren                                                          |
| **SV5W**                  | UART-interface voor DY-SV5W, inclusief wrapper voor commando’s, volume, en play-by-path |
| **Config.h**              | Centrale pin-, kanaal- en scenario-instellingen                                         |

---

## Scenario’s

### Onweer (ProgThunder)

* Willekeurige burstintervallen (kort en clusterachtig)
* Preglow, 1-4 subflitsen met intensiteitsvariatie
* Kanaalverschuiving (8-25 ms)
* Nagloei (70-150 ms)
* LED-gewichten (`SCENARIO_THUNDER_WEIGHTS`) bepalen welke LED’s het meest oplichten

### Dag (ProgDay)

* Langzame sinusgolf-modulatie van helderheid
* Willekeurige sparkles
* LED-gewichten (`SCENARIO_DAY_WEIGHTS`) definiëren basiskleuren

---

## Bedieningsknoppen

| Knop            | Actie                               |
| --------------- | ----------------------------------- |
| **NEXT**        | Volgende modus (Onweer → Dag)       |
| **PREV**        | Vorige modus                        |
| **VOL+ / VOL–** | Volume aanpassen (met bounds-check) |

---

## SV5W-functionaliteit

De `SV5W`-klasse ondersteunt:

| Functie                                      | Beschrijving                        |
| -------------------------------------------- | ----------------------------------- |
| `playTrack(index)`                           | Speelt een specifiek MP3-bestand af |
| `stop()`, `pause()`, `next()`                | Basisbediening                      |
| `setVolume(vol)`                             | Volume 0–30                         |
| `volumeUpCmd()` / `volumeDownCmd()`          | Directe increment/decrement         |
| `playByPath(drive, "\\FOLDER\\00001.MP3")`   | Afspelen via bestandsnaam           |
| `queryVersion()`                             | Firmware-info uitlezen              |
| `playRandomFolderClip("\\FOLDER", maxIndex)` | Speelt willekeurige clip uit map    |

---

## LEDs uitbreiden

1. In `Config.h`:

   ```cpp
   constexpr int LED_COUNT = 4;
   constexpr int LEDC_CH[LED_COUNT] = {0,1,2,3};
   constexpr int PIN_LED[LED_COUNT] = {25,26,27,33};
   ```
2. Voeg eventueel extra scenario-gewichten toe:

   ```cpp
   constexpr float SCENARIO_NIGHT_WEIGHTS[LED_COUNT] = {0.2f, 0.2f, 0.8f, 0.1f};
   ```
3. Maak een nieuwe `LedSet` en `LightProgram`-klasse (zoals ProgDay) voor jouw scenario.

---

## Tips & Troubleshooting

* Als je compile-fout krijgt met `Serial2`: verwijder `HardwareSerial Serial2(2);` (ESP32-core levert dat al).
* `vector`-gerelateerde fouten → zorg dat `<vector>` **boven** `<Arduino.h>` staat.
* Controleer dat je **DIP-switches correct** staan voor UART: 1 = OFF, 2 = OFF, 3 = ON.
- **Geen audio**: check 9600-8N1, UART-wiring (RX↔TX), DIP-stand, juiste **track index** (bijv. `00001.mp3`).
* Als PWM niet werkt: check of je pinnen **LEDC-compatibel** zijn (niet allemaal zijn dat op elke ESP32).
 **LED knippert niet**: check `Config::LEDC_FREQ`/`LEDC_RES_BITS`, PWM-pin mapping en transistor-schema.

---

## Verdere uitbreidingen (nog te doen)

* Nieuwe scenario’s (bijv. *Nacht*, *Alarm*, *Regenboog*)
* Fade-overgangen tussen modi
* Audio-sync via BUSY-pin
* Externe configuratie via webinterface of Bluetooth

-