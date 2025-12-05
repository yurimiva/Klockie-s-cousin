#include "Edit.h"
#include "RTC.h"

bool EditFields(const char* title, int* values, int* minVal, int* maxVal, int numFields) {
    uint8_t index = 0;
    unsigned long lastMove = 0;
    const unsigned long repeatDelay = 140;

    while (true) {
        unsigned long now = millis();
        Direction dir = readJoystick();

        if (now - lastMove >= repeatDelay) {
            switch (dir) {
                case UP:
                    values[index]++;
                    if (values[index] > maxVal[index]) values[index] = minVal[index];
                    lastMove = now;
                    break;
                case DOWN:
                    values[index]--;
                    if (values[index] < minVal[index]) values[index] = maxVal[index];
                    lastMove = now;
                    break;
                case RIGHT:
                    index++;
                    if (index >= numFields) return true; // fine editing
                    lastMove = now;
                    break;
                case LEFT:
                    if (index > 0) index--;
                    lastMove = now;
                    break;
                default: break;
            }
        }

        // Aggiorna display
        DisplayChange(title, values, numFields, index);

        if (WasClicked()) return true; // uscita rapida con bottone
    }
}

void EditDateTime(DateTime& dt, bool USAFormat) {

    UserDateTime u = fromRTC(dt, USAFormat);

    int values[6], minVal[6], maxVal[6], fields = 0;

    // Ora
    values[fields] = u.hour; minVal[fields] = USAFormat ? 1 : 0; maxVal[fields] = USAFormat ? 12 : 23; fields++;
    // Minuti
    values[fields] = u.minute; minVal[fields] = 0; maxVal[fields] = 59; fields++;
    // AM/PM
    if (USAFormat) { values[fields] = u.isPM; minVal[fields] = 0; maxVal[fields] = 1; fields++; }
    // Giorno/Mese
    values[fields] = USAFormat ? u.month : u.day; minVal[fields] = 1; maxVal[fields] = 12; fields++;
    values[fields] = USAFormat ? u.day : u.month; minVal[fields] = 1; maxVal[fields] = 31; fields++;
    // Anno
    values[fields] = u.year; minVal[fields] = 2000; maxVal[fields] = 2099; fields++;

    if (EditFields("SET DATE/TIME", values, minVal, maxVal, fields)) {
        // Conversione indietro
        u.hour   = values[0];
        u.minute = values[1];
        if (USAFormat) u.isPM = values[2];
        if (USAFormat) { u.month = values[3]; u.day = values[4]; }
        else { u.day = values[3]; u.month = values[4]; }
        u.year = values[5];

        // Validazione con DateTime_utils
        if (!isValidDate(u.year, u.month, u.day)) {
            clampDate(u.year, u.month, u.day); // correzione automatica
        }

        dt = toRTC(u);
        AdjustRTC(dt);
    }
}

// --- Sveglia (Alarm2) ---
void EditAlarm() {
    DateTime now = GetNow();
    UserDateTime u = fromRTC(now, true); // supponiamo formato USA per esempio

    int values[3], minVal[3], maxVal[3], fields = 0;

    // Ora
    values[fields] = u.hour; minVal[fields] = 1; maxVal[fields] = 12; fields++;
    // Minuti
    values[fields] = u.minute; minVal[fields] = 0; maxVal[fields] = 59; fields++;
    // AM/PM
    values[fields] = u.isPM; minVal[fields] = 0; maxVal[fields] = 1; fields++;

    if (EditFields("SET ALARM", values, minVal, maxVal, fields)) {
        u.hour   = values[0];
        u.minute = values[1];
        u.isPM   = values[2];

        // Validazione
        clampDate(u.year, u.month, u.day);

        DateTime alarm = toRTC(u);
        SetAlarm2(alarm); // usa Alarm2
    }
}

// --- Timer (Alarm1) ---
void EditTimer() {
    int values[3], minVal[3], maxVal[3];
    values[0] = 0; minVal[0] = 0; maxVal[0] = 23; // ore
    values[1] = 0; minVal[1] = 0; maxVal[1] = 59; // minuti
    values[2] = 0; minVal[2] = 0; maxVal[2] = 59; // secondi

    if (EditFields("SET TIMER", values, minVal, maxVal, 3)) {
        StartTimer(values[0], values[1], values[2]); // usa Alarm1
    }
}
