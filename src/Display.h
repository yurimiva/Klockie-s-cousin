#pragma once
#include "RTC.h"
#include "DateTime_utils.h"

// Inizializza OLED (U8x8, ultra-low RAM)
void setupDisplay();

// Mostra un messaggio semplice
void ShowMessage(const char* msg);

// Orologio (data e ora)
void DisplayTimeDate(const DateTime& now, bool USAFormat);

// Sensori (temperatura + umidità)
void DisplayDHT(int8_t temp, int8_t hum, bool USAFormat);

// Sveglia (Alarm2)
void DisplayAlarm(const AlarmState& alarm, bool USAFormat);

// Timer (Alarm1)
void DisplayTimer(const TimerState& timer);

// Editing dei valori con joystick
void DisplayChange(const char* title, int* values, int8_t numFields, int8_t index);
