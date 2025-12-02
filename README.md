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

ESP32 TX2 (GPIO17)  →  SV5W RX   (met evt. 1 k in serie)
ESP32 RX2 (GPIO16)  ←  SV5W TX   (via 10 k/20 k spanningsdeler, of een andere waarde 1:2)
GND ↔ GND,  +5 V ↔ VCC

De DY-SV5W werkt intern op 5 V logica.
De ESP32 werkt op 3,3 V logica.
→ Dus:

Van ESP32 → SV5W (TX2 → RX) = veilig (3.3 V is hoog genoeg voor 5 V-logica).

Van SV5W → ESP32 (TX → RX2) = NIET veilig direct, want 5 V kan de ESP32-input beschadigen.
Daarom verlagen we dat signaal met een spanningsdeler.
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
/data/
/include
 ├── BlinkOverlay.h
 ├── Button.h
 ├── Config.h
 ├── LedPwm.h
 ├── LedSet.h
 ├── LightProgram.h
 └── SV5W.h
/src
 ├── Button.cpp
 ├── LedPwm.cpp
 ├── LightProgram.cpp
 ├── main.cpp
 └── Sv5W.cpp
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
   constexpr int LED_COUNT = 4; //aantal leds
   constexpr int LEDC_CH[LED_COUNT] = {0,1,2,3}; //toekenning aan array
   constexpr int PIN_LED[LED_COUNT] = {25,26,27,33}; //daadwerkelijke led pins
   ```
   
2. Voeg eventueel extra scenario-gewichten toe, (1.00f = ja, 0.00f = negeren):

   ```cpp
   constexpr float SCENARIO_NIGHT_WEIGHTS[LED_COUNT] = {0.2f, 0.2f, 0.8f, 0.1f};
   constexpr float LEDSET_BLINK_WEIGHTS[LED_COUNT]   = { 0.0f, 1.0f, 0.0f, 0.0f };
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

---
## Wiring

1. LEDs
```
NPN-transistor (bijv. BC547(CBE) / 2N2222 (EBC))
5V ---/\/\/---|>|----+--- (collector) (220Ω–470Ω)
              LED    |
                     |
                     C NPN transistor
ESP32 PWM ---[1KΩ]---B (1 kΩ basisweerstand)
                     E
                     |
                    GND

MOSFET (bijv. IRLZ44N, AO3400, 2N7002)
5V ---/\/\/---|>|------+--- D (drain)
              LED      |
                       S (source)
                       |
                      GND
ESP32 PWM ----[100Ω]---G (gate)
```

2. componentwaarden
Onderdeel	Waarde	Opmerking
R_LED	220Ω – 470Ω	afhankelijk van LED en gewenste helderheid
R_BASE (transistor)	1 kΩ	beperkt basisstroom tot ±3 mA
R_GATE (MOSFET)	100 Ω	voorkomt oscillatie bij snelle PWM



---

| Onderdeel	             | Aantal | Voorbeeldtype           | Opmerking            |
| ---------------------- | ---------------------------------- | -------------------- |
| ESP32 Dev Kit          | 1      |	DOIT ESP32 DEVKIT V1      | Hoofdcontroller      |
| DY-SV5W Module         | 1      |	DY-SV5W                   | Geluidsmodule (UART-modus)
| LED’s (5 mm / 3 mm)    | 4–8    | willekeurige kleuren	     | 20 mA, max. 2.0–3.3 V per LED
| Weerstanden LED        | 4–8    | 220–470 Ω                 | in serie met elke LED |
| NPN-transistoren       | 4      | BC547 / 2N2222            | schakelen LED’s op 5 V (of gebruik MOSFETs) |
| MOSFETs (alternatief)  | 4      | 2N7002 / AO3400 / IRLZ44N | Logic-level, lage gate threshold |
| Weerstanden basis/gate | 4      | 1 kΩ (transistor) / 100Ω (MOSFET) | aansturing vanaf ESP32-pin |
| 5V voeding             | 1      | 5V DC adapter, 1A         | voor ESP32, DY-SV5W en LED’s |
| Breadboard & jumpers   | -      | -                         | voor prototyping |
| Draden (GND-koppeling) | -      | -                         | verbind alle GND’s! |


-Geel licht 20mA
-Lataarnpaal 10mA p/s
-Flitsen 30mA