#ifndef COMMANDS_H
#define COMMANDS_H
#include <Arduino.h>

void get_battery_json(char* outStr);
void get_thp_json(char* outStr);
void get_uv_json(char* outStr);
void get_wind_json(char* outStr);
void get_rain_json(char* outStr);

#endif