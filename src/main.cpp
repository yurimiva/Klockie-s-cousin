#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <RTClib.h>
#include "pitches.h"
#include "button.h"
#include "nokia_melody.h"
#include <time.h>

// define the DHT
#define DHTPIN 13 // pin connected to DHT11 sensor
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// define the OLED
#define screen_width 128 // OLED display width, in pixels
#define screen_height 64 // OLED display height, in pixels
Adafruit_SSD1306 oled(screen_width, screen_height, &Wire, -1);

// define the joystick
const int Joystick_X = 14;
const int Joystick_Y = 15;

// TODO; verificare che qualcuno a cui spiego questa cosa capisca
// declare constants necessary to determine which interface to show to the user
// with how analogic inputs work, we have values between 0 and 1023

// it goes from top (0) to bottom (1023)
// and left (0) to right (1023)

// the upper area will be between 0 and 255
const int upper_area_limit = 255;
// lower area will be between 769 and 1023
const int lower_area_limit = 769;
// left between 0 and 255
const int left_area_limit = 255;
// right between 769 and 1023
const int right_area_limit = 769;

// define the button
Button SW_Pin; // switch pin

// define an rtc object
RTC_DS3231 RTC;
// define the PIN connected to SQW
const int CLOCK_INTERRUPT = 2;
volatile bool alarm = false; // need a global variable here so I won't mess up the interrupt function

// define global variables for the alarm
uint8_t alarm_h;
uint8_t alarm_m;

// define buzzer
const int Buzzer_Pin = 10;

// need to set an enumerable that helps me with the display of the modes

enum menu
{
  Idle,
  ShowTimeAndDate,
  ShowTempAndHum,
  ChangeAlarm,
  StopAlarm,
  ChangeTimeDate,
  // AlarmFiring
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
  pinMode(Joystick_X, INPUT);
  pinMode(Joystick_Y, INPUT);

  // define mode for button in mode INPUT_PULLUP
  SW_Pin.begin(4);

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
  // if not done, this easily leads to problems, as the register isn't reset on reboot/recompile
  RTC.disableAlarm(1);
  RTC.clearAlarm(1);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  RTC.writeSqwPinMode(DS3231_OFF);

  RTC.clearAlarm(2); // we won't use it, but still best to avoid problems
  RTC.disableAlarm(2);
}

