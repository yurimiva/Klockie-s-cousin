#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <RTClib.h>
#include "pitches.h"
#include "button.h"
#include "nokia_melody.h"

// define the DHT
#define DHTPIN 13 // pin connected to DHT11 sensor
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// define the OLED
#define screen_width 128 // OLED display width, in pixels
#define screen_height 64 // OLED display height, in pixels
Adafruit_SSD1306 oled(screen_width, screen_height, &Wire, -1);

// define the joystick
const int VrX = 14;
const int VrY = 15;

// define the buttons
Button SW_Pin;       // switch pin
Button button_minus; // increases value
Button button_plus;  // decreases value

// define an rtc object
RTC_DS3231 RTC;
// define the PIN connected to SQW
const int CLOCK_INTERRUPT = 2;
volatile bool alarm = false; // need a global variable here so I won't mess up the interupt function

// define global variables for each parameter of the DateTime class
uint8_t hour;
uint8_t minutes;
uint8_t day = 1;
uint8_t month = 1;
uint16_t year;

// define global variables for the alarm
uint8_t alarm_h;
uint8_t alarm_m;

// define buzzer
const int Buzzer_Pin = 10;

// need to set an enumerable that helps me with the display of the modes

enum menu
{
  Idle,            // 0
  ShowTimeAndDate, // 1
  ShowTempAndHum,  // 2
  ChangeAlarm,     // 3
  StopAlarm,       // 4
  ChangeTimeDate,  // 5
  AlarmFiring
};

// ---

// ISR function
void onAlarm()
{

  alarm = true;
}

void setup()
{

  // initialize the dht sensor
  dht.begin();

  // initialize the oled
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  // coordinates for joystick
  pinMode(VrX, INPUT);
  pinMode(VrY, INPUT);

  // define mode for buttons (each is in mode INPUT_PULLUP)
  SW_Pin.begin(4);
  button_minus.begin(6);
  button_plus.begin(7);

  // buzzer
  pinMode(Buzzer_Pin, OUTPUT);

  // testing
  pinMode(LED_BUILTIN, OUTPUT);

  // initialize the RTC
  RTC.begin();
  // don't need the 32K Pin, so I simply didn't connect it

  // setup for the SQW PIN
  // Making it so, that the alarm will trigger an interrupt
  pinMode(CLOCK_INTERRUPT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT), onAlarm, FALLING);

  // set alarm 1 flag to false (so alarm 1 didn't happen so far)
  // if not done, this easily leads to problems, as thr register isn't reset on reboot/recompile
  RTC.disableAlarm(1);
  RTC.clearAlarm(1);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  RTC.writeSqwPinMode(DS3231_OFF);

  RTC.clearAlarm(2); // we won't use it, but still best to avoid problems
  RTC.disableAlarm(2);
}

void DisplayDHT(int temperature, int humidity)
{

  // how I want my oled
  oled.clearDisplay();

  // printing stuff
  oled.setCursor(0, 0);
  oled.print("  TEMP. & HUMIDITY");

  oled.setCursor(0, 25);
  oled.print(" TEMPERATURE : ");

  oled.setCursor(90, 25);
  oled.print(temperature);
  oled.print(" C ");

  oled.setCursor(0, 50);
  oled.print(" HUMIDITY  : ");

  oled.setCursor(90, 50);
  oled.print(humidity);
  oled.print(" % ");

  oled.display();
}

void DisplayTimeDate(DateTime previous)
{

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("  TIME AND DATE");

  oled.setCursor(30, 25);
  if (previous.hour() <= 9)
  {
    oled.print("0");
  }
  oled.print(previous.hour());

  oled.print(":");

  oled.setCursor(48, 25);
  if (previous.minute() <= 9)
  {
    oled.print("0");
  }
  oled.print(previous.minute());

  oled.setCursor(20, 50);
  if (previous.day() <= 9)
  {
    oled.print("0");
  }
  oled.print(previous.day());

  oled.print("/");

  oled.setCursor(38, 50);
  if (previous.month() <= 9)
  {
    oled.print("0");
  }
  oled.print(previous.month());

  oled.print("/");

  oled.setCursor(56, 50);
  oled.print(previous.year());

  oled.display();
}

