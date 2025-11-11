// --- file: example_usage.ino
#include <Arduino.h>
#include <HardwareSerial.h>
#include "SV5W.h"
#include "Button.h"
#include "LedPwm.h"
#include "LightProgram.h"
#include "Config.h"
#include "LedSet.h"

/*
// ======= CONDITIONELE INCLUDES =======
#ifdef HARDWARE_A
  #include <SensorA.h>
#elif defined(HARDWARE_B)
  #include <SensorB.h>
#else
  #error "Geen hardwaretype gedefinieerd! Gebruik HARDWARE_A of HARDWARE_B."
#endif
*/


// ===================== Serial2 definitie (indien core het niet aanbiedt) =====================
//HardwareSerial Serial2(2);   // gebruik UART2 (hardware poort 2)

// (Alle pinnen/parameters komen uit Config.h)

//LedPwmChannel led1(Config::LEDC_CH1, Config::PIN_LED_CH1, Config::LEDC_FREQ, Config::LEDC_RES_BITS);
//LedPwmChannel led2(Config::LEDC_CH2, Config::PIN_LED_CH2, Config::LEDC_FREQ, Config::LEDC_RES_BITS);
static LedPwmChannel* LEDS[Config::LED_COUNT];

// ===================== Buttons =====================
Button btnNext{Config::PIN_BTN_NEXT,true};
Button btnPrev{Config::PIN_BTN_PREV,true};

Button btnVolUp(Config::PIN_BTN_VOL_UP, true);
Button btnVolDown(Config::PIN_BTN_VOL_DOWN, true);

// ===================== DY-SV5W =====================
SV5W sv5w;
constexpr uint16_t TRACK_THUNDER = 1; // pas aan naar jouw index/bestandsnaam
constexpr uint16_t TRACK_DAY = 2; // pas aan naar jouw index/bestandsnaam

// --- SV5W BUSY monitoring (active LOW)
static bool busyState = false;               // gefilterde/stabiele staat
static bool busyRaw   = false;               // laatst gemeten raw
static uint32_t busyLastEdgeMs = 0;          // laatste raw edge
static const uint32_t BUSY_DEBOUNCE_MS = 5;  // kleine debounce tegen rammel
inline bool sv5wBusyRaw() { return digitalRead(Config::PIN_BUSY) == LOW; } // LOW = playing

// ===================== Lichtprogramma's =====================
//ProgThunder progThunder(led1, led2);
ProgThunder progThunder(*(new LedSet(LEDS, Config::LED_COUNT, Config::SCENARIO_THUNDER_WEIGHTS)), Config::SCENARIO_THUNDER_WEIGHTS);
//ProgDay progDay(led1, led2);
ProgDay progDay(*(new LedSet(LEDS, Config::LED_COUNT, Config::SCENARIO_DAY_WEIGHTS)), Config::SCENARIO_DAY_WEIGHTS);

enum class Mode
{
  Thunder = 0,
  Day = 1,
  COUNT
};
Mode currentMode = Mode::Thunder;
LightProgram *currentProg = nullptr;

void startMode(Mode m, uint32_t now)
{
  currentMode = m;
  sv5w.stop();
  delay(10);
  switch (m)
  {
  case Mode::Thunder:
    sv5w.playTrack(TRACK_THUNDER);
    //sv5w.playByPath(0x01, "\\THUNDER\\00001.MP3"); // drive: 0x01 = SD
    //sv5w.playByPath("\\THUNDER\\00001.MP3"); // gebruikt default drive //Zorg dat de backslash-escapes kloppen in C++-strings (\"\\\\THUNDER\\\\00001.MP3\" in code).
    //sv5w.playRandomFolderClip("\\THUNDER", /*maxIndex=*/20); //speel random clip uit map THUNDER met bestanden 00001.MP3 .. 00020.MP3
    break;
  case Mode::Day:
    sv5w.playTrack(TRACK_DAY);
    break;
  default:
    break;
  }
  switch (m)
  {
  case Mode::Thunder:
    currentProg = &progThunder;
    break;
  case Mode::Day:
    currentProg = &progDay;
    break;
  default:  //Welke modus dan ook, fallback naar Thunder
    currentProg = &progThunder;
    break;
  }
  if (currentProg)
    currentProg->start(now);
}
void nextMode(uint32_t now)
{
  int n = (int)Mode::COUNT;
  int cur = (int)currentMode;
  cur = (cur + 1) % n;
  startMode((Mode)cur, now);
}
void prevMode(uint32_t now)
{
  int n = (int)Mode::COUNT;
  int cur = (int)currentMode;
  cur = (cur - 1 + n) % n;
  startMode((Mode)cur, now);
}

