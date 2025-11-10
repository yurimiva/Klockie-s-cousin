#include <Wire.h>
#include <U8g2lib.h>
#include <DHTStable.h>
#include <RTClib.h>
#include <OneButton.h>
#include "pitches.h"
#include "melodies.h"
#include <avr/sleep.h>
#include <avr/power.h>


// define the DHT
#define DHTPIN 7 // pin connected to DHT11 sensor
DHTStable DHT;

// define the OLED (U8g2 version)
U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// define the joystick
const int Joystick_X = 14;
const int Joystick_Y = 15;

// let's add an enum to mark things more easily
enum Direction { UP, DOWN, LEFT, RIGHT, CENTER };

#define BUTTON_PIN 3
OneButton button(BUTTON_PIN, true, true); // pin, activeLow, pullup
// variabile globale da usare solo quando suonano sveglia o timer
volatile bool stopMelody = false;    // fermare la sveglia attiva
volatile bool melodyPlaying = false; // segnala se una melodia suona o meno

volatile bool buttonClicked = false;
volatile bool longPressAction = false; // trigger per funzione extra

bool useFahrenheit = false;  // true → Fahrenheit
bool use12hFormat = false;   // true → formato 12h (AM/PM)
bool alarmEnabled = false;


// define an rtc object
RTC_DS3231 RTC;

// define the PIN connected to SQW
const int SQW_INTERRUPT = 2;
volatile bool rtcInterruptFlag = false;

// define buzzer
const int Buzzer_Pin = 8;

// a struct for the timer is needed since I'll treat it as a second alarm
struct Timer {
  DateTime endTime;  // momento in cui scade il timer
  bool running;      // true se countdown attivo
  bool done;         // true se scaduto
};

// need to set an enumerable that helps me with the display of the modes
enum menu
{
  Idle, // fatto
  ShowTimeAndDate, // fatto
  ChangeTimeDate, // fatto
  ShowTempAndHum, // fatto
  ShowAlarm, // fatto
  changeAlarm, // fatto
  ShowTimer, // sto facendo
  changeTimer // sto facendo
};

// ---

// ISR function
void onRTCInterrupt() {
  rtcInterruptFlag = true;
}

// ---
// Functions for the long and short press of the button
void onButtonClick() {
  // Se la sveglia o il timer stanno suonando → ferma solo la melodia
  if (melodyPlaying) {
    stopMelody = true;
  } else {
    // Altrimenti, è un click normale → usato dal menu
    buttonClicked = true;
  }
}

void onButtonLongPress() {
  longPressAction = true; // funzione speciale, gestita nel loop
}


// setup
void setup()
{
  // Initialazing oled with U8g2
  oled.begin();

  // Mostra messaggio di setup
  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tf); // font leggibile e piccolo
    oled.drawStr(0, 12, "Setup OK!");
  } while (oled.nextPage());
  delay(500);
  
  // coordinates for joystick
  pinMode(Joystick_X, INPUT);
  pinMode(Joystick_Y, INPUT);

  button.attachClick(onButtonClick);           // click breve
  button.attachLongPressStart(onButtonLongPress); // pressione lunga

  // buzzer
  pinMode(Buzzer_Pin, OUTPUT);

  // initialize the RTC
  RTC.begin();
  // don't need the 32K Pin, so I simply didn't connect it

  // setup for the SQW PIN
  // Making it so, that the alarm will trigger an interrupt
  pinMode(SQW_INTERRUPT, INPUT_PULLUP);
  // stop oscillating signals at SQW Pin
  // otherwise setAlarm will fail
  RTC.writeSqwPinMode(DS3231_OFF);

  RTC.clearAlarm(1);
  RTC.clearAlarm(2);

  attachInterrupt(digitalPinToInterrupt(SQW_INTERRUPT), onRTCInterrupt, FALLING);

}


// UTILITY FUNCTIONS
void goIdle() {

  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  sleep_cpu();    // dorme finché arriva un interrupt
  sleep_disable();
}