void DisplayDHT()
{

  // read temp and hum
  int temperature = (int)dht.readTemperature();
  int humidity = (int)dht.readHumidity();

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

// TODO: test strftime
void DisplayTimeDate()
{

  char buffer[80]; // this buffer is used for the strftime
  // this is for each change in parameters of DateTime
  DateTime now = RTC.now(); // (DateTime(year, month, day, hour, minutes, 0));

  struct tm date_and_time;

  date_and_time.tm_hour = now.hour();
  date_and_time.tm_min = now.minute();
  date_and_time.tm_mday = now.day();
  date_and_time.tm_mon = now.month() - 1;
  date_and_time.tm_year = now.year() - 1900;

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("  TIME AND DATE");

  oled.setCursor(20, 25);
  strftime(buffer, 80, "%H:%M \n %d/%m/%Y", &date_and_time);
  oled.print(buffer);

  oled.display();
}

// TODO: do a snprintf
/*
int ExecuteChange(int value, int min_value, int max_value, int cursor_x, int cursor_y)
{

  while (!SW_Pin.debounce())
  {

    oled.drawFastHLine(cursor_x, (cursor_y - 5), 11, 1); // draws an horizontal line with the cursors from input
    oled.display();

    if ()
    {

      // animation for the change of the hour
      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1, 0);
      oled.print("  ");
      oled.display();

      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1);

      // TODO: snprintf
      if (value >= 9)
      {

        if (value == max_value)
        {
          value = min_value;
          oled.print("0");
          oled.print(value);
          oled.display();
        }
        else
        {
          value += 1;
          oled.print(value);
          oled.display();
        }
      }
      else
      {

        oled.print("0");
        value += 1;
        oled.print(value);
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

      if (value > 10)
      {

        value -= 1;
        oled.print(value);
        oled.display();
      }
      else
      {

        if (value == min_value)
        {
          value = max_value;
          oled.print(value);
          oled.display();
        }
        else
        {
          value -= 1;
          oled.print("0");
          oled.print(value);
          oled.display();
        }
      }
    }
  }

  if (SW_Pin.debounce())
  {
    return value;
  }
}

*/

// TODO: use snprintf
/*
void ChangeTimeAndDate()
{

  // this is for each change in parameters of DateTime
  DateTime now = RTC.now(); // (DateTime(year, month, day, hour, minutes, 0));

  // TIME //
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("TIME AND DATE SETTING");
  oled.setCursor(30, 25);
  if (now.hour() <= 9)
  {
    oled.print("0");
  }
  oled.print(now.hour());
  oled.print(":");
  if (now.minute() <= 9)
  {
    oled.print("0");
  }
  oled.print(now.minute());
  uint8_t current_hour = now.hour();
  uint8_t current_minutes = now.minute();

  // DATE //
  oled.setCursor(20, 50);
  if (now.day() <= 9)
  {
    oled.print("0");
  }
  oled.print(now.day());
  oled.print("/");
  if (now.month() <= 9)
  {
    oled.print("0");
  }
  oled.print(now.month());
  oled.print("/");
  oled.print(now.year());
  oled.display();

  uint8_t current_day = now.day();
  uint8_t current_month = now.month();

  hour = ExecuteChange(current_hour, 0, 23, 30, 25);
  minutes = ExecuteChange(current_minutes, 0, 59, 48, 25);
  day = ExecuteChange(current_day, 1, 31, 20, 50);
  month = ExecuteChange(current_month, 1, 12, 38, 50);
  year += ExecuteChange(0, 0, 99, 68, 50);
  RTC.adjust(DateTime(year, month, day, hour, minutes, 0));
}
*/

/*
void PrepareAlarm()
{
  // Declare an object of class TimeDate
  DateTime my_alarm = RTC.getAlarm1();

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
*/

// TODO: change stuff
void DisableAlarm()
{

  // Declare an object of class TimeDate
  DateTime MY_alarm = RTC.getAlarm1();

  while (true)
  {
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

void DetermineState(menu *state, int X_Value, int Y_Value)
{

  static bool BEEPSTATE;

  // upper and lower area of the joystick
  if (Y_Value < upper_area_limit)
  {
    BeepOnce(&BEEPSTATE);
    *state = ShowTimeAndDate;
  }
  else if (Y_Value > lower_area_limit)
  {
    BeepOnce(&BEEPSTATE);
    *state = ChangeAlarm;
  }
  else if (X_Value < left_area_limit)
  {
    BeepOnce(&BEEPSTATE);
    *state = StopAlarm;
  }
  else if (X_Value > right_area_limit)
  {
    BeepOnce(&BEEPSTATE);
    *state = ShowTempAndHum;
  }
  else
  {
    BEEPSTATE = false;
  }

  if ((SW_Pin.debounce()) && (*state == ShowTimeAndDate))
  {
    *state = ChangeTimeDate;
  }
}

void loop()
{

  oled.clearDisplay();

  static menu STATE;
  // read coordinates to determine the state
  int X_Value = analogRead(Joystick_X);
  int Y_Value = analogRead(Joystick_Y);
  DetermineState(&STATE, X_Value, Y_Value);

  switch (STATE)
  {
  case Idle:
  case ShowTimeAndDate:
    DisplayTimeDate();
    break;
  case ShowTempAndHum:
    DisplayDHT();
    break;
  case ChangeAlarm:
    // PrepareAlarm();
    STATE = ShowTimeAndDate;
    break;
  case StopAlarm:
    DisableAlarm();
    STATE = ShowTimeAndDate;
    break;
  case ChangeTimeDate:
    // ChangeTimeAndDate();
    STATE = ShowTimeAndDate;
    break;
  }

  // this works out but only for a few seconds, I'll need to make simple and repetitive melody
  /*
  if (alarm)
  {
    STATE = AlarmFiring;
    oled.clearDisplay();
    RTC.alarmFired(1);
    oled.setCursor(0, 0);
    oled.print("  ALARM IS FIRING");
    oled.display();

    int size = sizeof(durations) / sizeof(int);

    for (int note = 0; note < size && alarm; note++)
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
    { RTC.clearAlarm(1);
      alarm = false;
      analogWrite(Buzzer_Pin, 0);
      STATE = ShowTimeAndDate;
    }
  }
  */
}