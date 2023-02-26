#ifndef COMMANDS_H
#define COMMANDS_H
#include <Arduino.h>
#include <ArduinoJson.h>

template <unsigned int N> void print(char (&a)[N]);
char* execute_command(char flag);
void get_battery_json(DynamicJsonDocument doc);
void get_thp_json(DynamicJsonDocument doc);
void get_uv_json(DynamicJsonDocument doc);
void get_wind_json(DynamicJsonDocument doc);
void get_rain_json(DynamicJsonDocument doc);

#endif