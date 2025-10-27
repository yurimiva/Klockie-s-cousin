#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTStable.h>
#include <RTClib.h>
#include "pitches.h"
#include "button.h"
#include "nokia_melody.h"

// define the DHT
#define DHTPIN 7 // pin connected to DHT11 sensor
DHTStable DHT;

// define the OLED
#define screen_width 128 // OLED display width, in pixels
#define screen_height 64 // OLED display height, in pixels
Adafruit_SSD1306 oled(screen_width, screen_height, &Wire, -1);

// define the joystick
const int Joystick_X = 14;
const int Joystick_Y = 15;

// let's add an enum to mark things more easily
enum Direction { UP, DOWN, LEFT, RIGHT, CENTER };

// define the button
Button SW_Pin; // switch pin

// define an rtc object
RTC_DS3231 RTC;

// define the PIN connected to SQW
const int SQW_INTERRUPT = 2;
volatile bool alarm1Triggered = false;
volatile bool alarm2Triggered = false;

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
void onAlarm() {
  if (RTC.alarmFired(1)) alarm1Triggered = true;
  if (RTC.alarmFired(2)) alarm2Triggered = true;
}

void setup()
{
  // initialize the oled
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  // coordinates for joystick
  pinMode(Joystick_X, INPUT);
  pinMode(Joystick_Y, INPUT);

  // define mode for button in mode INPUT_PULLUP
  SW_Pin.begin(4);

  // buzzer
  pinMode(Buzzer_Pin, OUTPUT);

  // initialize the RTC
  RTC.begin();
  // don't need the 32K Pin, so I simply didn't connect it

  // setup for the SQW PIN
  // Making it so, that the alarm will trigger an interrupt
  pinMode(SQW_INTERRUPT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SQW_INTERRUPT), onAlarm, FALLING);

  // set alarm 1 flag to false (so alarm 1 didn't happen so far)
  // if not done, this easily leads to problems, as the register isn't reset on reboot/recompile
  RTC.clearAlarm(1);
  RTC.clearAlarm(2);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  RTC.writeSqwPinMode(DS3231_OFF);
}


// UTILITY FUNCTIONS
Direction readJoystick(int x, int y) {

  int deadzone = 100;

  // Deadzone check (joystick idle)
  if (abs(x) < deadzone && abs(y) < deadzone)
    return CENTER;

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
  default:
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

  char buffer[32];

  // Titolo
  oled.clearDisplay();
  snprintf(buffer, sizeof(buffer), "  TEMP. & HUMIDITY");
  oled.setCursor(0, 0);
  oled.print(buffer);

  // Temperatura
  snprintf(buffer, sizeof(buffer), " TEMPERATURE : %d C", lastTemperature);
  oled.setCursor(0, 25);
  oled.print(buffer);

  // Umidità
  snprintf(buffer, sizeof(buffer), " HUMIDITY    : %d %%", lastHumidity);
  oled.setCursor(0, 50);
  oled.print(buffer);

  // Invio al display
  oled.display();

}


void DisplayTimeDate()
{
  char buffer[32] = {0}; // this buffer is used for copying a certain format and then actual printing
  // this is for each change in parameters of DateTime
  DateTime now = RTC.now(); // (DateTime(year, month, day, hour, minutes, 0));

  snprintf(buffer, sizeof(buffer), "%02d:%02d\n%02d/%02d/%04d",
             now.hour(), now.minute(),
             now.day(), now.month(), now.year());

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("  TIME AND DATE");
 
  oled.setCursor(20, 25);
  oled.print(buffer);

  oled.display();

}


void DisplayAlarm() {

  // Leggi i valori correnti della sveglia (Alarm1)
  DateTime alarm1 = RTC.getAlarm1();
  
  // Mostra su OLED
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("     ALARM");
  
  oled.setCursor(20, 25);
  char buf[10] = {0};
  snprintf(buf, sizeof(buf), "%02d:%02d", alarm1.hour(), alarm1.minute());
  oled.print(buf);
  
  oled.display();

}


// Questa funzione va chiamata ogni ciclo del loop per aggiornare il display
void UpdateTimerDisplay(Timer &t) {
  DateTime now = RTC.now();

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("   TIMER");

  if (!t.running && !t.done) {
    // nessun timer impostato
    oled.setCursor(20, 25);
    oled.print("No timer set");
  } else {
    TimeSpan remaining = t.endTime - now;

    if (remaining.totalseconds() <= 0) {
      t.running = false;
      t.done = true;
      tone(Buzzer_Pin, 1047); // beep
      remaining = TimeSpan(0); // evita valori negativi
    }

    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             remaining.hours(), remaining.minutes(), remaining.seconds());
    oled.setCursor(20, 25);
    oled.print(buf);
  }

  oled.display();
}



