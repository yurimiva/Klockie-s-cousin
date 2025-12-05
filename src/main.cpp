#include <Arduino.h>
#include <Wire.h>
#include "RTC.h"
#include "Display.h"
#include "Edit.h"
#include "Input.h"
#include "Sensors.h"
#include "Melody.h"
#include "Settings.h"

#include <avr/wdt.h>
#include <avr/interrupt.h>

RTC_DS3231 RTC;

enum SystemState {
  STATE_CLOCK,
  STATE_SENSORS,
  STATE_TIMER,
  STATE_ALARM,
  STATE_SETTINGS,
  IDLE
};

void setup() {
  
  Wire.begin();
  delay(50);

  setupRTC();
  setupDisplay();
  setupInput();
  setupSensors();
  setupMelody();
  setupSettings();

  ShowMessage("System Ready");

}


void HandleButton(Direction dir) {

  if (WasClicked()) {
    switch (dir) {

      case LEFT: {
        EditTimer();
        break;
      }

      case DOWN: {
        EditAlarm();
        break;
      }

      case UP: {
        DateTime now = GetNow();
        EditDateTime(now, use12hFormat); // qui usi la tua funzione
        break;
      }

      case RIGHT:
      case CENTER: {
        break;
      }

    }
  }

  if (WasLongPressed()) {
    switch (dir) {
      case UP: {
        // Schermata orologio → toggle formato orario
        use12hFormat = !use12hFormat;
        ShowMessage(use12hFormat ? "12h format" : "24h format");
        break;
      }
        
      case RIGHT: { 
        // Schermata sensori → toggle °C/°F
        useFahrenheit = !useFahrenheit;
        ShowMessage(useFahrenheit ? "Fahrenheit" : "Celsius");
        break;
      }
      
      case LEFT: {
        // Schermata timer → stop timer se attivo
        if (GetTimerState().running) {
          StopTimer();
          ShowMessage("Timer OFF");
        } else {
          ShowMessage("No Timer");
        }
        break;
      }
        
      case DOWN: {
        // Schermata sveglia → disattiva sveglia se attiva
        if (GetAlarm2State().enabled) {
          ClearAlarm2();
          ShowMessage("Alarm OFF");
        } else {
          ShowMessage("No Alarm");
        }
        break;
      }
        
      default: {
        // Nessuna direzione → fallback generale
        ShowMessage("LongPress ignored");
        break;
      }
        
    }
  }

}

void DetermineState(SystemState* state) {

  Direction dir = readJoystick();

  bool actionTaken = false; 

  // 1. GESTIONE AZIONI (CLICK/LONG PRESS)
  if (WasClicked() || WasLongPressed()) {
      HandleButton(dir);
      actionTaken = true;
  }

  if (actionTaken) {
    return; 
  }

  // 2. GESTIONE CAMBIO SCHERMATA
  switch (dir) {
    case UP: {
      *state = STATE_CLOCK; // Aggiorna lo stato tramite puntatore
      break;
    }

    case RIGHT: { 
      *state = STATE_SENSORS;
      break;
    }

    case DOWN: { 
      *state = STATE_ALARM;
      break;
    }

    case LEFT: {
      *state = STATE_TIMER;
      break;
    }

    default:
      break;
  }

}

void updateMelodyState() {
  if (TimerIsActive() && !AlarmIsActive()) {
      if (!isMelodyPlaying()) {
          startMelody(timerMelody, timerMelodyLength);
      }
  } else if (AlarmIsActive()) {
      if (!isMelodyPlaying()) {
          startMelody(alarmMelody, alarmMelodyLength);
      }
  } else {
      if (isMelodyPlaying()) {
          stopMelody();
      }
  }
}


void loop() {

  static SystemState currentState;

  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 1000) {
    Serial.println("Loop entered...");
    lastMillis = millis();
  }

  // Aggiornamento dei bottoni
  updateButton();

    // Gestione Timer e Sveglia
  CheckTimer();

  // Gestione melodia (continua finché non premi bottone)
  updateMelodyState();
  handleMelody();

  DetermineState(&currentState);

  switch (currentState) {
    case STATE_CLOCK: {
      DateTime now = GetNow();
      DisplayTimeDate(now, use12hFormat);
      break;
    }
      
    case STATE_SENSORS: {
      int8_t temp = readTemperature(useFahrenheit);
      int8_t hum  = readHumidity();

      if (temp == -100 || hum == -1) {
        ShowMessage("Sensor Error");
      } else {
        DisplayDHT(temp, hum, useFahrenheit);
      }
      break;
    }

    case STATE_TIMER: {
      DisplayTimer(GetTimerState());
      break;
    }
      
    case STATE_ALARM: {
      DisplayAlarm(GetAlarm2State(), use12hFormat);
      break;
    }
      
    case STATE_SETTINGS: {
      ShowMessage(use12hFormat ? "12h format" : "24h format");
      break;
    }

    case IDLE: {
      break;
    }

  }

      
}

