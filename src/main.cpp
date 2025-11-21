#include <Wire.h>
#include <U8g2lib.h>
#include <DHTStable.h>
#include <RTClib.h>
#include <OneButton.h>
#include "pitches.h"
#include "melodies.h"

// define the DHT
#define DHTPIN 7 // pin connected to DHT11 sensor
DHTStable DHT;

// define the OLED (U8g2 version)
U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// define the joystick
const int Joystick_X = 14;
const int Joystick_Y = 15;

// let's add an enum to mark things more easily
enum Direction
{
  UP,
  DOWN,
  LEFT,
  RIGHT,
  CENTER
};

#define BUTTON_PIN 3
OneButton button(BUTTON_PIN, true, true); // pin, activeLow, pullup
volatile bool buttonClicked = false;
volatile bool longPressAction = false; // trigger per funzione extra

// variabili booleane per gestire la modalità delle interfacce con long press
bool useFahrenheit = false; // true → Fahrenheit
bool use12hFormat = false;  // true → formato 12h (AM/PM)
bool alarmEnabled = false;

// define an rtc object
RTC_DS3231 RTC;

// define the PIN connected to SQW
const int SQW_INTERRUPT = 2;
volatile bool rtcInterruptFlag = false;

// define buzzer
const int Buzzer_Pin = 11;

// ───────────────────────────────────────────
//     MELODIE NON BLOCCANTI  (TIMER2)
// ───────────────────────────────────────────

// Indica quale melodia suonare
enum MelodyType { NONE, ALARM_MELODY, TIMER_MELODY };
volatile MelodyType activeMelody = NONE;

// Stato della melodia
int currentNote = -1;
unsigned long noteStartTime = 0;
bool melodyPlaying = false;

// Segnali da ISR/bottone
volatile bool requestStartMelody = false;
volatile bool requestStopMelody = false;

volatile bool oneShotBeep = false;
unsigned long oneShotBeepStart = 0; // segno la frequenza del beep
const unsigned long oneShotBeepMs = 120; // durata beep

// struct usata per i campi da modificare nelle modalità change
struct Field {
  const char* fmt;   // formato printf, es. "%02d" oppure "AM"/"PM"
  int index;         // indice dentro values[], oppure -1 per testo fisso
  const char* sep;   // separatore dopo il campo, es ":" "/" " "
};

// a struct for the timer is needed since I'll treat it as a second alarm
struct Timer
{
  DateTime endTime; // momento in cui scade il timer
  bool running;     // true se countdown attivo
  bool done;        // true se scaduto
};

// need to set an enumerable that helps me with the display of the modes
enum menu
{
  Idle,            // fatto
  ShowTimeAndDate, // fatto
  ChangeTimeDate,  // fatto
  ShowTempAndHum,  // fatto
  ShowAlarm,       // fatto
  changeAlarm,     // fatto
  ShowTimer,       // sto facendo
  changeTimer      // sto facendo
};

// ---

// ISR function
void onRTCInterrupt()
{
  rtcInterruptFlag = true;
}

// ---
// Functions for the long and short press of the button
void onButtonClick()
{
  // Se la sveglia o il timer stanno suonando → ferma solo la melodia
  if (melodyPlaying) {
    StopMelody();
    return;
  }

  buttonClicked = true;
}

void onButtonLongPress()
{
  longPressAction = true; // funzione speciale, gestita nel loop
}

