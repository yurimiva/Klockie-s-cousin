#pragma once

// Preferenze globali
extern bool use12hFormat;   // true = formato 12h, false = 24h
extern bool useFahrenheit;  // true = °F, false = °C

// Funzioni di gestione
void setupSettings();       // inizializza valori di default
void toggle12hFormat();     // cambia formato orario
void toggleFahrenheit();    // cambia unità di misura