// ALL CHANGE FUNCTIONS
void DisplayChange(const char* title, int* values, int numFields) {

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print(title);

  oled.setCursor(10, 25);

  char buf[40];
  buf[0] = '\0';  // azzera il buffer

  // Aggiunge i campi in base a quanti ne servono
  switch (numFields) {
    case 2: // es. Sveglia, Timer
      snprintf(buf, sizeof(buf), "%02d:%02d", values[0], values[1]);
      break;
    case 3: // es. Timer con secondi
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", values[0], values[1], values[2]);
      break;
    case 5: // es. Ora e data completa
      snprintf(buf, sizeof(buf), "%02d:%02d \n %02d/%02d/%04d",
               values[0], values[1], values[2], values[3], values[4]);
      break;
    default:
      snprintf(buf, sizeof(buf), "ERR");
      break;
  }

  oled.print(buf);
  oled.display();
}


void ChangeValue(int* values, int* minVals, int* maxVals, int numFields, const char* title) {

  int index = 0;
  bool done = false;

  while (!done) {

    Direction dir = readJoystick(analogRead(Joystick_X) - 512, analogRead(Joystick_Y) - 512);

    switch (dir)
    {
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
    }

    DisplayChange(title, values, numFields); // funzione esterna che mostra il dato su OLED

    if (index >= numFields) {
      done = true;
      break;
    }
    delay(150);
  }

  // Piccola animazione di conferma
  oled.clearDisplay();
  oled.setCursor(10, 25);
  oled.print("Saved!");
  oled.display();
  delay(500);

}


void ChangeTimeAndDate() {

  DateTime now = RTC.now();
  int valori[5] = { now.hour(), now.minute(), now.day(), now.month(), now.year() };
  int minVal[5] = { 0, 0, 1, 1, 2000 };
  int maxVal[5] = { 23, 59, 31, 12, 2099 };

  ChangeValue(valori, minVal, maxVal, 5, "CHANGE TIME AND DATE");

  RTC.adjust(DateTime(valori[4], valori[3], valori[2], valori[0], valori[1], 0));
}


void ChangeAlarm() {

  DateTime alarm = RTC.getAlarm1();

  int values[2]  = {alarm.hour(), alarm.minute()};
  int minVals[2] = {0, 0};
  int maxVals[2] = {23, 59};

  ChangeValue(values, minVals, maxVals, 2, "SET ALARM");

  RTC.setAlarm1(DateTime(0, 0, 0, values[0], values[1], 0), DS3231_A1_Hour);

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
  oled.clearDisplay();
  oled.setCursor(20, 25);
  oled.print("TIMER STARTED");
  oled.display();
  delay(500);
}

// --------
// Functions for handling the alarms
void PlayMelody(const int *melody, const int *durations, int length) {
  for (int i = 0; i < length; i++) {
    int noteDuration = 1000 / durations[i];
    tone(Buzzer_Pin, melody[i], noteDuration);
    delay(noteDuration * 1.3);
  }
  noTone(Buzzer_Pin);
}


void HandleWakeup() {
  oled.clearDisplay();
  oled.setCursor(10, 20);
  oled.print("ALARM RINGING!");
  oled.display();

  // Esegui melodia sveglia finché non viene premuto il joystick
  while (!SW_Pin.debounce()) {
    PlayMelody(alarm_melody, alarm_NoteDurations);
  }

  noTone(Buzzer_Pin);
  RTC.disableAlarm(1); // disattiva l’allarme fino a nuovo set
  oled.clearDisplay();
  oled.setCursor(10, 25);
  oled.print("ALARM STOPPED");
  oled.display();
  delay(1000);
}


void HandleTimerDone() {
  oled.clearDisplay();
  oled.setCursor(10, 20);
  oled.print("TIMER DONE!");
  oled.display();

  // Melodia del timer finché non premi joystick
  while (!SW_Pin.debounce()) {
    PlayMelody(timer_melody, timer_noteDurations);
  }

  noTone(Buzzer_Pin);
  RTC.disableAlarm(2); // disattiva il timer
  oled.clearDisplay();
  oled.setCursor(10, 25);
  oled.print("TIMER STOPPED");
  oled.display();
  delay(1000);
}


void HandleAlarms() {
  if (alarm1Triggered) {
    alarm1Triggered = false;
    RTC.clearAlarm(1);
    HandleWakeup();  // funzione separata per la sveglia
  }

  if (alarm2Triggered) {
    alarm2Triggered = false;
    RTC.clearAlarm(2);
    HandleTimerDone();  // funzione separata per il timer
  }
}



void loop()
{

  HandleAlarms();
  oled.clearDisplay();

  static menu STATE = Idle;
  // read coordinates to determine the state
  int x = analogRead(Joystick_X) - 512; // Center around 0
  int y = analogRead(Joystick_Y) - 512;
  DetermineState(&STATE, x, y);

  static Timer timer; // mantiene lo stato del timer tra i cicli


  switch (STATE)
  {
  case Idle:
    break;

  case ShowTimeAndDate:
    DisplayTimeDate();
    if (SW_Pin.debounce()) STATE = ChangeTimeDate;
    break;

  case ShowTempAndHum:
    DisplayDHT();
    break;

  case ShowAlarm:
    DisplayAlarm();
    if (SW_Pin.debounce()) STATE = changeAlarm;
    break;

  case ShowTimer:
    UpdateTimerDisplay(timer);
    if (SW_Pin.debounce()) STATE = changeTimer;
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
}