#pragma once
#include <RTClib.h>
#include "DateTime_utils.h"
#include "Display.h"
#include "Input.h"
#include "RTC.h"

// Funzione generica per editare un array di valori
bool EditFields(const char* title, int* values, int* minVal, int* maxVal, int numFields);

// Funzioni specifiche di editing
void EditDateTime(DateTime& dt, bool USAFormat);
void EditAlarm();
void EditTimer();
