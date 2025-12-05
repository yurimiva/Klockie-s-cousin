#include "DateTime_utils.h"

UserDateTime fromRTC(const DateTime& dt, bool USAFormat) {
    UserDateTime u;
    u.year   = dt.year();
    u.month  = dt.month();
    u.day    = dt.day();
    u.hour   = dt.hour();
    u.minute = dt.minute();
    u.second = dt.second();
    u.isPM   = false;

    if (USAFormat) {
        if (u.hour == 0) { u.hour = 12; u.isPM = false; }
        else if (u.hour == 12) { u.isPM = true; }
        else if (u.hour > 12) { u.hour -= 12; u.isPM = true; }
    }
    return u;
}

DateTime toRTC(const UserDateTime& u) {
    int hour24 = u.hour;
    if (u.isPM && hour24 != 12) hour24 += 12;
    if (!u.isPM && hour24 == 12) hour24 = 0;
    return DateTime(u.year, u.month, u.day, hour24, u.minute, u.second);
}

// --- Utility di validazione ---
bool isLeapYear(uint16_t year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

int getMaxDaysInMonth(uint8_t month, uint16_t year) {
    switch (month) {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12: return 31;
        case 4: case 6: case 9: case 11: return 30;
        case 2: return isLeapYear(year) ? 29 : 28;
        default: return 0;
    }
}

// mi assicuro che la data sia valida
bool isValidDate(uint16_t year, uint8_t month, uint8_t day) {
    if (month < 1 || month > 12) return false;
    int maxDays = getMaxDaysInMonth(month, year);
    return (day >= 1 && day <= maxDays);
}

// al posto di dare errore forzo l'utente a restare dentro i limiti
void clampDate(uint16_t &year, uint8_t &month, uint8_t &day) {
    if (month < 1) month = 1;
    if (month > 12) month = 12;
    int maxDays = getMaxDaysInMonth(month, year);
    if (day < 1) day = 1;
    if (day > maxDays) day = maxDays;
}