// setup
void setup()
{
  // Initialazing oled with U8g2
  oled.begin();

  // Mostra messaggio di setup
  oled.firstPage();
  do
  {
    oled.setFont(u8g2_font_6x12_tf); // font leggibile e piccolo
    oled.drawStr(0, 12, "Setup OK!");
  } while (oled.nextPage());
  delay(500);

  // coordinates for joystick
  pinMode(Joystick_X, INPUT);
  pinMode(Joystick_Y, INPUT);

  button.attachClick(onButtonClick);              // click breve
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
// need to setup Timer2 register to produce a square wavw with PWM and without using the CPU
void setupTimer2(uint16_t freq) {

  // this powers off the timer
  if (freq == 0) {
    TCCR2A = 0;
    TCCR2B = 0;
    return;
  }

  pinMode(11, OUTPUT);
  // this formula generates the tone
  uint32_t top = 16000000UL / (2UL * 64UL * freq) -1;

  TCCR2A = (1 << WGM21) | (1 << COM2A0);
  TCCR2B = (1 << CS22);
  OCR2A = top;

}

void StartMelody(MelodyType type)
{
  activeMelody = type;
  requestStartMelody = true;
}

void StopMelody()
{
  requestStopMelody = true;
}


void OneShotBeep(uint16_t freq) {

  // qui scegliamo: se non c'è una melodia attiva, useremo Timer2; altrimenti non beep.
  if (melodyPlaying || requestStartMelody) return; // evita conflitti

  // usa setupTimer2 per generare la frequenza e poi fermarla nel loop dopo N ms
  setupTimer2(freq);
  oneShotBeep = true;
  oneShotBeepStart = millis();
}

void BeepOnce(bool *beepstate)
{

  if (*beepstate == false) {
    *beepstate = true;
    OneShotBeep(1047); // RE1 ~ 1047 Hz
  }
  
}


Direction readJoystick(int x, int y)
{

  int deadzone = 100;

  // Deadzone check (joystick idle)
  if (abs(x) < deadzone && abs(y) < deadzone)
    return CENTER;

  // Compare absolute values to determine dominant direction
  if (abs(x) > abs(y))
  {
    return (x > 0) ? RIGHT : LEFT;
  }
  else
  {
    return (y < 0) ? UP : DOWN;
  }
}

void DetermineState(menu *state, int Joystick_X, int Joystick_Y)
{

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
  if (now - lastReadTime > 2000)
  {
    int chk = DHT.read11(DHTPIN);
    if (chk == DHTLIB_OK)
    {
      lastTemperature = (int)DHT.getTemperature();
      lastHumidity = (int)DHT.getHumidity();
    }
    lastReadTime = now;
  }

  int displayTemp = lastTemperature;
  const char *unit = "C";

  if (useFahrenheit)
  {
    displayTemp = (displayTemp * 1.8) + 32;
    unit = "F";
  }

  char buffer[32];
  oled.firstPage();
  do
  {
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 12, "TEMP. & HUMIDITY");

    snprintf(buffer, sizeof(buffer), "TEMP: %d %s", displayTemp, unit);
    oled.drawStr(0, 30, buffer);

    snprintf(buffer, sizeof(buffer), "HUM:  %d %%", lastHumidity);
    oled.drawStr(0, 46, buffer);
  } while (oled.nextPage());
}

void DisplayTimeDate()
{
  char buffer[32] = {0};
  DateTime now = RTC.now();

  int displayHour = now.hour();
  const char *ampm = "";

  if (use12hFormat)
  {
    if (displayHour == 0)
    {
      displayHour = 12;
      ampm = "AM";
    }
    else if (displayHour == 12)
    {
      ampm = "PM";
    }
    else if (displayHour > 12)
    {
      displayHour -= 12;
      ampm = "PM";
    }
    else
    {
      ampm = "AM";
    }
  }

  oled.firstPage();
  do
  {
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 12, "TIME AND DATE");

    // Ora
    oled.setFont(u8g2_font_logisoso16_tr);
    if (use12hFormat)
      snprintf(buffer, sizeof(buffer), "%02d:%02d %s", displayHour, now.minute(), ampm);
    else
      snprintf(buffer, sizeof(buffer), "%02d:%02d", now.hour(), now.minute());
    oled.drawStr(10, 36, buffer);

    // Data
    oled.setFont(u8g2_font_6x12_tf);
    if (use12hFormat) {
      snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", now.month(), now.day(), now.year());
    } else {
      snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", now.day(), now.month(), now.year());
    }
      
    oled.drawStr(20, 56, buffer);
  } while (oled.nextPage());
}

void DisplayAlarm()
{
  oled.firstPage();
  do
  {
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 12, "ALARM");

    if (!alarmEnabled)
    {
      oled.drawStr(20, 40, "NO ALARM SET");
    }
    else
    {
      DateTime alarm1 = RTC.getAlarm1();
      char buf[16];
      snprintf(buf, sizeof(buf), "%02d:%02d", alarm1.hour(), alarm1.minute());
      oled.drawStr(30, 40, buf);
    }

  } while (oled.nextPage());
}

// Questa funzione va chiamata ogni ciclo del loop per aggiornare il display
void UpdateTimerDisplay(Timer &t)
{
  DateTime now = RTC.now();

  char buf[16];

  oled.firstPage();
  do
  {
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 12, "TIMER");

    if (!t.running && !t.done)
    {
      oled.drawStr(20, 40, "No timer set");
    }
    else
    {
      TimeSpan remaining = t.endTime - now;

      if (remaining.totalseconds() <= 0)
      {
        t.running = false;
        t.done = true;
        remaining = TimeSpan(0);
      }

      snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
               remaining.hours(), remaining.minutes(), remaining.seconds());
      oled.drawStr(20, 40, buf);
    }
  } while (oled.nextPage());
}



