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

// let's add an enum to mark things more easily
enum Direction { UP, DOWN, LEFT, RIGHT, CENTER };

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

  oled.clearDisplay();

  char buffer[19]; // this buffer is used for the strftime
  for (int i = 0; i < 19; i++) buffer[i] = 0;
  // this is for each change in parameters of DateTime
  DateTime now = RTC.now(); // (DateTime(year, month, day, hour, minutes, 0));

  struct tm t;

  t.tm_year = now.year() - 1900;  // struct tm expects years since 1900
  t.tm_mon  = now.month() - 1;    // struct tm expects months in [0, 11]
  t.tm_mday = now.day();
  t.tm_hour = now.hour();
  t.tm_min  = now.minute();
  mktime(&t);
  oled.setCursor(0, 0);
  oled.print("  TIME AND DATE");

  oled.setCursor(20, 25);
  strftime(buffer, 19, "%H:%M \n %d/%m/%Y", &t);
  oled.print(buffer);
  oled.display();

}




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
  default:
    BEEPSTATE = false;
    break;
  }

}

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
  case ShowTimeAndDate:
    DisplayTimeDate();
    break;
  case ShowTempAndHum:
    // DisplayDHT();
    oled.clearDisplay();
    oled.print("AAAAAAA");
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
}