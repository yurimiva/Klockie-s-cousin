#include "Melody.h"
#include <Arduino.h>
#include "Input.h"

#define BUZZER_PIN 11

const int alarmMelody[][2] = {
    {1319, 300}, {988, 300}, {1109, 300}, {1175, 600}, {0, 200}
};

const int timerMelody[][2] = {
    {784, 200}, {784, 200}, {784, 200}, {0, 100}, {988, 400}, {784, 400}, {0, 200}
};

const int alarmMelodyLength = sizeof(alarmMelody)/sizeof(alarmMelody[0]);
const int timerMelodyLength = sizeof(timerMelody)/sizeof(timerMelody[0]);

bool melodyActive = false;
const int (*currentMelody)[2] = nullptr;
int melodyLength = 0;
int melodyIndex = 0;
unsigned long lastNoteTime = 0;

void setupMelody() {
    pinMode(BUZZER_PIN, OUTPUT);
}

void handleMelody() {
    if (!melodyActive || currentMelody == nullptr) return;

    unsigned long now = millis();
    int freq = currentMelody[melodyIndex][0];
    int dur  = currentMelody[melodyIndex][1];

    if (now - lastNoteTime >= (unsigned long)dur) {
        melodyIndex++;
        if (melodyIndex >= melodyLength) melodyIndex = 0;

        freq = currentMelody[melodyIndex][0];
        dur  = currentMelody[melodyIndex][1];
        lastNoteTime = now;

        if (freq > 0) tone(BUZZER_PIN, freq);
        else noTone(BUZZER_PIN);
    }

    if (WasClicked()) {
        stopMelody();
    }
}

void startMelody(const int melody[][2], int length) {
    currentMelody = melody;
    melodyLength = length;
    melodyIndex = 0;
    melodyActive = true;
    lastNoteTime = millis();
}

void stopMelody() {
    noTone(BUZZER_PIN);
    melodyActive = false;
    currentMelody = nullptr;
}

bool isMelodyPlaying() {
    return melodyActive;
}
