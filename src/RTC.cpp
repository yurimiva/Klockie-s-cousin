#include "RTC.h"
#include <Arduino.h>

TimerState timerState;
AlarmState alarm2State;

volatile bool alarm1Triggered = false;
volatile bool alarm2Triggered = false;

bool timerActive = false;    // Stato "attivo" del timer (Alarm1)
bool alarmActive = false;    // Stato "attivo" della sveglia (Alarm2)

volatile bool alarmInterruptFlag = false;

void AlarmISR() {
    alarmInterruptFlag = true;  // Segnala che è arrivata un'interrupt RTC
}

bool IsTimerTriggered() {
    if (alarm1Triggered) {
        alarm1Triggered = false;
        return true;
    }
    return false;
}

bool IsAlarmTriggered() {
    if (alarm2Triggered) {
        alarm2Triggered = false;
        return true;
    }
    return false;
}

// Funzioni per verificare se timer o sveglia sono attivi (melodia in loop)
bool TimerIsActive() {
    return timerActive;
}

bool AlarmIsActive() {
    return alarmActive;
}

// Funzioni per resettare timer e sveglia (quando utente spegne)
void ResetTimer() {
    RTC.clearAlarm(1);
    timerActive = false;
    timerState.running = false;
    timerState.done = false;
}

void ResetAlarm() {
    RTC.clearAlarm(2);
    alarmActive = false;
    alarm2State.enabled = false;
}  


void setupRTC() {

    if (!RTC.begin()) {
        Serial.println("RTC lost power, adjusting time");
        RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
        Serial.println("RTC time set");
    }
    

    // ------------------------------
    // 2) Reset strutture interne
    // ------------------------------
    timerState.running = false;
    timerState.done = false;
    alarm2State.enabled = false;

    alarm1Triggered = false;
    alarm2Triggered = false;
    timerActive = false;
    alarmActive = false;

    // ------------------------------
    // 3) Pin interrupt
    // ------------------------------
    
    pinMode(2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(2), AlarmISR, FALLING);

}


// Timer functions
void StartTimer(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    timerState.endTime = GetNow() + TimeSpan(hours, minutes, seconds, 0);
    timerState.running = true;
    timerState.done = false;
    RTC.setAlarm1(timerState.endTime, DS3231_A1_Second);
}

void StopTimer() {
    ResetTimer();
}

void CheckTimer() {
    if (timerState.running) {
        if (RTC.alarmFired(1)) {
            timerState.running = false;
            timerState.done = true;
        }
    }
}

TimerState GetTimerState() {
    return timerState;
}

// Alarm (sveglia) functions
void SetAlarm2(const DateTime& alarm) {
    alarm2State.time = alarm;
    alarm2State.enabled = true;
    RTC.setAlarm2(alarm, DS3231_A2_Hour);
}

void ClearAlarm2() {
    ResetAlarm();
}

AlarmState GetAlarm2State() {
    return alarm2State;
}

DateTime GetNow() {
    return RTC.now();
}

void AdjustRTC(const DateTime& dt) { 
    RTC.adjust(dt);
}

bool RTC_lostPower() {
    return RTC.lostPower();
}
