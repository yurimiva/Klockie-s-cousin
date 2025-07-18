#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 
#include <DHT.h>  
#include <RTClib.h>
// #include "pitches.h"
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

// define buzzer
const int Buzzer_Pin = 10;

// define variables for DateTime
uint8_t hour;
uint8_t minutes;
uint8_t day;
uint8_t month;
uint16_t year;

// need to set an enumerable that helps me with the display of the 4 modes

enum menu {
  TimeAndDate, // 0
  TempAndHum, // 1
  Alarm, // 2
  StopAlarm,  // 3
  TimeDateChange, // 4
  Idle // 5
};

// ---

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
  SW_Pin.begin(2);
  button_minus.begin(5);
  button_plus.begin(7);

  //buzzer
  pinMode(Buzzer_Pin, OUTPUT);

  //testing
  pinMode(LED_BUILTIN, OUTPUT);

  // initialize the RTC
  RTC.begin();

} 

//I have the display function for temp and hum
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

  do {

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

    if (SW_Pin.debounce()) {
      return what_to_change;
      break;
    }

  } while (!SW_Pin.debounce());

}

uint16_t ExecuteChangeYear(uint16_t changing_year) {

  do {

    oled.drawFastHLine(56, 45, 22, 1);

    if (button_plus.debounce()) {

      oled.setCursor(56, 45);
      oled.setTextColor(1, 0);
      oled.print("    ");
      oled.display();
            
      oled.setCursor(66, 45);
      oled.setTextColor(1);
      changing_year++;
      oled.print(changing_year);
      oled.display();

    }

    if (button_minus.debounce()) {

      oled.setCursor(66, 45);
      oled.setTextColor(1, 0);
      oled.print("    ");
      oled.display();
            
      oled.setCursor(66, 45);
      oled.setTextColor(1);
      changing_year--;
      oled.print(changing_year);
      oled.display();

    }

    if (SW_Pin.debounce()) {
      return changing_year;
      break;
    }

  } while (!SW_Pin.debounce());

}

void ChangeTimeAndDate(DateTime need_change, byte count) {

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
  oled.display();
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

  switch (count) {
  case 1:
    change_hour = ExecuteChange(change_hour, 0, 23, 30, 25);
    oled.display();
    count++;
    hour = change_hour;
    break;
  case 2:
    change_minutes = ExecuteChange(change_minutes, 0, 59, 48, 25);
    oled.display();
    count = 3;
    minutes = change_minutes;
    break;
  case 3:
    change_day = ExecuteChange(change_day, 1, 31, 20, 50);
    oled.display();
    count = 4;
    day = change_day;
  case 4:
    change_month = ExecuteChange(change_month, 1, 12, 48, 50);
    oled.display();
    count = 5;
    month = change_month;
  case 5:
    change_year = ExecuteChangeYear(change_year);
    oled.display();
    count = 6;
    year = change_year;
    break;
  case 6:
    RTC.adjust(DateTime(year, month, day, hour, minutes, 0));
  }


}


void BeepOnce(bool *beepstate) {

  if (!*beepstate) {
    *beepstate = true;
    tone(Buzzer_Pin, 1047, 200);
  }
}


void loop () {  

  // Declare an object of class TimeDate
  DateTime now = RTC.now();
  // this is for each change in parameters of DateTime
  byte count;

  // read temp and hum
  int temperature = (int)dht.readTemperature();  
  int humidity = (int)dht.readHumidity(); 
 
  // read coordinates
  int X_Value = analogRead(VrX);
  int Y_Value = analogRead(VrY);

  static menu MENU;
  static bool *BEEPSTATE;

  if ((Y_Value < 312) && (150 < X_Value) && (X_Value < 874)) { // up condition

    BeepOnce(BEEPSTATE);
    MENU = TimeAndDate;

  } else if ((Y_Value > 712) &&  (150 < X_Value) && (X_Value < 874)) { // down condition

    // ALARM
    BeepOnce(BEEPSTATE);
    MENU = Alarm;

  } else if ((X_Value < 312) &&  (150 < Y_Value) && (Y_Value < 874)) { // left condition

    BeepOnce(BEEPSTATE);
    MENU = StopAlarm;

  } else if ((X_Value > 712) &&  (150 < Y_Value) && (Y_Value < 874)) { // right condition

    BeepOnce(BEEPSTATE);
    MENU = TempAndHum;

  } else {

    if (SW_Pin.debounce()) {

      *BEEPSTATE = false;
      MENU = TimeDateChange;
    }

  }

  switch (MENU) {
    case TimeAndDate:
      DisplayTimeDate(now);
      break;
    case TempAndHum:
      DisplayDHT(temperature, humidity);
      break;
    case Alarm:
      // DisplayAlarm;
      break;
    case StopAlarm:
      // StopAlarm;
      break;
    case TimeDateChange:
      count = 1;
      ChangeTimeAndDate(now, count);
      break;
  }
 
}