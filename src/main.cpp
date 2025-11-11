// --- file: example_usage.ino
#include <Arduino.h>
#include <HardwareSerial.h>
#include "SV5W.h"
#include "Button.h"
#include "LedPwm.h"
#include "LightProgram.h"
#include "Config.h"
#include "LedSet.h"
#include "BlinkOverlay.h" // voor knipper-overlay

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


// ===================== Buttons =====================
Button btnNext{Config::PIN_BTN_NEXT,true};
Button btnPrev{Config::PIN_BTN_PREV,true};
Button btnVolUp(Config::PIN_BTN_VOL_UP, true);
Button btnVolDown(Config::PIN_BTN_VOL_DOWN, true);


// ===================== DY-SV5W =====================
SV5W sv5w;
constexpr uint16_t TRACK_THUNDER = Config::TRACK_THUNDER; // pas aan naar jouw index/bestandsnaam
constexpr uint16_t TRACK_DAY = Config::TRACK_DAY; // pas aan naar jouw index/bestandsnaam

// --- SV5W BUSY monitoring (active LOW)
static bool busyState = false;               // gefilterde/stabiele staat
static bool busyRaw   = false;               // laatst gemeten raw
static uint32_t busyLastEdgeMs = 0;          // laatste raw edge
static const uint32_t BUSY_DEBOUNCE_MS = 5;  // kleine debounce tegen rammel
inline bool sv5wBusyRaw() { return digitalRead(Config::PIN_BUSY) == LOW; } // LOW = playing


// ===================== Lichtprogramma's =====================
enum class Mode
{
  Thunder = 0,
  Day = 1,
  COUNT
};
Mode currentMode = Mode::Thunder;

static BlinkOverlay gBlink; // globale knipper-overlay

// Led kanalen:
static LedPwmChannel* LEDS[Config::LED_COUNT];

// LedSets per scenario:
static LedSet* thunderSetPtr = nullptr;
static LedSet* daySetPtr     = nullptr;
static LedSet* blinkSetPtr   = nullptr;

// Programma’s als pointers:
static ProgThunder* progThunderPtr = nullptr;
static ProgDay*     progDayPtr     = nullptr;

static inline LedSet* activeSet() {
  return (currentMode == Mode::Thunder) ? thunderSetPtr : daySetPtr;
}

LightProgram *currentProg = nullptr;

void startMode(Mode m, uint32_t now)
{
  currentMode = m;
  sv5w.stop();
  delay(100);
  switch (m)
  {
  case Mode::Thunder:
    sv5w.playTrack(TRACK_THUNDER);
    currentProg = progThunderPtr;
    //sv5w.playByPath(0x01, "\\THUNDER\\00001.MP3"); // drive: 0x01 = SD
    //sv5w.playByPath("\\THUNDER\\00001.MP3"); // gebruikt default drive //Zorg dat de backslash-escapes kloppen in C++-strings (\"\\\\THUNDER\\\\00001.MP3\" in code).
    //sv5w.playRandomFolderClip("\\THUNDER", /*maxIndex=*/20); //speel random clip uit map THUNDER met bestanden 00001.MP3 .. 00020.MP3
    break;
  case Mode::Day:
    sv5w.playTrack(TRACK_DAY);
    currentProg = progDayPtr;
    break;
  default:
    sv5w.playTrack(TRACK_THUNDER);
    currentProg = progThunderPtr;
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
  btnVolUp.begin();
  btnVolDown.begin();
  
  //sv5w BUSY pin initialiseren
  busyRaw   = sv5wBusyRaw();
  delay(100);
  busyState = busyRaw;
  delay(100);
  Serial.printf("SV5W BUSY init: %s (active LOW)\n", busyState ? "ACTIVE (playing)" : "IDLE");

  // LED kanalen initialiseren
  for (int i = 0; i < Config::LED_COUNT; ++i) {
    LEDS[i] = new LedPwmChannel(Config::LEDC_CH[i], Config::PIN_LED[i],
                                Config::LEDC_FREQ, Config::LEDC_RES_BITS);
    LEDS[i]->begin();
    LEDS[i]->setDuty(0);
  }

  // LedSets per scenario (met weights uit Config)
  thunderSetPtr = new LedSet(LEDS, Config::LED_COUNT, Config::LEDSET_THUNDER_WEIGHTS);
  daySetPtr     = new LedSet(LEDS, Config::LED_COUNT, Config::LEDSET_DAY_WEIGHTS);
  blinkSetPtr   = new LedSet(LEDS, Config::LED_COUNT, Config::LEDSET_BLINK_WEIGHTS); // <— nieuw

  // Programma’s aanmaken op die sets
  progThunderPtr = new ProgThunder(*thunderSetPtr, Config::LEDSET_THUNDER_WEIGHTS);
  progDayPtr     = new ProgDay(*daySetPtr,       Config::LEDSET_DAY_WEIGHTS);
  
  //start knipper-overlay (optioneel)
  gBlink.start(millis(), 500, 50, 1.0f, 0.0f); //test: start knipper-overlay (500ms periode, 50% duty)
  //gBlink.start(millis(), 800, 50, 1.0f, 0.3f); //test: start knipper-overlay (800ms periode, 50% duty, 30% brightness)
  //gBlink.stop(blinkSetPtr); //standaard uitzetten

  // SV5W init
  sv5w.begin(Serial2, Config::UART_RX_PIN, Config::UART_TX_PIN, Config::UART_BAUD);
  delay(100);
  sv5w.setVolume(Config::VOLUME_DEFAULT);
  // Kies desgewenst standaard-drive (0x00=USB, 0x01=SD, 0x02=FLASH)
  sv5w.setDefaultDrive(0x01);

  // (Optioneel) huidige afspeeldrive uitlezen (test communicatie)
  auto playdrive = sv5w.queryCurrentPlayDrive();
  delay(100);
  if (playdrive.valid) {
    Serial.print(F("SV5W Current Play Drive: "));
    for (auto b : playdrive.data) { Serial.printf(" %02X", b); }
    Serial.println();
  } else {
    Serial.println(F("SV5W Current Play Drive query failed."));
  }

  // (Optioneel) firmwareversie uitlezen
  auto ver = sv5w.queryVersion();
  delay(100);
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
  
  //initialeseer knoppen:
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
  
  //volume knoppen bediening
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
  
  if (currentProg)
    currentProg->update(now);

  if (blinkSetPtr) {
    gBlink.update(now, *blinkSetPtr);
  }
   
  //
  //LedSet* activeSet = (currentMode == Mode::Thunder) ? thunderSetPtr : daySetPtr;
  //if (activeSet) gBlink.update(now, *activeSet);

}