#include "Sensors.h"
#include <DHTStable.h>

#define DHTPIN 7
DHTStable dht;

// cache
static unsigned long lastRead = 0;
static float cachedTemp = NAN;
static float cachedHum  = NAN;

void setupSensors() {
    // nessuna init necessaria per DHTStable, ma puoi fare un delay di warm-up
    delay(2000);
}

static bool updateCache() {

    unsigned long now = millis();
    if (now - lastRead < 2000) {
        // meno di 2 secondi dall’ultima lettura → usa cache
        return !isnan(cachedTemp) && !isnan(cachedHum);
    }

    // nuova lettura
    int status = dht.read11(DHTPIN); // o dht.read11(DHTPIN) se DHT11
    if (status == DHTLIB_OK) {
        cachedTemp = dht.getTemperature();
        cachedHum  = dht.getHumidity();
        lastRead   = now;
        return true;
    } else {
        // mantieni cache precedente se valida
        return !isnan(cachedTemp) && !isnan(cachedHum);
    }
}

int readTemperature(bool useFahrenheit) {
    if (!updateCache()) return -100; // errore
    if (useFahrenheit) {
        return (int)((cachedTemp * 9.0 / 5.0) + 32.0);
    }
    return (int)cachedTemp;
}

int readHumidity() {
    if (!updateCache()) return -1; // errore
    return (int)cachedHum;
}
