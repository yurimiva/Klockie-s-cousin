#include <Wire.h>
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 
#include <DHT.h>  
#include <RTClib.h>
#include "pitches.h"
#include "button.h"
  
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
Button SW_Pin;
Button button_minus;
Button button_plus;

// define an rtc object
RTC_DS3231 RTC;
// define the PIN connected to SQW
const int CLOCK_INTERRUPT = 2;
volatile bool alarm = false; // need a global variable here so I won't mess up the interupt function

// define buzzer
const int Buzzer_Pin = 10;

// need to set an enumerable that helps me with the display of the modes

enum menu {
  Idle, // 0
  TimeAndDate, // 1
  TempAndHum, // 2
  Alarm, // 3
  StopAlarm,  // 4
  TimeDateChange // 5
};

// ---

// ISR function
void onAlarm() {

  alarm = true;

}


void setup () {

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

  //buzzer
  pinMode(Buzzer_Pin, OUTPUT);

  //testing
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
  RTC.clearAlarm(1);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  RTC.writeSqwPinMode(DS3231_OFF);

  RTC.clearAlarm(2); // we won't use it, but still best to avoid problems
  RTC.disableAlarm(2);

} 


void DisplayDHT(int temperature, int humidity) {

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


void DisplayTimeDate(DateTime previous) {

  oled.clearDisplay();
  oled.setCursor(0,25);
  oled.print(previous.hour());

  oled.setCursor(25,25);
  oled.print(previous.minute());

  oled.setCursor(0,50);
  oled.print(previous.day());

  oled.setCursor(25,50);
  oled.print(previous.month());

  oled.setCursor(50,50);
  oled.print(previous.year());

  oled.display();
  

}


uint8_t ExecuteChange(uint8_t what_to_change, int min, int max, int cursor_x, int cursor_y) {

  while (!SW_Pin.debounce()) {

    oled.drawFastHLine(cursor_x, (cursor_y - 5), 11, 1);
    oled.display();

    if (button_plus.debounce()) {

      // animation for the change of the hour
      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1, 0);
      oled.print("  ");
      oled.display();

      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1);

      if (what_to_change >= 9) {
            
        if (what_to_change == max) {
          what_to_change = min;
          oled.print("0");
          oled.print(what_to_change);
          oled.display();
        } else {
          what_to_change += 1;
          oled.print(what_to_change);
          oled.display();
        }

      } else { 

        oled.print("0");
        what_to_change += 1;
        oled.print(what_to_change);
        oled.display();

      }
          
    }

    if (button_minus.debounce()) {

      // animation for the change of the hour
      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1, 0);
      oled.print("  ");
      oled.display();

      oled.setCursor(cursor_x, cursor_y);
      oled.setTextColor(1);

      if (what_to_change > 10) {

        what_to_change -= 1;
        oled.print(what_to_change);
        oled.display();

      } else { 

        if (what_to_change == min) {
          what_to_change = max;
          oled.print(what_to_change);
          oled.display();
        } else {
          what_to_change -= 1;
          oled.print("0");
          oled.print(what_to_change);
          oled.display();
        }

      }

    }

  } 

  if (SW_Pin.debounce()) {
    return what_to_change;
  }


}


uint16_t ExecuteChangeYear(uint16_t changing_year) {

  while (!SW_Pin.debounce()) {

    oled.drawFastHLine(56, 45, 22, 1);

    if (button_plus.debounce()) {

      oled.setCursor(56, 50);
      oled.setTextColor(1, 0);
      oled.print("    ");
      oled.display();
            
      oled.setCursor(56, 50);
      oled.setTextColor(1);
      changing_year++;
      oled.print(changing_year);
      oled.display();

    }

    if (button_minus.debounce()) {

      oled.setCursor(56, 50);
      oled.setTextColor(1, 0);
      oled.print("    ");
      oled.display();
            
      oled.setCursor(56, 50);
      oled.setTextColor(1);
      changing_year--;
      oled.print(changing_year);
      oled.display();

    }

  } 

  if (SW_Pin.debounce()) {
    return changing_year;
  }

}


void ChangeTimeAndDate(DateTime need_change) {

  // TIME //
  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("TIME AND DATE SETTING");
  oled.setCursor(30,25);
  if (need_change.hour() <= 9) {
    oled.print("0");
  }
  oled.print(need_change.hour());
  oled.print(":");
  if (need_change.minute() <= 9) {
    oled.print("0");
  }
  oled.print(need_change.minute());
  uint8_t change_hour = need_change.hour();
  uint8_t change_minutes = need_change.minute();

  // DATE //
  oled.setCursor(20, 50);
  if (need_change.day() <= 9) {
    oled.print("0");
  }
  oled.print(need_change.day());
  oled.print("/");
  if (need_change.month() <= 9) {
    oled.print("0");
  }
  oled.print(need_change.month());
  oled.print("/");
  oled.print(need_change.year());
  oled.display();

  uint8_t change_day = need_change.day();
  uint8_t change_month = need_change.month();
  uint16_t change_year = need_change.year();

  static int change_time_date;

  if (SW_Pin.debounce()) {
    change_time_date++;
  }

  switch (change_time_date) {
  case 1:
    change_hour = ExecuteChange(change_hour, 0, 23, 30, 25);
    change_time_date++;

    oled.setCursor(30, 25);
    oled.setTextColor(1, 0);
    oled.print("  ");
    oled.display();

    oled.setCursor(30, 25);
    oled.setTextColor(1);
    oled.print(change_hour);
    oled.display();

    break;
  case 2:
    change_minutes = ExecuteChange(change_minutes, 0, 59, 48, 25);
    change_time_date++;
    break;
  case 3:
    change_day = ExecuteChange(change_day, 1, 31, 20, 50);
    change_time_date++;
    break;
  case 4:
    change_month = ExecuteChange(change_month, 1, 12, 38, 50);
    change_time_date++;
    break;
  case 5:
    change_year = ExecuteChangeYear(change_year);
    change_time_date++;
    break;
  case 6:
    RTC.adjust(DateTime(change_year, change_month, change_day, change_hour, change_minutes, 0));
    change_time_date = 0;
    break;
  }


}


