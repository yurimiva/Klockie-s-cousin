#pragma once
#include <RTClib.h>

struct UserDateTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;    // 1–12 se USAFormat, 0–23 altrimenti
    uint8_t minute;
    uint8_t second;
    bool isPM;   // valido solo se USAFormat
};

// --- FUNZIONI DI CONVERSIONE ---
UserDateTime fromRTC(const DateTime& dt, bool USAFormat);
DateTime toRTC(const UserDateTime& u);

// --- Utility di validazione ---
bool isLeapYear(uint16_t year);
int getMaxDaysInMonth(uint8_t month, uint16_t year);
bool isValidDate(uint16_t year, uint8_t month, uint8_t day);
void clampDate(uint16_t &year, uint8_t &month, uint8_t &day);