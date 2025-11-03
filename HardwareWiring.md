## Hardware-aansluitschema – ESP32 + DY-SV5W + LED-transistoren

### Overzicht

Dit project gebruikt:

* **ESP32 Dev Kit**
* **DY-SV5W** (UART-voice playback module)
* **3.3V GPIO-signalen** voor besturing
* **5V voeding** voor de DY-module en LED’s
* **Transistoren of MOSFETs** voor het schakelen van LED’s via PWM

---

## DY-SV5W ↔ ESP32-verbinding

| DY-SV5W Pin | Verbinden met ESP32 | Opmerking                    |
| ----------- | ------------------- | ---------------------------- |
| TXD (IO0)   | GPIO 16 (RX2)       | UART2 RX                     |
| RXD (IO1)   | GPIO 17 (TX2)       | UART2 TX                     |
| BUSY        | GPIO 4              | Input (LOW tijdens afspelen) |
| VCC         | 5 V                 | Voeding module               |
| GND         | GND                 | Gezamenlijke massa           |

> **Let op:** DIP-switchinstelling voor UART:
> `SW1=OFF`, `SW2=OFF`, `SW3=ON`

---

## LED-aansluitingen via transistor of MOSFET

Omdat de ESP32 slechts ~12 mA per pin kan leveren, worden de LED’s aangestuurd via **NPN-transistoren** of **N-kanaal MOSFETs**.

### Basisprincipe

* De ESP32 stuurt een **PWM-signaal (3.3 V)** naar de **basis/gate** van de transistor.
* De LED’s (met serieweerstand) hangen aan **+5 V**.
* De transistor schakelt de verbinding naar **GND**.

#### NPN-transistor (bijv. BC547 / 2N2222)

```
5V  ---/\/\/---|>|---+--- (collector)
                LED   |
                     |
                     C  NPN transistor
ESP32 PWM ---/\/\/---B   (1 kΩ basisweerstand)
                      E
                      |
                     GND
```

#### MOSFET (bijv. IRLZ44N, AO3400, 2N7002)

```
5V ---/\/\/---|>|---+--- D (drain)
                LED   |
                      S (source)
                      |
                     GND
ESP32 PWM ----[100Ω]----G (gate)
```

### Componentwaarden

| Onderdeel               | Waarde                      | Opmerking                                  |
| ----------------------- | --------------------------- | ------------------------------------------ |
| **R_LED**               | 220 Ω – 470 Ω               | afhankelijk van LED en gewenste helderheid |
| **R_BASE (transistor)** | 1 kΩ                        | beperkt basisstroom tot ±3 mA              |
| **R_GATE (MOSFET)**     | 100 Ω                       | voorkomt oscillatie bij snelle PWM         |

---

## Meerdere LED-kanalen

In `Config.h` stel je per LED in:

```cpp
constexpr int LED_COUNT = 4;
constexpr int PIN_LED[LED_COUNT] = {25, 26, 27, 33};
constexpr int LEDC_CH[LED_COUNT] = {0, 1, 2, 3};
```

Elke LED (of LED-stripkanaal) krijgt z’n eigen transistor/mosfet.
Je mag verschillende kleuren of secties gebruiken — bijvoorbeeld:

* LED 1 = koud wit
* LED 2 = warm wit
* LED 3 = blauw
* LED 4 = amber

De **scenario-weights** bepalen dan hoe sterk elk kanaal meedoet.

---

## Voedingsadvies

| Lijn        | Spanning | Tip                                           |
| ----------- | -------- | --------------------------------------------- |
| ESP32 VIN   | 5 V      | voeding via USB of externe adapter            |
| DY-SV5W VCC | 5 V      | uit dezelfde voeding, max. ~100 mA            |
| LED’s       | 5 V      | afhankelijk van stroom — reken ~20 mA per LED |
| GND         | Gedeeld  | **Altijd gemeenschappelijke massa!**          |

---

## Kort samengevat

DY-SV5W → UART (RX2/TX2)
BUSY → GPIO 4
LEDs → via transistor/MOSFET naar 5 V
PWM → vanuit ESP32 LEDC-kanalen
GND → gedeeld tussen alle onderdelen


## Componentenlijst (BOM)

| Onderdeel                  | Aantal | Voorbeeldtype                      | Opmerking                                   |
| -------------------------- | ------ | ---------------------------------- | ------------------------------------------- |
| **ESP32 Dev Kit**          | 1      | DOIT ESP32 DEVKIT V1               | Hoofdcontroller                             |
| **DY-SV5W Module**         | 1      | DY-SV5W                            | Geluidsmodule (UART-modus)                  |
| **LED’s (5 mm / 3 mm)**    | 4–8    | willekeurige kleuren               | 20 mA, max. 2.0–3.3 V per LED               |
| **Weerstanden LED**        | 4–8    | 220–470 Ω                          | in serie met elke LED                       |
| **NPN-transistoren**       | 4      | BC547 / 2N2222                     | schakelen LED’s op 5 V (of gebruik MOSFETs) |
| **MOSFETs (alternatief)**  | 4      | 2N7002 / AO3400 / IRLZ44N          | Logic-level, lage gate threshold            |
| **Weerstanden basis/gate** | 4      | 1 kΩ (transistor) / 100 Ω (MOSFET) | aansturing vanaf ESP32-pin                  |
| **5V voeding**             | 1      | 5V DC adapter, 1A                  | voor ESP32, DY-SV5W en LED’s                |
| **Breadboard & jumpers**   | -      | -                                  | voor prototyping                            |
| **Draden (GND-koppeling)** | -      | -                                  | verbind alle GND’s!                         |

