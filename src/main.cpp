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

// define each variable for the changed time and date
uint8_t hour;
uint8_t minutes;
uint8_t day;
uint8_t month;
uint16_t year;


// need to set an enumerable that helps me with the display of the 4 modes

enum menu {
  TimeAndDate,
  TempAndHum,
  Alarm,
  StopAlarm,
  Idle
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


void SingleBeep(bool *beep_state) {

  if (*beep_state == false) {
    *beep_state = true;
    tone(Buzzer_Pin, 1047, 200);
  }

}

/*

void ExecuteWhat(int interface_state) {
 
  while (interface_state == 0) {
    // SU
    //  DisplayTimeDate();
  } 

  while (interface_state == 1) {
    // DESTRA
    //DisplayDHT();
  }

}

*/
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


void ChangeTime(DateTime need_change, int state) {

  oled.clearDisplay();

  oled.setCursor(0,0);
  oled.print("   TIME SETTING");

  oled.setCursor(30,25);
  oled.setTextSize(2);
  oled.print(need_change.hour());
  oled.print(":");
  oled.print(need_change.minute());
  oled.display();

  uint8_t change_hour = need_change.hour();
  uint8_t change_minutes = need_change.minute();

  // need an animation to show the change
  oled.drawFastHLine(30, 20, 22, 1);

  while (state == 1) {

    if (button_plus.debounce()) {

      if (change_hour >= 9) {
        // animation for the change of the hour
        oled.setCursor(30,25);
        oled.setTextColor(1, 0);
        oled.print("  ");
        oled.display();
        
        oled.setCursor(30,25);
        oled.setTextColor(1);

        if (change_hour == 23) {
          change_hour = 0;
          oled.print("0");
          oled.print(change_hour);
          oled.display();
        } else {
          change_hour += 1;
          oled.print(change_hour);
          oled.display();
        }

      } else { 

        oled.setCursor(30,25);
        oled.setTextColor(1,0);
        oled.print("  ");
        oled.display();

        oled.setCursor(30,25);
        oled.setTextColor(1);
        oled.print("0");
        change_hour += 1;
        oled.print(change_hour);
        oled.display();

      }
      
    }

    if (button_minus.debounce()) {

      if (change_hour > 10) {
        // animation for the change of the hour
        oled.setCursor(30,25);
        oled.setTextColor(1, 0);
        oled.print("  ");
        oled.display();
        
        oled.setCursor(30,25);
        oled.setTextColor(1);
        change_hour -= 1;
        oled.print(change_hour);
        oled.display();

      } else { 

        oled.setCursor(30,25);
        oled.setTextColor(1,0);
        oled.print("  ");
        oled.display();

        oled.setCursor(30,25);
        oled.setTextColor(1);

        if (change_hour == 0) {
          change_hour = 23;
          oled.print(change_hour);
          oled.display();
        } else {
          change_hour -= 1;
          oled.print("0");
          oled.print(change_hour);
          oled.display();
        }

      }

    }

    if (SW_Pin.debounce()) {
      hour = change_hour;
      state = 2;
      break;
    }

  }

  while (state == 2) {

    if (button_plus.debounce()) {

      if (change_minutes >= 9) {
        // animation for the change of the hour
        oled.setCursor(66,25);
        oled.setTextColor(1, 0);
        oled.print("  ");
        oled.display();
        
        oled.setCursor(66,25);
        oled.setTextColor(1);

        if (change_minutes == 59) {
          change_minutes = 0;
          oled.print("0");
          oled.print(change_minutes);
          oled.display();
        } else {
          change_minutes += 1;
          oled.print(change_minutes);
          oled.display();
        }

      } else { 

        oled.setCursor(66,25);
        oled.setTextColor(1,0);
        oled.print("  ");
        oled.display();

        oled.setCursor(66,25);
        oled.setTextColor(1);
        oled.print("0");
        change_minutes += 1;
        oled.print(change_minutes);
        oled.display();

      }
      
    }

    if (button_minus.debounce()) {

      if (change_minutes > 10) {
        // animation for the change of the hour
        oled.setCursor(66,25);
        oled.setTextColor(1, 0);
        oled.print("  ");
        oled.display();
        
        oled.setCursor(66,25);
        oled.setTextColor(1);
        change_minutes -= 1;
        oled.print(change_minutes);
        oled.display();

      } else { 

        oled.setCursor(66,25);
        oled.setTextColor(1,0);
        oled.print("  ");
        oled.display();

        oled.setCursor(66,25);
        oled.setTextColor(1);

        if (change_minutes == 0) {
          change_minutes = 23;
          oled.print(change_minutes);
          oled.display();
        } else {
          change_minutes -= 1;
          oled.print("0");
          oled.print(change_minutes);
          oled.display();
        }

      }

      if (SW_Pin.debounce()) {
        minutes = change_minutes;
        state = 3;
        break;
      }

    }
  }


}

void loop () {  

  // Declare an object of class TimeDate
  DateTime now = RTC.now();
  // define variable for change mode
  int menu = 0;

  // read temp and hum
  int temperature = (int)dht.readTemperature();  
  int humidity = (int)dht.readHumidity(); 
 
  // read coordinates
  int X_Value = analogRead(VrX);
  int Y_Value = analogRead(VrY);

  // qua il puntatore fa comodo per ogni volta che chiamerò la funzione SingleBeep
  bool *BeepState;
  bool BEEP_STATE = false;
  BeepState = &BEEP_STATE;

  if ((Y_Value < 312) && (150 < X_Value) && (X_Value < 874)) { // up condition

    SingleBeep(BeepState);
    DisplayTimeDate(now);

  } else if ((Y_Value > 712) &&  (150 < X_Value) && (X_Value < 874)) { // down condition

    SingleBeep(BeepState);

    oled.clearDisplay();
    oled.setCursor(50,50);
    oled.print("GIU");
    oled.display();

  } else if ((X_Value < 312) &&  (150 < Y_Value) && (Y_Value < 874)) { // left condition

    // set the buzzer's sound signal frequency 
    SingleBeep(BeepState);

    oled.clearDisplay();
    oled.setCursor(0,50);
    oled.print("SINISTRA");
    oled.display();

  } else if  ((X_Value > 712) &&  (150 < Y_Value) && (Y_Value < 874)) { // right condition

    SingleBeep(BeepState);

    DisplayDHT(temperature, humidity);

  } else {
    // make sure it will do one single beep again
    *BeepState = false;

    if ((menu == 0) && (SW_Pin.debounce())) {
      menu = 1;
      ChangeTime(now, menu);
    }

  }
 

}