void setup()
{
  Serial.begin(115200);
  Serial.println(F("Thunderstorm Light Program starting..."));
  pinMode(Config::PIN_BUSY, INPUT_PULLUP);
  btnNext.begin();
  btnPrev.begin();
  
  //sv5w BUSY pin initialiseren
  busyRaw   = sv5wBusyRaw();
  busyState = busyRaw;
  Serial.printf("SV5W BUSY init: %s (active LOW)\n", busyState ? "ACTIVE (playing)" : "IDLE");

  // LED kanalen initialiseren
  //led1.begin(); 
  //led2.begin();
  //led1.setDuty(0); 
  //led2.setDuty(0);
  for (int i = 0; i < Config::LED_COUNT; ++i) {
    LEDS[i] = new LedPwmChannel(Config::LEDC_CH[i], Config::PIN_LED[i],
                                Config::LEDC_FREQ, Config::LEDC_RES_BITS);
    LEDS[i]->begin();
    LEDS[i]->setDuty(0);
  }

  // LedSet voor elk scenario (weights komen uit Config)
  static LedSet thunderSet(LEDS, Config::LED_COUNT, Config::SCENARIO_THUNDER_WEIGHTS);
  static LedSet daySet(LEDS,     Config::LED_COUNT, Config::SCENARIO_DAY_WEIGHTS);

  // Programma-instanties met hun set/weights
  static ProgThunder progThunder(thunderSet, Config::SCENARIO_THUNDER_WEIGHTS);
  static ProgDay     progDay(daySet,         Config::SCENARIO_DAY_WEIGHTS);
  // ... bewaar referenties in globale pointers als je dat al deed:
  // currentProg = &progThunder / &progDay tijdens startMode(...)

  
  // SV5W init
  sv5w.begin(Serial2, Config::UART_RX_PIN, Config::UART_TX_PIN, Config::UART_BAUD);
  sv5w.setVolume(Config::VOLUME_DEFAULT);
  // Kies desgewenst standaard-drive (0x00=USB, 0x01=SD, 0x02=FLASH)
  sv5w.setDefaultDrive(0x01);

// (Optioneel) huidige afspeeldrive uitlezen (test communicatie)
  auto playdrive = sv5w.queryCurrentPlayDrive();
  if (playdrive.valid) {
    Serial.print(F("SV5W Current Play Drive: "));
    for (auto b : playdrive.data) { Serial.printf(" %02X", b); }
    Serial.println();
  } else {
    Serial.println(F("SV5W Current Play Drive query failed."));
  }

  /*
    De meeste ESP32-boards (zoals de “ESP32 Dev Module” of “DOIT ESP32 DEVKIT V1”) hebben: HardwareSerial Serial2(2); al voor je gedefinieerd.
    Maar sommige (bijv. ESP32-S3, ESP32-C3 of een minimal core in PlatformIO) doen dat niet automatisch.
  */
  // (Optioneel) firmwareversie uitlezen
  auto ver = sv5w.queryVersion();
  if (ver.valid) {
    Serial.print(F("SV5W Version bytes:"));
  for (auto b : ver.data) { Serial.printf(" %02X", b); }
    Serial.println();
  } else {
    Serial.println(F("SV5W Version query failed."));
  }

  uint32_t now = millis();
  if (btnNext.readRaw())  {
    startMode(Mode::Day, now);
    Serial.println(F("Start in DAY mode (NEXT ingedrukt)."));
  }
  else
  {
    startMode(Mode::Thunder, now);
    Serial.println(F("Start in THUNDER mode (standaard)."));
  }
    
  Serial.println(F("Klaar. NEXT/PREV voor moduswissel."));
}

void loop()
{
  static uint8_t volume = Config::VOLUME_DEFAULT;
  uint32_t now = millis();

  btnNext.update(now);
  btnPrev.update(now);
  btnVolUp.update(now);
  btnVolDown.update(now);

  // --- SV5W BUSY monitoring met debounce
  bool r = sv5wBusyRaw();
  if (r != busyRaw) {
    busyRaw = r;
    busyLastEdgeMs = now; // start debounce
  }
  if (r != busyState && (now - busyLastEdgeMs) >= BUSY_DEBOUNCE_MS) {
    busyState = r;
    Serial.printf("[%lu ms] SV5W BUSY %s\n",
                  (unsigned long)now,
                  busyState ? "ACTIVE (playing)" : "IDLE");
  }

  if (btnNext.consumePressed()) {
    Serial.println(F("Modus wisselen: NEXT"));
    nextMode(now);
  }
    
  if (btnPrev.consumePressed()) {
    Serial.println(F("Modus wisselen: PREV"));  
    prevMode(now);
  }
  //Volume bediening (optioneel) uitgezet ivm storing/debounce
  /*
  if (btnVolUp.consumePressed()){
    if(volume < Config::VOLUME_MAX) {
      volume++;
      sv5w.setVolume(volume);
      Serial.print(F("Volume verhoogd naar ")); Serial.println(volume);
      //sv5w.increaseVolume();
    }
  }
  if(btnVolDown.consumePressed()){
    if(volume > Config::VOLUME_MIN) {
      volume--;
      sv5w.setVolume(volume);
      Serial.print(F("Volume verlaagd naar ")); Serial.println(volume);
      //sv5w.decreaseVolume();
    }
  }
    */
  if (currentProg)
    currentProg->update(now);


}