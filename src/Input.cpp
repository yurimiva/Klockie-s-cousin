#include "Input.h"
#include <Arduino.h>
#include <OneButton.h>

#define BUTTON_PIN 3
#define JOY_X 14
#define JOY_Y 15
#define DEADZONE 100   // zona neutra per evitare falsi movimenti

OneButton button(BUTTON_PIN, true);

volatile bool buttonClicked = false;
volatile bool longPressAction = false;

void setupInput() {
    pinMode(JOY_X, INPUT);
    pinMode(JOY_Y, INPUT);
    button.attachClick([](){ buttonClicked = true; });
    button.attachLongPressStart([](){ longPressAction = true; });
}

void updateButton() {
    button.tick();
}

Direction readJoystick() {
    int x = analogRead(JOY_X) - 512; // centrato intorno a 0
    int y = analogRead(JOY_Y) - 512;

    if (abs(x) < DEADZONE && abs(y) < DEADZONE) return CENTER;

    if (abs(x) > abs(y)) { return (x > 0) ? RIGHT : LEFT;}
    else { return (y < 0) ? UP : DOWN; }
    return CENTER;
}

bool WasClicked() {
    if (buttonClicked) { buttonClicked = false; return true; }
    return false;
}

bool WasLongPressed() {
    if (longPressAction) { longPressAction = false; return true; }
    return false;
}