Direction readJoystick(int x, int y) {

  int deadzone = 100;

  // Deadzone check (joystick idle)
  if (abs(x) < deadzone && abs(y) < deadzone) {
    goIdle();
    return CENTER;
  }

  // Compare absolute values to determine dominant direction
  if (abs(x) > abs(y)) {
    return (x > 0) ? RIGHT : LEFT;
  } else {
    return (y < 0) ? UP : DOWN;
  }
}

void BeepOnce(bool *beepstate)
{

  if (*beepstate == false)
  {
    *beepstate = true;
    tone(Buzzer_Pin, 1047, 200);
  }
}

void DetermineState(menu *state, int Joystick_X, int Joystick_Y) {

  static bool BEEPSTATE;
  Direction joystick_state;
  joystick_state = readJoystick(Joystick_X, Joystick_Y);

  // upper and lower area of the joystick
  switch (joystick_state)
  {
  case UP:
    BeepOnce(&BEEPSTATE);
    *state = ShowTimeAndDate;
    break;
  case RIGHT:
    BeepOnce(&BEEPSTATE);
    *state = ShowTempAndHum;
    break;
  case DOWN:
    BeepOnce(&BEEPSTATE);
    *state = ShowAlarm;
    break;
  case LEFT:
    BeepOnce(&BEEPSTATE);
    *state = ShowTimer;
    break;
  case CENTER:
    BEEPSTATE = false;
    break;
  }

}


// ALL OF THE DISPLAY FUNCTIONS ON OLED
void DisplayDHT()
{

  // mi assicura che gli input del DHT siano letti solo dopo che è passato un certo tempo
  // evita così dati corrotti
  static unsigned long lastReadTime = 0;

  static int lastTemperature = 0;
  static int lastHumidity = 0;

  unsigned long now = millis();

  // Leggi il sensore solo ogni 2 secondi
  if (now - lastReadTime > 2000) {
    int chk = DHT.read11(DHTPIN);
    if (chk == DHTLIB_OK) {
      lastTemperature = (int)DHT.getTemperature();
      lastHumidity = (int)DHT.getHumidity();
    }
    lastReadTime = now;
  }

  int displayTemp = lastTemperature;
  const char* unit = "C";

  if (useFahrenheit) {
    displayTemp = (displayTemp * 1.8) + 32;
    unit = "F";
  }

  // con questo pezzo evito di aggiornare ogni volta l'oled
  // In questo modo ne riduco i consumi
  static int lastShownTemp = -1000;
  static int lastShownHum = -1;

  if (abs(displayTemp - lastShownTemp) < 1 && lastHumidity == lastShownHum) return;

  lastShownTemp = displayTemp;
  lastShownHum = lastHumidity;

  char buffer[32];
  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 12, "TEMP. & HUMIDITY");

    snprintf(buffer, sizeof(buffer), "TEMP: %d %s", displayTemp, unit);
    oled.drawStr(0, 30, buffer);

    snprintf(buffer, sizeof(buffer), "HUM:  %d %%", lastHumidity);
    oled.drawStr(0, 46, buffer);
  } while (oled.nextPage());

}

void DisplayTimeDate() {

  static int lastHour = -1;
  static int lastMinute = -1;

  DateTime now = RTC.now();

  if (now.hour() == lastHour && now.minute() == lastMinute) return;

  lastHour = now.hour();
  lastMinute = now.minute();

  int displayHour = now.hour();  // variabile temporanea solo per la stampa
  const char* ampm = "";

  if (use12hFormat) {
    if (displayHour == 0) {
      displayHour = 12;
      ampm = "AM";
    } else if (displayHour == 12) {
      ampm = "PM";
    } else if (displayHour > 12) {
      displayHour -= 12;
      ampm = "PM";
    } else {
      ampm = "AM";
    }
  }

  char buffer[32] = {0};
  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 12, "TIME AND DATE");

    // Ora
    oled.setFont(u8g2_font_logisoso16_tr);
    if (use12hFormat)
      snprintf(buffer, sizeof(buffer), "%02d:%02d %s", displayHour, lastMinute, ampm);
    else
      snprintf(buffer, sizeof(buffer), "%02d:%02d", lastHour, lastMinute);
    oled.drawStr(10, 36, buffer);

    // Data
    oled.setFont(u8g2_font_6x12_tf);
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", now.day(), now.month(), now.year());
    oled.drawStr(20, 56, buffer);
  } while (oled.nextPage());

}