> **Tip:** gebruik bij grotere LED-belastingen (>100 mA) MOSFETs i.p.v. NPN-transistoren voor minder spanningsverlies en warmteontwikkeling.

---

## Blokdiagram

```
      +------------------+            +------------------+
      |      ESP32       |  UART2     |     DY-SV5W      |
      |                  |<---------> |   (Voice Mod.)   |
      |  GPIO16 (RX2)    |            |  IO0=TXD, IO1=RXD|
      |  GPIO17 (TX2)    |            |  BUSY → GPIO4    |
      |  GPIO4  (BUSY in)| <----------+                  |
      |                  |            |                  |
      |  PWM CH0..N ----+-----> Gate/Basis (per LED kanaal)
      |                  |            |
      +------------------+            +------------------+
                 |                                 |
                 |                                 |
                 v                                 v
           +-----------+                      +----------+
           | N-MOSFET  |--- LED + R_LED ---+  |   5V     |
           +-----------+                    |  |  Supply |
                |                           |  +----------+
               GND -------------------------+         
```

---

## Bedradingstabel (overzicht)

### UART & status

| Verbinding | ESP32  | DY-SV5W   | Opmerking                           |
| ---------- | ------ | --------- | ----------------------------------- |
| RX2        | GPIO16 | IO0 (TXD) | Seriële ontvangst ESP32             |
| TX2        | GPIO17 | IO1 (RXD) | Seriële zenden ESP32                |
| BUSY       | GPIO4  | BUSY      | LOW tijdens afspelen (INPUT_PULLUP) |
| GND        | GND    | GND       | Gezamenlijke massa                  |
| VCC        | 5V     | VCC       | 5 V voeding voor module             |

### Knoppen

| Functie | ESP32 GPIO               | Aansluiting          | Opmerking            |
| ------- | ------------------------ | -------------------- | -------------------- |
| NEXT    | Config::PIN_BTN_NEXT     | Naar GND bij drukken | INPUT_PULLUP in code |
| PREV    | Config::PIN_BTN_PREV     | Naar GND bij drukken | INPUT_PULLUP         |
| VOL+    | Config::PIN_BTN_VOL_UP   | Naar GND bij drukken | INPUT_PULLUP         |
| VOL–    | Config::PIN_BTN_VOL_DOWN | Naar GND bij drukken | INPUT_PULLUP         |

### LED-kanalen (per kanaal i)

Gebruik de arrays uit `Config.h`:

* Pin: `PIN_LED[i]`
* PWM-kanaal: `LEDC_CH[i]`

| Kanaal i | ESP32 pin `PIN_LED[i]` | Component         | Aansluiting                          | Waarde    |
| -------- | ---------------------- | ----------------- | ------------------------------------ | --------- |
| 0..N     | bv. 25 / 26 / 27 / 33  | **MOSFET** gate   | Via **R_GATE** naar ESP32 PWM        | 100 Ω     |
| 0..N     | —                      | **MOSFET** drain  | Naar LED– (anode via R_LED naar +5V) | —         |
| 0..N     | —                      | **MOSFET** source | Naar **GND**                         | —         |
| 0..N     | —                      | **LED** anode     | Naar **+5V via R_LED**               | 220–470 Ω |
| 0..N     | —                      | **LED** kathode   | Naar **MOSFET drain**                | —         |

> Alternatief met **NPN-transistor**:
>
> * ESP32 PWM → **R_BASE 1 kΩ** → **B** (basis)
> * **E** (emitter) → **GND**
> * **C** (collector) → LED– (LED+ via **R_LED** naar **+5 V**)

### Voeding

| Lijn  | Advies                                          |
| ----- | ----------------------------------------------- |
| +5 V  | DY-SV5W + LED’s (voldoende stroom)              |
| 3.3 V | Alleen ESP32 (niet voor LED’s)                  |
| GND   | Alle GND’s koppelen (ESP32, DY-SV5W, LED power) |

---

## Testvolgorde

1. Verbind alleen **ESP32 ↔ DY-SV5W** en controleer `queryVersion()` output in de seriële monitor.
2. Sluit **één** LED-kanaal met MOSFET aan, test PWM met `ProgDay`.
3. Breid uit naar meerdere kanalen en pas **scenario-weights** aan in `Config.h`.
4. Controleer thermische belasting bij hogere LED-stromen (gebruik MOSFET met lage Rds(on)).
