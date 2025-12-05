#ifndef MELODY_H
#define MELODY_H

#include <Arduino.h>

extern const int alarmMelody[][2];
extern const int timerMelody[][2];

extern const int alarmMelodyLength;
extern const int timerMelodyLength;

void setupMelody();
void startMelody(const int melody[][2], int length);
void stopMelody();         // Ferma la melodia
bool isMelodyPlaying();    // Stato
void handleMelody();       // Da chiamare nel loop

#endif