void DisplayAlarm() {

  static int lastHour = -1;
  static int lastMinute = -1;
  static bool lastEnabled = false;

  if (!alarmEnabled && !lastEnabled) {
    // Nessuna sveglia e già mostrato "NO ALARM SET" → non ridisegnare
    return;
  }

  DateTime alarm1 = RTC.getAlarm1();

  if (alarmEnabled &&
      alarm1.hour() == lastHour &&
      alarm1.minute() == lastMinute &&
      lastEnabled == alarmEnabled) {
    // Sveglia non cambiata → niente aggiornamento
    return;
  }

  lastHour = alarm1.hour();
  lastMinute = alarm1.minute();
  lastEnabled = alarmEnabled;

  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 12, "ALARM");

    if (!alarmEnabled) {
      oled.drawStr(20, 40, "NO ALARM SET");
    } else {
      int displayHour = alarm1.hour();
      const char* ampm = "";

      if (use12hFormat) {
        if (displayHour == 0) {
          displayHour = 12;
          ampm = "AM";
        } else if (displayHour == 12) {
          ampm = "PM";
        } else if (displayHour > 12) {
          displayHour -= 12;
          ampm = "PM";
        } else {
          ampm = "AM";
        }
      }

      char buf[16];
      if (use12hFormat)
        snprintf(buf, sizeof(buf), "%02d:%02d %s", displayHour, alarm1.minute(), ampm);
      else
        snprintf(buf, sizeof(buf), "%02d:%02d", alarm1.hour(), alarm1.minute());

      oled.drawStr(30, 40, buf);
    }

  } while (oled.nextPage());
}


void UpdateTimerDisplay(Timer &t) {

  static unsigned long lastDisplayUpdate = 0;
  static int lastSeconds = -1;

  unsigned long nowMillis = millis();
  if (nowMillis - lastDisplayUpdate < 200) return; // controlla più spesso, ma aggiorna solo se serve
  lastDisplayUpdate = nowMillis;

  DateTime now = RTC.now();
  TimeSpan remaining = t.endTime - now;

  // Gestione scadenza
  if (remaining.totalseconds() <= 0 && t.running) {
    t.running = false;
    t.done = true;
    remaining = TimeSpan(0);
  }

  // Evita refresh se i secondi non cambiano
  if (remaining.seconds() == lastSeconds && t.running) return;
  lastSeconds = remaining.seconds();

  // --- DISPLAY ---
  char buf[16];
  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 12, "TIMER");

    if (!t.running && !t.done) {
      oled.drawStr(20, 40, "NO TIMER SET");
    } else {
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
               remaining.hours(), remaining.minutes(), remaining.seconds());
      oled.drawStr(20, 40, buf);
    }
  } while (oled.nextPage());
}


// ALL CHANGE FUNCTIONS
/**
 * @brief Display value editing interface on OLED.
 * @param title The title shown at the top of the screen.
 * @param values Array of integers (time/date values).
 * @param numFields Number of editable fields.
 */
