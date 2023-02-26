#include "sensors.h";

float readBatteryVoltage() {
  return analogRead(BATT_VIN);
}