// ALL CHANGE FUNCTIONS
void drawFieldDynamic(int x, int y, const char* text, bool selected) {
  int w = oled.getStrWidth(text);

  if (selected) {
    oled.setDrawColor(1);
    oled.drawBox(x - 1, y - 10, w + 3, 12);
    oled.setDrawColor(0);
    oled.drawStr(x, y, text);
    oled.setDrawColor(1);
  } else {
    oled.drawStr(x, y, text);
  }
}

void SetFields_6(int* values, Field* fields, int &n) {
  fields[n++] = {"%02d", 0, ":"};
  fields[n++] = {"%02d", 1, " "};
  fields[n++] = {values[2] ? "PM" : "AM", -1, " "};
  fields[n++] = {"%02d", 3, "/"};
  fields[n++] = {"%02d", 4, "/"};
  fields[n++] = {"%04d", 5, ""};
}

void SetFields_5(int* values, Field* fields, int &n) {
  fields[n++] = {"%02d", 0, ":"};
  fields[n++] = {"%02d", 1, " "};
  fields[n++] = {"%02d", 2, "/"};
  fields[n++] = {"%02d", 3, "/"};
  fields[n++] = {"%04d", 4, ""};
}

void SetFields_3(int* values, Field* fields, int &n) {
  fields[n++] = {"%02d", 0, ":"};
  fields[n++] = {"%02d", 1, ":"};
  fields[n++] = {"%02d", 2, ""};
}

void SetFields_2(int* values, Field* fields, int &n) {
  fields[n++] = {"%02d", 0, ":"};
  fields[n++] = {"%02d", 1, ""};
}

void DisplayChange(const char* title, int* values, int numFields, int index) {
  static Field fields[8];  // array di 8 campi (abbastanza per qualsiasi formato)
  int n = 0;  // Indice per aggiungere i campi

  // Mappiamo numFields sulla funzione corretta
  switch (numFields) {
    case 6:
      SetFields_6(values, fields, n);
      break;
    case 5:
      SetFields_5(values, fields, n);
      break;
    case 3:
      SetFields_3(values, fields, n);
      break;
    case 2:
      SetFields_2(values, fields, n);
      break;
    default:
      // Gestione di errore, nel caso il numero di campi non sia supportato
      return;
  }

  // Disegna effettivamente i campi
  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tf);
    oled.drawStr(0, 12, title);

    int x = 0;
    int y = 36;
    char buf[8];

    for (int i = 0; i < n; i++) {
      const Field& f = fields[i];
      
      // Se il campo viene da values[]
      if (f.index >= 0) {
        snprintf(buf, sizeof(buf), f.fmt, values[f.index]);
        drawFieldDynamic(x, y, buf, (f.index == index));
        x += oled.getStrWidth(buf);
      } else {
        // Campo testuale fisso (AM/PM)
        drawFieldDynamic(x, y, f.fmt, false);
        x += oled.getStrWidth(f.fmt);
      }

      // Separatore
      if (strlen(f.sep) > 0) {
        oled.drawStr(x + 2, y, f.sep);
        x += oled.getStrWidth(f.sep) + 4;
      }
    }

  } while (oled.nextPage());
}

void ChangeValue(int* values, int* minVals, int* maxVals, int numFields, const char* title)
{
  int index = 0;
  bool done = false;

  unsigned long lastMove = 0;
  const unsigned long repeatDelay = 140;

  while (!done) {

    unsigned long now = millis();
    Direction dir = readJoystick(analogRead(Joystick_X) - 512, analogRead(Joystick_Y) - 512);

    if (now - lastMove >= repeatDelay) {
      switch (dir) {

      case UP:
        values[index]++;
        if (values[index] > maxVals[index])
        values[index] = minVals[index];
        lastMove = now;
        break;

      case DOWN:
        values[index]--;
        if (values[index] < minVals[index])
        values[index] = maxVals[index];
        lastMove = now;
        break;

      case RIGHT:
        index++;
        if (index >= numFields) done = true;
        lastMove = now;
        break;

      case LEFT:
        if (index > 0) index--;
        lastMove = now;
        break;

      default:
        break;
      }
    }

    // Aggiorna GUI
    DisplayChange(title, values, numFields, index);

    if (done) {
      oled.firstPage();
      do {
        oled.setFont(u8g2_font_6x12_tf);
        oled.drawStr(25, 35, "Saved!");
      } while (oled.nextPage());

    delay(500);
    return;
    }
  }
}

