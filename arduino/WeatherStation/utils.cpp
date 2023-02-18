#include "utils.h";

unsigned long diff(unsigned long a, unsigned long b) {
  return (a > b) ? (a - b) : (b - a);
}

bool inRange(int val, int min, int max)
{
  return ((min <= val) && (val <= max));
}

// Return the wind direction, each direction is separated by 22.5 degrees
float getDegreesFromDirection(byte windDirection) {
  return windDirection * 22.5;
}

int averageAnalogRead(int inputPin) {
  byte i;
  unsigned int accumulator = 0;

  for (i = 0; i < 8; i++) {
    accumulator += analogRead(inputPin);
  }

  return accumulator / 8;
}