int ExecuteChange(int base, int min_base, int max_base, int cursor_x, int cursor_y)
{

  while (!SW_Pin.debounce())
  {

    oled.drawFastHLine(cursor_x, (cursor_y - 5), 11, 1); // draws an horizontal line with the cursors from input
    oled.display();

    if (button_plus.debounce())
    {

      // animation for the change of the hour
      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1, 0);
      oled.print("  ");
      oled.display();

      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1);

      if (base >= 9)
      {

        if (base == max_base)
        {
          base = min_base;
          oled.print("0");
          oled.print(base);
          oled.display();
        }
        else
        {
          base += 1;
          oled.print(base);
          oled.display();
        }
      }
      else
      {

        oled.print("0");
        base += 1;
        oled.print(base);
        oled.display();
      }
    }

    if (button_minus.debounce())
    {

      // animation for the change of the hour
      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1, 0);
      oled.print("  ");
      oled.display();

      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1);

      if (base > 10)
      {

        base -= 1;
        oled.print(base);
        oled.display();
      }
      else
      {

        if (base == min_base)
        {
          base = max_base;
          oled.print(base);
          oled.display();
        }
        else
        {
          base -= 1;
          oled.print("0");
          oled.print(base);
          oled.display();
        }
      }
    }
  }

  if (SW_Pin.debounce())
  {
    return base;
  }
}

void ChangeTimeAndDate(DateTime need_change)
{

  // TIME //
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("TIME AND DATE SETTING");
  oled.setCursor(30, 25);
  if (need_change.hour() <= 9)
  {
    oled.print("0");
  }
  oled.print(need_change.hour());
  oled.print(":");
  if (need_change.minute() <= 9)
  {
    oled.print("0");
  }
  oled.print(need_change.minute());
  uint8_t current_hour = need_change.hour();
  uint8_t current_minutes = need_change.minute();

  // DATE //
  oled.setCursor(20, 50);
  if (need_change.day() <= 9)
  {
    oled.print("0");
  }
  oled.print(need_change.day());
  oled.print("/");
  if (need_change.month() <= 9)
  {
    oled.print("0");
  }
  oled.print(need_change.month());
  oled.print("/");
  oled.print(need_change.year());
  oled.display();

  uint8_t current_day = need_change.day();
  uint8_t current_month = need_change.month();

  hour = ExecuteChange(current_hour, 0, 23, 30, 25);
  minutes = ExecuteChange(current_minutes, 0, 59, 48, 25);
  day = ExecuteChange(current_day, 1, 31, 20, 50);
  month = ExecuteChange(current_month, 1, 12, 38, 50);
  year += ExecuteChange(0, 0, 99, 68, 50);
  RTC.adjust(DateTime(year, month, day, hour, minutes, 0));
}

void PrepareAlarm(DateTime my_alarm)
{

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("   ALARM SETTING");

  uint8_t hour_alarm = my_alarm.hour();
  uint8_t minutes_alarm = my_alarm.minute();

  oled.setCursor(30, 25);
  if (hour_alarm <= 9)
  {
    oled.print("0");
  }
  oled.print(hour_alarm);

  oled.print(":");

  if (minutes_alarm <= 9)
  {
    oled.print("0");
  }
  oled.print(minutes_alarm);
  oled.display();

  alarm_h = (uint8_t)ExecuteChange(hour_alarm, 0, 23, 30, 25);
  alarm_m = (uint8_t)ExecuteChange(minutes_alarm, 0, 59, 48, 25);
  
  RTC.setAlarm1(DateTime(0, 0, 0, alarm_h, alarm_m, 0), DS3231_A1_Hour);
}

