#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include <RTClib.h>  

struct TimerState {
    DateTime endTime;
    bool running;
    bool done;
};

struct AlarmState {
    DateTime time;
    bool enabled;
};

extern RTC_DS3231 RTC;

extern TimerState timerState;
extern AlarmState alarm2State;

// Flag interni per ISR
extern volatile bool alarm1Triggered;
extern volatile bool alarm2Triggered;

// Stato "attivo" (melodia in loop)
extern bool timerActive;
extern bool alarmActive;

// Setup
void setupRTC();

bool checkOscillatorStopFlag();

void clearOscillatorStopFlag();

// Timer (Alarm1)
void StartTimer(uint8_t hours, uint8_t minutes, uint8_t seconds);
void StopTimer();
void CheckTimer();
TimerState GetTimerState();

bool IsTimerTriggered();
bool TimerIsActive();
void ResetTimer();

// Sveglia (Alarm2)
void SetAlarm2(const DateTime& alarm);
void ClearAlarm2();
AlarmState GetAlarm2State();

bool IsAlarmTriggered();
bool AlarmIsActive();
void ResetAlarm();

// Funzione di utilità
DateTime GetNow();
void AdjustRTC(const DateTime& dt);
bool RTC_lostPower();

#endif
