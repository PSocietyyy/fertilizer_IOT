#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>

void initOLED();
void readSensor(float &temperature, float &humidity);
void showSensorData(float temperature, float humidity);
void showError(const char *message);

void controlRelay(float temperature, float humidity);

void fanOn();
void fanOff();

void pumpOn();
void pumpOff();

#endif // HARDWARE_H