void DisableAlarm(DateTime MY_alarm)
{

  while (true) {
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.print("WANT TO DISABLE ALARM?");

    oled.setCursor(30, 25);
    if (MY_alarm.hour() <= 9)
    {
      oled.print("0");
    }
    oled.print(MY_alarm.hour());

    oled.print(":");

    if (MY_alarm.minute() <= 9)
    {
      oled.print("0");
    }
    oled.print(MY_alarm.minute());

    oled.setCursor(10, 50);
    oled.print("YES (+)");
    if (button_plus.debounce())
    {
      RTC.disableAlarm(1);
      break;
    }

    oled.setCursor(80, 50);
    oled.print("NO (-)");
    if (button_minus.debounce())
    {
      RTC.setAlarm1(MY_alarm, DS3231_A1_Hour);
      break;
    }

    oled.display();
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

void loop()
{

  // Declare an object of class TimeDate
  DateTime alarm_display = RTC.getAlarm1();
  // this is for each change in parameters of DateTime
  DateTime now = RTC.now();// (DateTime(year, month, day, hour, minutes, 0));

  // read temp and hum
  int temperature = (int)dht.readTemperature();
  int humidity = (int)dht.readHumidity();

  oled.clearDisplay();

  // read coordinates
  int X_Value = analogRead(VrX);
  int Y_Value = analogRead(VrY);

  static menu STATE;
  static bool BEEPSTATE;

  if ((Y_Value < 312) && (150 < X_Value) && (X_Value < 874))
  { // up condition

    BeepOnce(&BEEPSTATE);
    STATE = ShowTimeAndDate;
  }
  else if ((Y_Value > 712) && (150 < X_Value) && (X_Value < 874))
  { // down condition

    // ALARM
    BeepOnce(&BEEPSTATE);
    STATE = ChangeAlarm;
  }
  else if ((X_Value < 312) && (150 < Y_Value) && (Y_Value < 874))
  { // left condition

    BeepOnce(&BEEPSTATE);
    STATE = StopAlarm;
  }
  else if ((X_Value > 712) && (150 < Y_Value) && (Y_Value < 874))
  { // right condition

    BeepOnce(&BEEPSTATE);
    STATE = ShowTempAndHum;
  }
  else
  {

    BEEPSTATE = false;
    if ((SW_Pin.debounce()) && (STATE == ShowTimeAndDate))
    {

      STATE = ChangeTimeDate;
    }
  }

  switch (STATE)
  {
  case Idle:

    break;
  case ShowTimeAndDate:
    DisplayTimeDate(now);
    break;
  case ShowTempAndHum:
    DisplayDHT(temperature, humidity);
    break;
  case ChangeAlarm:
    PrepareAlarm(alarm_display);
    STATE = ShowTimeAndDate;
    break;
  case StopAlarm:
    DisableAlarm(alarm_display);
    STATE = ShowTimeAndDate;
    break;
  case ChangeTimeDate:
    ChangeTimeAndDate(now);
    STATE = ShowTimeAndDate;
    break;
  }

  // this works out but only for a few seconds, I'll need to make simple and repetitive melody
  if (alarm)
  {
    STATE = AlarmFiring;
    oled.clearDisplay();
    RTC.alarmFired(1);
    oled.setCursor(0, 0);
    oled.print("  ALARM IS FIRING");
    oled.display();

    int size = sizeof(durations) / sizeof(int);

    for (int note = 0; note < size; note++)
    {
      // to calculate the note duration, take one second divided by the note type.
      // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int duration = 1000 / durations[note];
      tone(Buzzer_Pin, melody[note], duration);

      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = duration * 1.30;
      delay(pauseBetweenNotes);

      // stop the tone playing:
      noTone(Buzzer_Pin);
    }

    if (SW_Pin.debounce())
    { // if I move the jystick to the left
      // RTC.disableAlarm(1);
      RTC.clearAlarm(1);
      alarm = false;
      analogWrite(Buzzer_Pin, 0);
      STATE = ShowTimeAndDate;
    }
  }
}