// --- file: README.md
# ESP32 + DY-SV5W + LED PWM (Onweer/Dag)


Deze repo bevat herbruikbare libraries en een voorbeeld voor:
- **SV5W** UART-audio (DY-SV5W module)
- **LED PWM** via `ledc` (instelbare freq/res)
- **Lichtprogramma’s** (Onweer/Dag) als niet-blokkerende state machines
- **Buttons** (debounce + edge events)


## Structuur (aanbevolen)
```
project/
├─ platformio.ini
├─ lib/
│ ├─ Button/
│ │ ├─ Button.h
│ │ └─ Button.cpp
│ ├─ LedPwm/
│ │ ├─ LedPwm.h
│ │ └─ LedPwm.cpp
│ ├─ LightProgram/
│ │ ├─ LightProgram.h
│ │ └─ LightProgram.cpp
│ └─ SV5W/
│ ├─ SV5W.h
│ └─ SV5W.cpp
└─ src/
├─ Config.h
└─ main.ino (onze example_usage.ino)
```


> **Tip:** In PlatformIO mag het bestand `main.ino` heten en in `src/` liggen.


## Configuratie
Alle instelbare waarden (pinnen, UART, volume, tracks, LEDC-frequentie/resolutie) staan in **`src/Config.h`**.


Belangrijke items:
- `Config::UART_RX_PIN`, `Config::UART_TX_PIN`, `Config::UART_BAUD`
- `Config::TRACK_THUNDER`, `Config::TRACK_DAY`, `Config::VOLUME_DEFAULT`, `Config::PIN_BUSY`
- `Config::PIN_BTN_NEXT`, `Config::PIN_BTN_PREV`
- `Config::LEDC_CH1`, `Config::LEDC_CH2`, `Config::PIN_LED_CH1`, `Config::PIN_LED_CH2`, `Config::LEDC_FREQ`, `Config::LEDC_RES_BITS`


## Hardware quickstart
- DIP: **1=OFF, 2=OFF, 3=ON** (UART-Mode)
- SV5W **TXD/IO0 → ESP32 RX**, **RXD/IO1 ← ESP32 TX**, **BUSY → ESP32 input (pull-up)**
- **GNDs koppelen**. SV5W voeden met **5V** (gedeelde massa met ESP32).
- LEDs via N-MOSFET low-side (AO3400A/Si2302), serieweerstand per LED, PWM via `ledc`.


## Build & Flash (PlatformIO)
```sh
# in de projectmap
pio run -t upload
pio device monitor -b 115200
```


## Veelvoorkomende issues
- **`Serial2` undefined**: we definiëren `HardwareSerial Serial2(2);` in `main.ino`. Laat die regel staan als jouw core `Serial2` niet aanbiedt.
- **Geen audio**: check 9600-8N1, UART-wiring (RX↔TX), DIP-stand, juiste **track index** (bijv. `00001.mp3`).
- **LED knippert niet**: check `Config::LEDC_FREQ`/`LEDC_RES_BITS`, PWM-pin mapping en transistor-schema.