void PrepareAlarm(DateTime my_alarm) {

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("   ALARM SETTING");

  uint8_t hour_alarm = my_alarm.hour();
  uint8_t minutes_alarm = my_alarm.minute();

  oled.setCursor(30,25);
  if (hour_alarm <= 9) {
    oled.print("0");
  }
  oled.print(hour_alarm);

  oled.print(":");

  if (minutes_alarm <= 9) {
    oled.print("0");
  }
  oled.print(minutes_alarm);
  oled.display();

  static int count_alarm;

  if (SW_Pin.debounce()) {
    count_alarm++;
  }

  switch (count_alarm) {
    case 1:
      hour_alarm = ExecuteChange(hour_alarm, 0, 23, 30, 25);
      break;
    case 2:
      minutes_alarm = ExecuteChange(minutes_alarm, 0, 59, 48, 25);
      break;
    case 3:
      RTC.setAlarm1(DateTime(0, 0, 0, hour_alarm, minutes_alarm, 0), DS3231_A1_Hour);
      oled.display();
      count_alarm = 0;
      break;
  }

}


void DisableAlarm(DateTime MY_alarm) {

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("WANT TO DISABLE ALARM?");

  oled.setCursor(30,25);
  if (MY_alarm.hour() <= 9) {
    oled.print("0");
  }
  oled.print(MY_alarm.hour());

  oled.print(":");

  if (MY_alarm.minute() <= 9) {
    oled.print("0");
  }
  oled.print(MY_alarm.minute());
  oled.display();

  oled.setCursor(10,50);
  oled.print("YES (press +)");
  if (button_plus.debounce()) {
    RTC.disableAlarm(1);
  }

  oled.setCursor(80,50);
  oled.print("NO (-)");
  if (button_minus.debounce()) {
    RTC.setAlarm1(MY_alarm, DS3231_A1_Hour);
  }

  oled.display();

}


void BeepOnce(bool *beepstate) {

  if (*beepstate == false) {
    *beepstate = true;
    tone(Buzzer_Pin, 1047, 200);
  }
}


void loop () {  

  // Declare an object of class TimeDate
  DateTime now = DateTime(2000,1,1,0,0,0);
  DateTime alarm_display = RTC.getAlarm1();
  // this is for each change in parameters of DateTime

  // read temp and hum
  int temperature = (int)dht.readTemperature();  
  int humidity = (int)dht.readHumidity(); 
 
  // read coordinates
  int X_Value = analogRead(VrX);
  int Y_Value = analogRead(VrY);

  static menu MENU;
  static bool BEEPSTATE;

  if ((Y_Value < 312) && (150 < X_Value) && (X_Value < 874)) { // up condition

    BeepOnce(&BEEPSTATE);
    MENU = TimeAndDate;

  } else if ((Y_Value > 712) &&  (150 < X_Value) && (X_Value < 874)) { // down condition

    // ALARM
    BeepOnce(&BEEPSTATE);
    MENU = Alarm;

  } else if ((X_Value < 312) &&  (150 < Y_Value) && (Y_Value < 874)) { // left condition

    BeepOnce(&BEEPSTATE);
    MENU = StopAlarm;

  } else if ((X_Value > 712) &&  (150 < Y_Value) && (Y_Value < 874)) { // right condition

    BeepOnce(&BEEPSTATE);
    MENU = TempAndHum;

  } else {

    BEEPSTATE = false;
    if ((SW_Pin.debounce()) && (MENU == TimeAndDate)) {

      MENU = TimeDateChange;
    }

  }

  switch (MENU) {
    case Idle:
      oled.setCursor(0,0);
      oled.print("MOVE THE JOYSTICK");
      oled.display();
      break;
    case TimeAndDate:
      DisplayTimeDate(now);
      break;
    case TempAndHum:
      DisplayDHT(temperature, humidity);
      break;
    case Alarm:
      PrepareAlarm(alarm_display);
      break;
    case StopAlarm:
      DisableAlarm(alarm_display);
      break;
    case TimeDateChange:
      ChangeTimeAndDate(now);
      break;
  }

  // this works out but only for a few seconds, I'll need to make simple and repetitive melody
  if (alarm) {
    RTC.alarmFired(1);
    oled.print("Alarm is firing");
    oled.display();
  }

 
}