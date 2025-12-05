#pragma once

enum Direction { UP, DOWN, LEFT, RIGHT, CENTER };

void setupInput();
void updateButton();
Direction readJoystick();
bool WasClicked();
bool WasLongPressed();
