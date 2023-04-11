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

byte getDirectionFromAnalogValue(int adc) {
  if (adc <= 106) return 5;    // 112.5 ESE
  if (adc <= 176) return 3;    // 67.5 ENE
  if (adc <= 209) return 4;    // 90 E
  if (adc <= 336) return 7;    // 157.5 SSE
  if (adc <= 565) return 6;    // 135 SE
  if (adc <= 795) return 9;    // 202.5 SSW
  if (adc <= 962) return 8;    // 180 S
  if (adc <= 1428) return 1;   // 22.5 NNE
  if (adc <= 1650) return 2;   // 45 NE
  if (adc <= 2190) return 11;  // 247.5 WSW
  if (adc <= 2306) return 10;  // 225 SW
  if (adc <= 2595) return 15;  // 337.5 NNW
  if (adc <= 2933) return 0;   // 0 N
  if (adc <= 3129) return 13;  // 292.5 WNW
  if (adc <= 3456) return 14;  // 315 NW
  if (adc <= 3842) return 12;  // 270 W

    // Older check with more constrictive parameters based on testing.
  // if (inRange(adc, 100, 106)) return 5;
  // if (inRange(adc, 171, 176)) return 3;
  // if (inRange(adc, 205, 209)) return 4;
  // if (inRange(adc, 330, 336)) return 7;
  // if (inRange(adc, 562, 565)) return 6;
  // if (inRange(adc, 790, 795)) return 9;
  // if (inRange(adc, 959, 962)) return 8;
  // if (inRange(adc, 1423, 1428)) return 1;
  // if (inRange(adc, 1646, 1650)) return 2;
  // if (inRange(adc, 2186, 2190)) return 11;
  // if (inRange(adc, 2303, 2306)) return 10;
  // if (inRange(adc, 2592, 2595)) return 15;
  // if (inRange(adc, 2928, 2933)) return 0;
  // if (inRange(adc, 3122, 3129)) return 13;
  // if (inRange(adc, 3449, 3456)) return 14;
  // if (inRange(adc, 3838, 3842)) return 12;

  return 16;
}