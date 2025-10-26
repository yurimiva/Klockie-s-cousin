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
const int CLOCK_INTERRUPT = 2;
volatile bool alarm = false; // need a global variable here so I won't mess up the interrupt function

// define buzzer
const int Buzzer_Pin = 10;

// need to set an enumerable that helps me with the display of the modes

enum menu
{
  Idle, // fatto
  ShowTimeAndDate, // fatto
  ChangeTimeDate, // fatto
  ShowTempAndHum, // fatto
  ShowAlarm, // fatto
  changeAlarm, // fatto
  StopAlarm, // da fare
  ShowTimer, // sto facendo
  changeTimer // sto facendo
};

// ---

// ISR function
void onAlarm()
{

  alarm = true;
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
  pinMode(CLOCK_INTERRUPT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT), onAlarm, FALLING);

  // set alarm 1 flag to false (so alarm 1 didn't happen so far)
  // if not done, this easily leads to problems, as the register isn't reset on reboot/recompile
  RTC.disableAlarm(1);
  RTC.clearAlarm(1);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  RTC.writeSqwPinMode(DS3231_OFF);

  RTC.clearAlarm(2); // we won't use it, but still best to avoid problems
  RTC.disableAlarm(2);
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

// TODO: capire come mai il display di temp e hum è bloccante
void DisplayDHT()
{

  int chk = DHT.read11(DHTPIN);
  // Leggi temperatura e umidità
  int temperature = (int)DHT.getTemperature();
  int humidity = (int)DHT.getHumidity();

  char buffer[32];

  // Titolo
  oled.clearDisplay();
  snprintf(buffer, sizeof(buffer), "  TEMP. & HUMIDITY");
  oled.setCursor(0, 0);
  oled.print(buffer);

  // Temperatura
  snprintf(buffer, sizeof(buffer), " TEMPERATURE : %d C", temperature);
  oled.setCursor(0, 25);
  oled.print(buffer);

  // Umidità
  snprintf(buffer, sizeof(buffer), " HUMIDITY    : %d %%", humidity);
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

// ------ Need to change stuff under there

DateTime timerEnd;
bool timerRunning = false;


// 2️⃣ Calcola momento di scadenza
void StartTimer(int hours, int minutes) {
  DateTime now = RTC.now();
  timerEnd = now + TimeSpan(hours, minutes, 0, 0); // somma ore e minuti
  timerRunning = true;
}


// 1️⃣ Imposta il timer
void ChangeTimer() {
    int h = 0;
    int m = 0;
    bool done = false;
    int index = 0;

    while(!done) {
        Direction dir = readJoystick(analogRead(Joystick_X)-512, analogRead(Joystick_Y)-512);

        switch(dir) {
            case UP:    if(index==0) h = (h>=23)?0:h+1; else m = (m>=59)?0:m+1; break;
            case DOWN:  if(index==0) h = (h<=0)?23:h-1; else m = (m<=0)?59:m-1; break;
            case RIGHT:
            case LEFT:  index = (index+1)%2; break;
            default: break;
        }

        // Stampa sul display
        oled.clearDisplay();
        oled.setCursor(20,25);
        char buf[8];
        snprintf(buf,sizeof(buf),"%02d:%02d", h, m);
        oled.print(buf);
        oled.display();

        if(SW_Pin.debounce()) done = true;
        delay(50);
    }

    // Avvia il timer
    StartTimer(h, m);
}

// 3️⃣ Aggiorna il timer sul display
void UpdateTimerDisplay() {
    if(!timerRunning) return;

    DateTime now = RTC.now();
    TimeSpan remaining = timerEnd - now;

    oled.clearDisplay();
    oled.setCursor(20,25);

    if(remaining.totalseconds() <= 0) {
        oled.print("TIMER DONE!");
        tone(Buzzer_Pin, 1047, 500); // beep
        timerRunning = false;
    } else {
        int h = remaining.hours();
        int m = remaining.minutes();
        int s = remaining.seconds();
        char buf[9];
        snprintf(buf,sizeof(buf),"%02d:%02d:%02d", h, m, s);
        oled.print(buf);
    }

    oled.display();
}

// --------

void loop()
{

  oled.clearDisplay();

  static menu STATE = Idle;
  // read coordinates to determine the state
  int x = analogRead(Joystick_X) - 512; // Center around 0
  int y = analogRead(Joystick_Y) - 512;
  DetermineState(&STATE, x, y);

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
    delay(150);
    break;

  case ShowAlarm:
    DisplayAlarm();
    if (SW_Pin.debounce()) STATE = changeAlarm;
    break;

  case StopAlarm:
    STATE = ShowTimeAndDate;
    break;

  case ChangeTimeDate:
    ChangeTimeAndDate();
    STATE = ShowTimeAndDate;
    break;

  case changeAlarm:
    ChangeAlarm();
    STATE = ShowAlarm;
    break;

  case ShowTimer:
    UpdateTimerDisplay();
    break;

  case changeTimer:
    ChangeTimer();
    STATE = ShowTimer;
    break;
  }
}