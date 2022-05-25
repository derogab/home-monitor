#include "Arduino.h"

typedef struct sensors
{
    float humidity;
    float temperature;
    float apparent_temperature;
    bool flame;
    bool light;
    long rssi;
    String mac;
    String name;
} sensors_t;
