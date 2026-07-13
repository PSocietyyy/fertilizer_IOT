#include "globals.h"

// WiFi
char ssid[] = "IPhone Perdi";
char pass[] = "12345678999";

// Objects
DHT dht(DHT_PIN, DHT_TYPE);

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);

BlynkTimer timer;

// State variables
float currentTemp = 0.0;
float currentHum = 0.0;

float lastSentTemp     = -999.0;
float lastSentHum      = -999.0;
bool  lastSentFanState = false;
bool  lastSentPumpState= false;
int   lastSentWifi     = -1;

unsigned long lastHeartbeat = 0;
