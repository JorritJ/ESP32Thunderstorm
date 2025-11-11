// --- file: Config.h
#pragma once    // include guard
#include <Arduino.h>


// Centrale projectconfiguratie
namespace Config {
// === UART naar SV5W ===
constexpr int UART_RX_PIN = 16; // ESP32 RX2 (naar SV5W TXD/IO0)
constexpr int UART_TX_PIN = 17; // ESP32 TX2 (naar SV5W RXD/IO1)
constexpr uint32_t UART_BAUD = 9600; // 9600-8N1


// === SV5W ===
constexpr uint8_t VOLUME_DEFAULT = 25; // 0..30 typisch
constexpr uint8_t VOLUME_MIN = 0;
constexpr uint8_t VOLUME_MAX = 30;
constexpr uint16_t TRACK_THUNDER = 1; // pas aan naar jouw index/bestandsnaam
constexpr uint16_t TRACK_DAY = 2; // pas aan naar jouw index/bestandsnaam
constexpr int PIN_BUSY = 4; // SV5W BUSY (actief LOW)


// === Knoppen ===
constexpr int PIN_BTN_NEXT = 12; // active LOW, interne pull-up
constexpr int PIN_BTN_PREV = 14; // active LOW, interne pull-up
constexpr int PIN_BTN_VOL_UP = 18; // active LOW, interne pull-up
constexpr int PIN_BTN_VOL_DOWN = 19; // active LOW, interne pull-up
//Hardware: externe 10 kΩ pull-up + 100 nF over elke volumeknop doet wonderen tegen ruis.

// === LED PWM (LEDC) ===
constexpr int LED_COUNT = 4; // <— PAS AAN: totaal aantal ledkanalen
// Kanalen & pinnen (index 0..LED_COUNT-1)
constexpr int LEDC_CH[LED_COUNT]   = { 0, 1, 2, 3 };     // <— extra kanalen
constexpr int PIN_LED[LED_COUNT]   = { 25, 26, 27, 33 }; // <— jouw pins



// === Scenario-profielen ===
// Per scenario een "gewicht" (0..1) per LED-kanaal (kleur/intensiteit basis)
// Thunder: bv. 2 “witte” kanalen dominant
//constexpr float SCENARIO_THUNDER_WEIGHTS[LED_COUNT] = {1.00f, 1.00f, 0.20f, 0.20f}; //SCENARIO_THUNDER_WEIGHTS->LEDSET_THUNDER_WEIGHTS
// Day: bv. alle kanalen zacht met nuance
//constexpr float SCENARIO_DAY_WEIGHTS[LED_COUNT] = {0.35f, 0.40f, 0.30f, 0.25f}; //SCENARIO_DAY_WEIGHTS->LEDSET_DAY_WEIGHTS

// === LED-set per scenario ===
constexpr float LEDSET_THUNDER_WEIGHTS[LED_COUNT] = { 1.00f, 1.00f, 0.20f, 0.20f };
constexpr float LEDSET_DAY_WEIGHTS[LED_COUNT]     = { 0.35f, 0.40f, 0.30f, 0.25f };
// Knipperlicht: kies expliciet welke kanalen mogen knipperen (1.0 = ja, 0.0 = negeren)
constexpr float LEDSET_BLINK_WEIGHTS[LED_COUNT]   = { 0.0f, 1.0f, 0.0f, 0.0f };

/*
Idee: koppel elke LED-index aan een fysieke kleur (bijv. LED 0 = koud wit, LED 1 = warm wit, LED 2 = blauw, etc.).
Per scenario stel je dan weights in om die kleurmix te bepalen.
*/

//klassieke variant:
//constexpr int LEDC_CH1 = 0;
//constexpr int LEDC_CH2 = 1;
//constexpr int PIN_LED_CH1 = 25;
//constexpr int PIN_LED_CH2 = 26;

constexpr uint32_t LEDC_FREQ = 3000; // 3 kHz (pas aan)
constexpr uint8_t LEDC_RES_BITS = 12; // 0..4095 (pas aan)
}