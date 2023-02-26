#include "commands.h";
#include "sensors.h";

#define JSON_CHAR_SIZE 384

char* execute_command(char flag) {
  DynamicJsonDocument doc(JSON_CHAR_SIZE);
  char stringBuffer[JSON_CHAR_SIZE];

  switch (flag) {
    case 'b':
      get_battery_json(doc);
  }

  serializeJson(doc, stringBuffer);

  return stringBuffer;
}

void get_battery_json(DynamicJsonDocument& doc) {
  float voltage;

  voltage = readBatteryVoltage();
  voltage *= ((20000.0 + 10000.0) / 10000.0);
  voltage *= 3.3;
  voltage /= 4096;

  doc["battery"] = voltage;
}

void get_thp_json(DynamicJsonDocument doc) {

}
void get_uv_json(DynamicJsonDocument doc) {

}
void get_wind_json(DynamicJsonDocument doc) {

}
void get_rain_json(DynamicJsonDocument doc) {
  
}