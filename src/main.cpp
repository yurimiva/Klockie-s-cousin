#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <RTClib.h>
#include "pitches.h"
#include "button.h"
#include "nokia_melody.h"

// define the DHT
#define DHTPIN 7 // pin connected to DHT11 sensor
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

// define buzzer
const int Buzzer_Pin = 10;

// need to set an enumerable that helps me with the display of the modes

enum menu
{
  Idle, // fatto
  ShowTimeAndDate, // fatto
  ChangeTimeDate, // fatto
  ShowTempAndHum, // fatto
  ShowAlarm, // sto facendo
  changeAlarm,
  StopAlarm,
  ShowTimer,
  ChangeTimer
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


void DisplayDHT()
{
  // Leggi temperatura e umidità
  int temperature = (int)dht.readTemperature();
  int humidity = (int)dht.readHumidity();

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
  delay(3);

  snprintf(buffer, sizeof(buffer), "%02d:%02d\n%02d/%02d/%04d",
             now.hour(), now.minute(),
             now.day(), now.month(), now.year());

  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("  TIME AND DATE");
 
  oled.setCursor(20, 25);
  oled.print(buffer);

  delay(3);
  oled.display();

}

// Permette di fare l'animazione su OLED
void aggiornaDisplay(int valori[], int index) {

  // lampeggio stateless: alterna ogni 500ms
  bool blink = ((millis() / 500) % 2);

  char displayBuffer[20];

  // scrive direttamente tutto il buffer con padding
  snprintf(displayBuffer, sizeof(displayBuffer), "%02d:%02d\n%02d/%02d/%04d",
           valori[0], valori[1],
           valori[2], valori[3], valori[4]);

  // definizione start/length di ogni campo nel buffer
  const int startIndex[5] = {0, 3, 6, 9, 12}; // hh, mm, DD, MM, YYYY
  const int length[5]     = {2, 2, 2, 2, 4};

  // lampeggio: sostituisce i caratteri del campo corrente con spazi
  if (!blink) {
      for (int i = 0; i < length[index]; i++)
          displayBuffer[startIndex[index] + i] = ' ';
  }

  // stampa su OLED
  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("   CHANGE MODE");

  oled.setCursor(20, 25);
  oled.print(displayBuffer);

  delay(3);
  oled.display();
}


// Restituisce il numero massimo di giorni di un dato mese e anno (usata per la normalizzazione)
int maxGiorni(int mese, int anno) {
  switch (mese) {
      case 1: case 3: case 5: case 7: case 8: case 10: case 12:
          return 31; // mesi con 31 giorni
      case 4: case 6: case 9: case 11:
          return 30; // mesi con 30 giorni
      case 2:
          // febbraio: controlla anno bisestile
          if ((anno % 4 == 0 && anno % 100 != 0) || (anno % 400 == 0))
              return 29; // anno bisestile
          else
              return 28; // anno normale
      default:
          return 31; // fallback
  }
}


void ChangeTimeAndDate() {

  // prima di tutto dobbiamo definire cosa faccio
  // voglio cambiare un certo valore presente in DateTime in base agli input del joystick

  // voglio definire un massimo e un minimo per ogni singolo valore della struct
  // userò due array; e un contatore
  int index = 0;
  const int minVal[5] = {0, 0, 1, 1, 2000};
  const int maxVal[5] = {23, 59, 31, 12, 2099};

  DateTime now = RTC.now();
  delay(3);

  // importo una variabile in modo da poter continuare la modifca del dato
  // TODO testare questa cosa

  bool modificaCompleta = false;

  // questi valori da modificare li memorizzo in un array
  int valori[5] = {now.hour(), now.minute(), now.day(), now.month(), now.year()};
  
  // normalizzazione giorno-mese iniziale
  int giorniMax = maxGiorni(valori[3], valori[4]);
  if(valori[2] < minVal[2]) valori[2] = minVal[2];
  if(valori[2] > giorniMax) valori[2] = giorniMax;

  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("   CHANGE MODE");

  while (!modificaCompleta) {

    int xVal = analogRead(Joystick_X) - 512; // Center around 0
    int yVal = analogRead(Joystick_Y) - 512;
    Direction joystick = readJoystick(xVal, yVal);

    switch (joystick)
    {
    case UP:
      (valori[index] >= maxVal[index]) ? (valori[index] = minVal[index]) : valori[index]++; // TODO testare che sta roba funzioni
      break;
    case DOWN:
      (valori[index] <= minVal[index]) ? (valori[index] = maxVal[index]) : valori[index]--;
      break;
    case RIGHT:
      index++;
      break;
    case LEFT:
      index--;
      break;
    }

    if (index >= 5) {
      index = 0;
      modificaCompleta = true;
      break;
    }

    aggiornaDisplay(valori, index);
    delay(150);
  }

  // normalizzazione finale
  giorniMax = maxGiorni(valori[3], valori[4]);
  if(valori[2] < minVal[2]) valori[2] = minVal[2];
  if(valori[2] > giorniMax) valori[2] = giorniMax;

  // faccio anche l'adjust con cui salco i dati normalizzati
  RTC.adjust(DateTime(valori[4], valori[3], valori[2], valori[0], valori[1], 0));
  delay(3);

}


void ChangeAlarm() {
  oled.clearDisplay();
  oled.setCursor(0,0);
  oled.print("   CHANGE ALARM");
  oled.display();

  int index = 0;  // 0 = ore, 1 = minuti
  int valori[2] = {RTC.getAlarm1().hour(), RTC.getAlarm1().minute()};
  const int minVal[2] = {0,0}, maxVal[2] = {23,59};
  bool done = false;

  unsigned long lastBlink = 0;
  bool blink = false;

  while(!done) {
      // Aggiorna lampeggio ogni 500ms
      if(millis() - lastBlink > 500) {
          lastBlink = millis();
          blink = !blink;
      }

      // Leggi joystick
      Direction dir = readJoystick(analogRead(Joystick_X)-512, analogRead(Joystick_Y)-512);
      switch(dir) {
          case UP:    
              valori[index] = (valori[index] >= maxVal[index]) ? minVal[index] : valori[index]+1; 
              break;
          case DOWN:  
              valori[index] = (valori[index] <= minVal[index]) ? maxVal[index] : valori[index]-1; 
              break;
          case RIGHT:
          case LEFT:  
              index = (index+1)%2;  // passa da ore a minuti
              break;
          default: break;
      }

      // Disegna campo lampeggiante solo sul campo attivo
      oled.fillRect(20, 25, 40, 10, SSD1306_BLACK); // cancella area numeri
      oled.setCursor(20,25);
      char buf[6];

      if(index == 0) { // lampeggia solo le ore
          snprintf(buf,sizeof(buf),"%s:%02d", blink ? "  " : String(valori[0]).c_str(), valori[1]);
      } else {        // lampeggia solo i minuti
          snprintf(buf,sizeof(buf),"%02d:%s", valori[0], blink ? "  " : String(valori[1]).c_str());
      }
      oled.print(buf);
      oled.display();

      // Uscita con pulsante
      if(SW_Pin.debounce()) done = true;

      delay(50); // piccolo delay per stabilità display
  }

  // Imposta Alarm1 con i valori scelti
  RTC.setAlarm1(DateTime(0,0,0,valori[0],valori[1],0), DS3231_A1_Hour);
  RTC.clearAlarm(1);

  // Messaggio conferma
  oled.fillRect(0,25,128,16,SSD1306_BLACK);
  oled.setCursor(20,25);
  oled.print("ALARM SET!");
  oled.display();
  delay(1000);
}


void DisplayAlarm() {

  // Leggi i valori correnti della sveglia (Alarm1)
  DateTime alarm1 = RTC.getAlarm1();
  delay(3);
  
  // Mostra su OLED
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("     ALARM 1");
  
  oled.setCursor(20, 25);
  char buf[10] = {0};
  snprintf(buf, sizeof(buf), "%02d:%02d", alarm1.hour(), alarm1.minute());
  oled.print(buf);
  
  delay(3);
  oled.display();

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
  }
}