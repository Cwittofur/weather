#ifndef UTILS_H
#define UTILS_H
#include <Arduino.h>

unsigned long diff(unsigned long a, unsigned long b);

bool inRange(int val, int min, int max);

float getDegreesFromDirection(byte windDirection);

int averageAnalogRead(int inputPin);

#endif