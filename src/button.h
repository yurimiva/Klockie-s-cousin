#ifndef button_h
#define button_h

#include "Arduino.h"

class Button
{
  private:
    uint8_t btn;
    uint16_t state;
  public:
    void begin(uint8_t button) {
      btn = button;
      pinMode(btn, INPUT_PULLUP);
    }
    bool debounce() {
      int pressed = 5;
      while (digitalRead(btn) == LOW) {
      // pressed = (pressed == 0)? 0: pressed--;
      if (pressed == 0) pressed = 0;
      else pressed--;
      }
      if (digitalRead(btn) == HIGH && !pressed) 
        return digitalRead(btn) == HIGH;
      return false;
    }
};
#endif