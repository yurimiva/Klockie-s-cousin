#include "Settings.h"

// Valori di default
bool use12hFormat = true;    // di default 12h
bool useFahrenheit = false;  // di default °C

void setupSettings() {
    // Qui puoi caricare da EEPROM o SD se vuoi persistenza
    use12hFormat = true;
    useFahrenheit = false;
}

void toggle12hFormat() {
    use12hFormat = !use12hFormat;
}

void toggleFahrenheit() {
    useFahrenheit = !useFahrenheit;
}
