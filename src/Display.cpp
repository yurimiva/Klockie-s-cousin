#include "Display.h"
#include <U8x8lib.h>
#include <Arduino.h>

// === OGGETTO GLOBALE U8X8 (ultra-low RAM) ===
// SSD1306 128x64 I2C
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(U8X8_PIN_NONE);

// --- Font consigliati ---
#define FONT u8x8_font_chroma48medium8_r   // 8x8       // 8x16: perfetto per l'orologio

// ---------------------------------------------------------------------------
// FORMATTATORI
// ---------------------------------------------------------------------------
static void formatHourMinute(const UserDateTime& u, bool USAFormat, char* out, uint8_t outSize) {
    if (USAFormat) {
        snprintf(out, outSize, "%02d:%02d %s", u.hour, u.minute, u.isPM ? "PM" : "AM");
    } else {
        uint8_t hour24 = u.hour;
        if (u.isPM && hour24 != 12) hour24 += 12;
        if (!u.isPM && hour24 == 12) hour24 = 0;
        snprintf(out, outSize, "%02d:%02d", hour24, u.minute);
    }
}

static void formatDate(const UserDateTime& u, bool USAFormat, char* out, uint8_t outSize) {
    if (USAFormat)
        snprintf(out, outSize, "%02d/%02d/%04d", u.month, u.day, u.year);
    else
        snprintf(out, outSize, "%02d/%02d/%04d", u.day, u.month, u.year);
}

// ---------------------------------------------------------------------------
// SETUP DISPLAY
// ---------------------------------------------------------------------------
void setupDisplay() {
    
    oled.begin();
    Serial.println("OLED initialized");
    oled.clear();

    oled.setFont(FONT);

    delay(600);
}

// ---------------------------------------------------------------------------
// MESSAGGI VELOCI
// ---------------------------------------------------------------------------
void ShowMessage(const char* msg) {
    oled.clear();
    oled.setFont(FONT);
    oled.drawString(0, 0, msg);
}

// ---------------------------------------------------------------------------
// OROLOGIO
// ---------------------------------------------------------------------------
void DisplayTimeDate(const DateTime& now, bool USAFormat) {
    oled.clear();

    UserDateTime u = fromRTC(now, USAFormat);
    char timeBuf[20], dateBuf[20];

    formatHourMinute(u, USAFormat, timeBuf, sizeof(timeBuf));
    formatDate(u, USAFormat, dateBuf, sizeof(dateBuf));

    // Titolo 8×8
    oled.setFont(FONT);
    oled.drawString(0, 0, "TIME AND DATE");

    // Ora grande 8×16 (2 righe di altezza)
    oled.setFont(FONT);
    oled.drawString(0, 2, timeBuf);   // righe 2–3

    // Data in piccolo
    oled.setFont(FONT);
    oled.drawString(0, 6, dateBuf);
}

// ---------------------------------------------------------------------------
// SENSORE
// ---------------------------------------------------------------------------
void DisplayDHT(int8_t temp, int8_t hum, bool USAFormat) {
    oled.clear();
    oled.setFont(FONT);

    oled.drawString(0, 0, "TEMP & HUMIDITY");

    char buf[32];

    snprintf(buf, sizeof(buf), "TEMP: %d %c", temp, USAFormat ? 'F' : 'C');
    oled.drawString(0, 2, buf);

    snprintf(buf, sizeof(buf), "HUM:  %d %%", hum);
    oled.drawString(0, 3, buf);
}

// ---------------------------------------------------------------------------
// SVEGLIA
// ---------------------------------------------------------------------------
void DisplayAlarm(const AlarmState& alarm, bool USAFormat) {
    oled.clear();
    oled.setFont(FONT);
    oled.drawString(0, 0, "ALARM");

    if (!alarm.enabled) {
        oled.drawString(0, 3, "NO ALARM SET");
        return;
    }

    UserDateTime u = fromRTC(alarm.time, USAFormat);
    char timeBuf[20];
    formatHourMinute(u, USAFormat, timeBuf, sizeof(timeBuf));

    // Ora grande
    oled.setFont(FONT);
    oled.drawString(0, 2, timeBuf);
}

// ---------------------------------------------------------------------------
// TIMER
// ---------------------------------------------------------------------------
void DisplayTimer(const TimerState& t) {
    oled.clear();
    oled.setFont(FONT);

    oled.drawString(0, 0, "TIMER");

    char buf[20];

    if (!t.running && !t.done) {
        oled.drawString(0, 3, "No timer set");
        return;
    }

    if (t.running) {
        TimeSpan remaining = t.endTime - GetNow();
        if (remaining.totalseconds() <= 0) {
            oled.drawString(0, 3, "Timer Done!");
            return;
        }

        snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
                 remaining.hours(), remaining.minutes(), remaining.seconds());

        // Ora
        oled.setFont(FONT);
        oled.drawString(0, 2, buf);
        return;
    }

    // Timer già concluso
    oled.drawString(0, 3, "Timer Done!");
}

// ---------------------------------------------------------------------------
// CAMBIO VALORI (editing)
// ---------------------------------------------------------------------------
void DisplayChange(const char* title, int* values, int8_t numFields, int8_t index) {
    oled.clear();

    oled.setFont(FONT);
    oled.drawString(0, 0, title);

    char buf[6];
    uint8_t x = 0;

    for (int8_t i = 0; i < numFields; i++) {

        snprintf(buf, sizeof(buf), "%02d", values[i]);

        if (i == index)
            oled.inverse();   // selezione campo

        oled.drawString(x, 3, buf);

        oled.noInverse();

        x += 4; // spaziatura
    }
}
