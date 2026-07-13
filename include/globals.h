#ifndef GLOBALS_H
#define GLOBALS_H

#include "config.h"
#include <Arduino.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>
#include <Blynk/BlynkTimer.h>

// Objects
extern DHT dht;
extern Adafruit_SSD1306 display;
extern BlynkTimer timer;

// State variables
extern float currentTemp;
extern float currentHum;

extern float lastSentTemp;
extern float lastSentHum;
extern bool lastSentFanState;
extern bool lastSentPumpState;
extern int lastSentWifi;

extern unsigned long lastHeartbeat;

#endif // GLOBALS_H