void DisplayChange(const char* title, int* values, int numFields) {
  char buf[32];

  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tf);

    // Title
    oled.drawStr(0, 12, title);

    switch (numFields) {
      case 2: // Example: Alarm or Timer (HH:MM)
        snprintf(buf, sizeof(buf), "%02d:%02d", values[0], values[1]);
        oled.drawStr(25, 36, buf);
        break;

      case 3: // Example: Timer with seconds (HH:MM:SS)
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", values[0], values[1], values[2]);
        oled.drawStr(20, 36, buf);
        break;

      case 5: // Example: Full date/time (HH:MM  DD/MM/YYYY or MM/DD/YYYY)
        snprintf(buf, sizeof(buf), "%02d:%02d", values[0], values[1]);
        oled.drawStr(30, 30, buf);

      default:
        oled.drawStr(10, 36, "Invalid format");
        break;
    }

  } while (oled.nextPage());
}

/**
 * @brief Handle value modification using joystick.
 * @param values Pointer to array of editable values.
 * @param minVals Minimum values for each field.
 * @param maxVals Maximum values for each field.
 * @param numFields Number of editable fields.
 * @param title Title displayed during edit.
 */
void ChangeValue(int* values, int* minVals, int* maxVals, int numFields, const char* title) {

  int index = 0;
  bool done = false;

  while (!done) {
    Direction dir = readJoystick(analogRead(Joystick_X) - 512, analogRead(Joystick_Y) - 512);

    switch (dir) {
      case UP:
        values[index] = (values[index] >= maxVals[index]) ? minVals[index] : values[index] + 1;
        break;
      case DOWN:
        values[index] = (values[index] <= minVals[index]) ? maxVals[index] : values[index] - 1;
        break;
      case RIGHT:
        index++;
        break;
      case LEFT:
        index--;
        break;
      case CENTER:
        break;
    }

    DisplayChange(title, values, numFields);

    if (index >= numFields) {
      done = true;
      oled.clearBuffer();
      oled.setFont(u8g2_font_6x12_tf);
      oled.drawStr(25, 35, "Saved!");
      oled.sendBuffer();
      break;
    }

    delay(150);
  }
}

/**
 * @brief Allow user to change system time and date.
 * Applies user preferences for MDY/DMY and 12h/24h formats.
 */
void ChangeTimeAndDate() {
  DateTime now = RTC.now();

  // Extract current date/time
  int hour = now.hour();
  int minute = now.minute();
  int day = now.day();
  int month = now.month();
  int year = now.year();

  // Handle 12-hour format input
  int isPM = 0;
  if (use12hFormat) {
    if (hour >= 12) {
      hour = (hour > 12) ? hour - 12 : hour;
      isPM = 1;
    } else if (hour == 0) {
      hour = 12;
    }
  }

  int values[5] = {hour, minute, day, month, year};

  // Value limits
  int minVal[5] = { use12hFormat ? 1 : 0, 0, 1, 1, 2000 };
  int maxVal[5] = { use12hFormat ? 12 : 23, 59, 31, 12, 2099 };

  ChangeValue(values, minVal, maxVal, 5, "SET TIME AND DATE");

  // Convert back to 24h before setting RTC
  int adjHour = values[0];
  if (use12hFormat) {
    if (adjHour == 12) adjHour = 0;
    if (isPM) adjHour += 12;
  }

}

void ChangeAlarm() {

  DateTime alarm = RTC.getAlarm1();

  int values[2]  = {alarm.hour(), alarm.minute()};
  int minVals[2] = {0, 0};
  int maxVals[2] = {23, 59};

  ChangeValue(values, minVals, maxVals, 2, "SET ALARM");

  RTC.setAlarm1(DateTime(0, 0, 0, values[0], values[1], 0), DS3231_A1_Hour);
  alarmEnabled = true;

}

void ChangeTimer(Timer &t) {
  int values[2]  = {0, 0};
  int minVals[2] = {0, 0};
  int maxVals[2] = {23, 59};

  // input dell’utente con cursore
  ChangeValue(values, minVals, maxVals, 2, "SET TIMER");

  // calcola momento di fine timer
  t.endTime = RTC.now() + TimeSpan(0, values[0], values[1], 0);
  t.running = true;
  t.done = false;

  // feedback breve
  oled.clearBuffer();             // Pulisce il buffer
  oled.setFont(u8g2_font_ncenB08_tr); // Imposta un font leggibile
  oled.drawStr(20, 30, "TIMER STARTED"); // Disegna la scritta al centro (approssimativo)
  oled.sendBuffer();              // Aggiorna il display
  delay(500);
}