// Funzione per aggiungere un campo con minVal e maxVal
void addField(int &fields, int *values, int *minVal, int *maxVal, int value, int minValue, int maxValue) {
  values[fields] = value;
  minVal[fields] = minValue;
  maxVal[fields] = maxValue;
  fields++;
}

bool isLeapYear(int year) {
  // Un anno è bisestile se è divisibile per 4, ma non per 100, salvo che sia divisibile per 400
  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

int getMaxDaysInMonth(int month, int year) {
  // Restituisce il numero massimo di giorni per un mese specifico dell'anno
  switch (month) {
    case 1:  // Gennaio
    case 3:  // Marzo
    case 5:  // Maggio
    case 7:  // Luglio
    case 8:  // Agosto
    case 10: // Ottobre
    case 12: // Dicembre
      return 31;
    case 4:  // Aprile
    case 6:  // Giugno
    case 9:  // Settembre
    case 11: // Novembre
      return 30;
    case 2:  // Febbraio
      return isLeapYear(year) ? 29 : 28;
    default:
      return 0; // Mese non valido
  }
}

bool isValidDate(int year, int month, int day) {
  // Verifica che il mese sia tra 1 e 12
  if (month < 1 || month > 12) return false;

  // Verifica che il giorno sia tra 1 e il numero massimo di giorni per quel mese
  int maxDays = getMaxDaysInMonth(month, year);
  if (day < 1 || day > maxDays) return false;

  return true;
}

void ChangeTimeAndDate() {
  DateTime now = RTC.now();

  // ---- ORA ----
  int hour = now.hour();
  int minute = now.minute();
  int isPM = 0;

  // Gestione AM/PM se formato 12h
  if (use12hFormat) {
    if (hour == 0) {
      hour = 12;  // Midnight
      isPM = 0;
    } else if (hour == 12) {
      isPM = 1;   // Noon
    } else if (hour > 12) {
      hour -= 12; // Convert to 12h format
      isPM = 1;
    }
  }

  // ---- DATA ----
  int day = now.day();
  int month = now.month();
  int year = now.year();

  // ---- Composizione dei campi ----
  int values[6];
  int minVal[6];
  int maxVal[6];
  int fields = 0;

  // Ora
  addField(fields, values, minVal, maxVal, hour, use12hFormat ? 1 : 0, use12hFormat ? 12 : 23);

  // Minuti
  addField(fields, values, minVal, maxVal, minute, 0, 59);

  // AM/PM (solo se formato 12h)
  if (use12hFormat) {
    addField(fields, values, minVal, maxVal, isPM, 0, 1);
  }

  // Data
  if (use12hFormat) {
    // Formato M / D / Y
    addField(fields, values, minVal, maxVal, month, 1, 12);
    addField(fields, values, minVal, maxVal, day, 1, 31);
  } else {
    // Formato D / M / Y
    addField(fields, values, minVal, maxVal, day, 1, 31);
    addField(fields, values, minVal, maxVal, month, 1, 12);
  }

  // Anno
  addField(fields, values, minVal, maxVal, year, 2000, 2099);

  // ---- Editor generico ----
  ChangeValue(values, minVal, maxVal, fields, "SET DATE/TIME");

  // ---- RICONVERSIONE ORA ----
  int newHour = values[0];
  int newMin = values[1];

  if (use12hFormat) {
    int newPM = values[2];
    if (newHour == 12) newHour = 0; // 12 AM/PM handling
    if (newPM) newHour += 12;       // Convert PM to 24h format
  }

  // ---- RICONVERSIONE DATA ----
  int newDay, newMonth;
  int idx = use12hFormat ? 3 : 2;

  if (use12hFormat) {
    // M/D/Y format
    newMonth = values[idx];
    newDay = values[idx + 1];
  } else {
    // D/M/Y format
    newDay = values[idx];
    newMonth = values[idx + 1];
  }

  int newYear = values[fields - 1];

  // Verifica se la data è valida
  if (!isValidDate(newYear, newMonth, newDay)) {
    // Mostra un messaggio di errore
    oled.clearBuffer();
    oled.setFont(u8g2_font_ncenB08_tr);
    oled.drawStr(10, 30, "INVALID DATE!");
    oled.sendBuffer();
    delay(1000);  // Mostra il messaggio per 1 secondo
    return;       // Non aggiornare l'orologio
  }

  // Set RTC solo se la data è valida
  RTC.adjust(DateTime(newYear, newMonth, newDay, newHour, newMin, 0));
}

void ChangeAlarm()
{

  DateTime alarm = RTC.getAlarm2();

  int values[2] = {alarm.hour(), alarm.minute()};
  int minVals[2] = {0, 0};
  int maxVals[2] = {23, 59};

  ChangeValue(values, minVals, maxVals, 2, "SET ALARM");

  RTC.setAlarm2(DateTime(0, 0, 0, values[0], values[1], 0), DS3231_A2_Minute);
  alarmEnabled = true;
}

void ChangeTimer(Timer &t)
{
  int values[3] = {0, 0, 0};      // HH, MM, SS
  int minVals[3] = {0, 0, 0};
  int maxVals[3] = {23, 59, 59};

  // input dell’utente con cursore
  ChangeValue(values, minVals, maxVals, 3, "SET TIMER");

  // calcola momento di fine timer
  t.endTime = RTC.now() + TimeSpan(0, values[0], values[1], values[2]);
  t.running = true;
  t.done = false;

  // ATTENZIONE: Alarm2 NON supporta i secondi.
  // Quindi lo settiamo solo a livello di ORA/MINUTO.
  RTC.setAlarm1(t.endTime, DS3231_A1_Date);

  // feedback breve
  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  oled.drawStr(10, 30, "TIMER STARTED");
  oled.sendBuffer();
  delay(500);
}


// Functions for handling the alarms

// Funzione per suonare melodia interrompibile dal bottone
void UpdateMelody()
{
  // richiesta di stop
  if (requestStopMelody && melodyPlaying) {
    melodyPlaying = false;
    setupTimer2(0);
    requestStopMelody = false;
    activeMelody = NONE;
    return;
  }

  // richiesta di start
  if (requestStartMelody) {
    requestStartMelody = false;
    requestStopMelody = false;

    melodyPlaying = true;
    currentNote = 0;
    noteStartTime = millis();

    if (activeMelody == ALARM_MELODY)
      setupTimer2(alarm_melody[currentNote]);
    else
      setupTimer2(timer_melody[currentNote]);

    return;
  }

  if (!melodyPlaying) return;

  // avanzamento melodia
  unsigned long now = millis();

  int duration;
  if (activeMelody == ALARM_MELODY)
    duration = alarm_durations[currentNote];
  else
    duration = timer_durations[currentNote];

  if (now - noteStartTime >= duration) {
    currentNote++;

    int length = (activeMelody == ALARM_MELODY)
        ? sizeof(alarm_melody)/sizeof(int)
        : sizeof(timer_melody)/sizeof(int);

    if (currentNote >= length) {
      currentNote = 0;   // ricomincia melodia da capo
    }

    if (activeMelody == ALARM_MELODY)
      setupTimer2(alarm_melody[currentNote]);
    else
      setupTimer2(timer_melody[currentNote]);

    noteStartTime = now;
  }
}

void StartMelody(MelodyType type)
{
  activeMelody = type;
  requestStartMelody = true;

  oled.firstPage();
  do {
    oled.setFont(u8g2_font_6x12_tf);
    if (type == ALARM_MELODY)
      oled.drawStr(10, 30, "ALARM RINGING!");
    else if (type == TIMER_MELODY)
      oled.drawStr(10, 30, "TIMER DONE!");
  } while (oled.nextPage());
}

void HandleAlarms() {

  rtcInterruptFlag = false;

  if (RTC.alarmFired(1)) {
    StartMelody(TIMER_MELODY);
    RTC.clearAlarm(1);
  }

  if (RTC.alarmFired(2)) {
    StartMelody(ALARM_MELODY);
    RTC.clearAlarm(2);
  } 

}

void loop()
{
  UpdateMelody();
  button.tick();

   // stop one-shot beep dopo la durata
  if (oneShotBeep) {
    if (millis() - oneShotBeepStart >= oneShotBeepMs) {
      setupTimer2(0); // ferma Timer2
      oneShotBeep = false;
    }
  }


  if (rtcInterruptFlag) {
    HandleAlarms();
  }

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
    if (buttonClicked)
    {
      STATE = ChangeTimeDate;
      buttonClicked = false;
    }
    break;

  case ShowTempAndHum:
    DisplayDHT();
    break;

  case ShowAlarm:
    DisplayAlarm();
    if (buttonClicked)
    {
      STATE = changeAlarm;
      buttonClicked = false;
    }
    break;

  case ShowTimer:
    UpdateTimerDisplay(timer);
    if (buttonClicked)
    {
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

  if (longPressAction)
  {
    switch (STATE)
    {

    case ShowAlarm:
      // Disable alarm
      RTC.clearAlarm(1);
      RTC.disableAlarm(1);
      alarmEnabled = false;
      break;

    case ShowTimer:
      if (timer.running)
      {
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
