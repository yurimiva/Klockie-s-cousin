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
// set a state value for the buzzer to make sure that it beeps only once
bool beep_state = false;


// define what I need for the time and date
int hour;
int minutes;
int day;
int month;
int year;

// menu changes for each parameter
int menu;

// need this variable to be global


// ---

void setup () {

  // initialize the dht sensor
  dht.begin(); 

  // initialize the oled
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  oled.clearDisplay(); 
  oled.setTextSize(1);  
  oled.setTextColor(SSD1306_WHITE); 
  
  // define mode for joystick
  // coordinates
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


void SingleBeep(bool beep_state) {

  if (beep_state == false) {
    beep_state = true;
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
void DisplayTimeDate(DateTime now) {

  oled.clearDisplay();
  oled.setCursor(0,25);
  oled.print(now.hour());

  oled.setCursor(25,25);
  oled.print(now.minute());

  oled.setCursor(0,50);
  oled.print(now.day());

  oled.setCursor(25,50);
  oled.print(now.month());

  oled.setCursor(50,50);
  oled.print(now.year());

  oled.display();
  

}


void ChangeTime(DateTime now) {

  oled.clearDisplay();

  oled.setCursor(0,0);
  oled.print(" HOUR AND MINUTES SETTING ");

  oled.setCursor(50,25);
  oled.setTextSize(2);
  oled.print(now.hour());

  




}


void loop () {  

  /*if (SW_Pin.debounce()) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
  }
  */

  // Declare an object of class TimeDate
  DateTime now = RTC.now();


  // read temp and hum
  int temperature = (int)dht.readTemperature();  
  int humidity = (int)dht.readHumidity(); 
 
  // read coordinates
  int X_Value = analogRead(VrX);
  int Y_Value = analogRead(VrY);

  if ((Y_Value < 312) && (150 < X_Value) && (X_Value < 874)) { // up condition

    SingleBeep(beep_state);

    DisplayTimeDate(now);


  } else if ((Y_Value > 712) &&  (150 < X_Value) && (X_Value < 874)) { // down condition

    SingleBeep(beep_state);

    oled.clearDisplay();
    oled.setCursor(50,50);
    oled.print("GIU");
    oled.display();

  } else if ((X_Value < 312) &&  (150 < Y_Value) && (Y_Value < 874)) { // left condition

    // set the buzzer's sound signal frequency 
    SingleBeep(beep_state);

    oled.clearDisplay();
    oled.setCursor(0,50);
    oled.print("SINISTRA");
    oled.display();

  } else if  ((X_Value > 712) &&  (150 < Y_Value) && (Y_Value < 874)) { // right condition

    SingleBeep(beep_state);

    DisplayDHT(temperature, humidity);

  } else {
    // make sure it will do one single beep again
    beep_state = false;

    if ((menu == 0) && (SW_Pin.debounce())) {
      menu = 1;
      ChangeTime(now);
    }

  }
 

}