// --------
// Functions for handling the alarms

// Funzione per suonare melodia interrompibile dal bottone
void PlayMelodyInterruptible(const int *melody, const int *durations, int length) {
  melodyPlaying = true;
  stopMelody = false;

  // Schermo vuoto mentre suona
  oled.clearBuffer();
  oled.sendBuffer();

  while (!stopMelody) {
    for (int i = 0; i < length && !stopMelody; i++) {
      int noteDuration = 1000 / durations[i];
      unsigned long start = millis();

      tone(Buzzer_Pin, melody[i]); // avvia la nota

      while (millis() - start < noteDuration * 1.3) {
        button.tick();
        if (stopMelody) break;
      }

      noTone(Buzzer_Pin);
    }
  }

  noTone(Buzzer_Pin);
  melodyPlaying = false;
  stopMelody = false;

  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  oled.drawStr(20, 30, "FIRING STOPPED");
  oled.sendBuffer();
  delay(800);
}


// Gestione allarmi/timer
void HandleAlarms() {

  if (rtcInterruptFlag) {
    rtcInterruptFlag = false;

    if (RTC.alarmFired(1)) {
      RTC.clearAlarm(1);
      PlayMelodyInterruptible(alarm_melody, alarm_durations, sizeof(alarm_melody)/sizeof(alarm_melody[0]));
    }

    if (RTC.alarmFired(2)) {
      RTC.clearAlarm(2);
      PlayMelodyInterruptible(timer_melody, timer_durations, sizeof(timer_melody)/sizeof(timer_melody[0]));
    }
  }
}



void loop()
{
  button.tick(); 
  HandleAlarms();

  static menu STATE = Idle;
  // read coordinates to determine the state
  DetermineState(&STATE, analogRead(Joystick_X) - 512, analogRead(Joystick_Y) - 512);

  static Timer timer; // mantiene lo stato del timer tra i cicli


  // ===========================
  // MAIN MENU STATE MACHINE
  // ===========================
  switch (STATE)
  {
    case Idle:
      break;

    case ShowTimeAndDate:
      DisplayTimeDate();
      if (buttonClicked) {
        STATE = ChangeTimeDate;
        buttonClicked = false;
      }
      break;

    case ShowTempAndHum:
      DisplayDHT();
      break;

    case ShowAlarm:
      DisplayAlarm();
      if (buttonClicked) {
        STATE = changeAlarm;
        buttonClicked = false;
      }
      break;

    case ShowTimer:
      UpdateTimerDisplay(timer);
      if (buttonClicked) {
        STATE = changeTimer;
        buttonClicked = false;
      }
      break;

    case ChangeTimeDate:
      ChangeTimeAndDate();
      STATE = ShowTimeAndDate;
      break;

    case changeAlarm:
      ChangeAlarm();
      STATE = ShowAlarm;
      break;

    case changeTimer:
      ChangeTimer(timer);
      STATE = ShowTimer;
      break;
  }


  if (longPressAction) {
    switch (STATE) {
  
      case ShowAlarm:
        // Disable alarm
        RTC.clearAlarm(1);
        RTC.disableAlarm(1);
        alarmEnabled = false;
        break;
  
      case ShowTimer:
        if (timer.running) {
          timer.running = false;
          timer.done = false;
        }
        break;
  
      case ShowTimeAndDate:
        use12hFormat = !use12hFormat;
        break;
  
      case ShowTempAndHum:
        useFahrenheit = !useFahrenheit;
        break;
  
      default:
        break;
    }
  
    longPressAction = false;
  }
  
  

}
