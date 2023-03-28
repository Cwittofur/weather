#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <Preferences.h>
#include "SparkFunBME280.h"
#include <SparkFun_VEML6075_Arduino_Library.h>
#include "SparkFun_AS3935.h"
#include "utils.h"
#include "commands.h"

// #define DEBUG

// Array depths
#define WIND_2M_ARRAY_DEPTH 120
#define RAIN_1H_ARRAY_DEPTH 60
#define GUST_10M_ARRAY_DEPTH 10
#define REPORT_STRING_BUFFER_LENGTH 74
#define RAIN_DAILY_ARRAY_DEPTH 24

// Constants
#define WIND_SENSOR_DEBOUNCE_SETTLING_TIME 10  // milliseconds
#define RAIN_SENSOR_DEBOUNCE_SETTLING_TIME 75  // milliseconds
#define HTTP_CONNECTION_TIMEOUT 500            // milliseconds
#define RAIN_CLICK_CONVERSION_FACTOR 0.011     // inches per click
#define WIND_CLICK_CONVERSION_FACTOR 1.492     // miles per hour per click

// Lightning Constants

#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

// Pin Definitions

int WSPEED = D0;
int WDIR = A1;
int RAIN = D1;
const int lightningInt = G3; // Interrupt pin for lightning detection
int spiCS = G1; // SPI Chip select pin for Lightning

String ssid;
String password;

Preferences preferences;
WebServer server(80);

// I2C Sensors
BME280 tempSensor;
VEML6075 uv;
SparkFun_AS3935 lightning;

StaticJsonDocument<1024> jsonDocument;
char buffer[1024];

// IRQ for Wind/Rain
volatile word irqRainClicks;
volatile word irqTotalRainClicks;
volatile unsigned long lastRainClickTime;

volatile word irqWindClicks;
volatile unsigned long lastWindClickTime;

// Time Based Values

unsigned long lastLoopCycleTime;
unsigned long lastWindCheckTime;
unsigned long lastLightningStrikeTime;
byte seconds;
byte minutes;
int prevClientRequestTime;

// Reading values

float tempF;
float tempC;
float humidity;
float pressure;
float uva;
float uvb;
float uvIndex;
float voltage;
float windDirection2mAverage;
float windSpeed2mAverage;

// Indices for arrays

byte wind2mIndex;
byte rain1hIndex;
byte wind10mGustIndex;
byte rainDailyIndex;
byte lightningHourlyIndex;
byte isLightning;

float windSpeed;
int windDirection;
float windSpeedAverage;
float windDirectionAverage;
float windGustSpeed;
int windGustDirection;
float windGustSpeedMax10m;
int windGustDirectionMax10m;
float windGustSpeedDailyMax;
int lightningDistance;

float hourlyRainAmount;
float dailyRainAmount;

float windSpeed2mAverageArray[WIND_2M_ARRAY_DEPTH];
byte windDirection2mAverageArray[WIND_2M_ARRAY_DEPTH];
float windGustSpeed10mArray[GUST_10M_ARRAY_DEPTH];
byte windGustDirection10mArray[GUST_10M_ARRAY_DEPTH];
int rainPerHourArray[RAIN_1H_ARRAY_DEPTH];
int rainDailyArray[RAIN_DAILY_ARRAY_DEPTH];
int lightningPerHourArray[RAIN_1H_ARRAY_DEPTH];
int lightningDailyArray[RAIN_DAILY_ARRAY_DEPTH];

// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector. 
int intVal = 0;
int noise = 4; // Value between 1-7 
int disturber = 5; // Value between 1-10

void WindSpeedIRQ() {
  unsigned long currentTime;

  currentTime = millis();

  if (diff(currentTime, lastWindClickTime) > WIND_SENSOR_DEBOUNCE_SETTLING_TIME) {
    lastWindClickTime = currentTime;

    irqWindClicks++;
  }
}

void RainfallIRQ() {
  unsigned long currentTime;

  currentTime = millis();

  if (diff(currentTime, lastWindClickTime) > RAIN_SENSOR_DEBOUNCE_SETTLING_TIME) {
    lastRainClickTime = currentTime;

    irqRainClicks++;
    irqTotalRainClicks++;
  }
}

void connectToWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.print("Connected! IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup_routing() {
  server.on("/", getWeatherJson); // Full Report
  server.on("/m", getMidnightReport);
  // Battery Level
  // Temperature | Pressure | Humidity
  // UV
  // Rain

  server.begin();
}

void getBatteryLevelJson() {
  char *stringBuffer;
  getBatteryLevel();
  stringBuffer = execute_command('b');

  server.send(200, "application/json", stringBuffer);
}

void getMidnightReport() {
  getWeatherJson();
  init_daily_variables();
}

void getWeatherJson() {
  char stringBuffer[1024];

  DynamicJsonDocument doc(1024);
  JsonObject uv = doc.createNestedObject("uv");
  JsonObject wind = doc.createNestedObject("wind");
  JsonObject thp = doc.createNestedObject("thp");
  JsonObject rain = doc.createNestedObject("rain");
  JsonObject lightning = doc.createNestedObject("lightning");

  // doc["battery"] = voltage;

  uv["a"] = uva;
  uv["b"] = uvb;
  uv["index"] = uvIndex;

  wind["speed"] = windSpeed;
  wind["direction"] = windDirection < 16 ? getDegreesFromDirection(windDirection) : -1;
  wind["speed2MinuteAverage"] = windSpeedAverage;
  wind["direction2MinuteAverage"] = windDirectionAverage;
  wind["gustTenMinuteMaxSpeed"] = windGustSpeedMax10m;
  wind["gustTenMinuteMaxDirection"] = windDirection < 16 ? getDegreesFromDirection(windGustDirectionMax10m) : -1;
  wind["maxDailyGust"] = windGustSpeedDailyMax;

  rain["hour"] = hourlyRainAmount;
  rain["daily"] = dailyRainAmount;

  thp["tempC"] = tempC;
  thp["tempF"] = tempF;
  thp["humidity"] = humidity;
  thp["pressure"] = pressure;

  lightning["strike"] = isLightning == 1 ? true : false;
  lightning["distance"] = lightningDistance;

  serializeJson(doc, stringBuffer);

  server.send(200, "application/json", stringBuffer);
}

void init_sensors() {
  Wire.begin();
  SPI.begin();

  // Temp readings seem slightly higher than other sources, changing the temp by 1 degree to see if that helps
  tempSensor.setTemperatureCorrection(-1.0);

  pinMode(lightningInt, INPUT);
  pinMode(WSPEED, INPUT_PULLUP);
  pinMode(RAIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(WSPEED), WindSpeedIRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(RAIN), RainfallIRQ, FALLING);
  interrupts();

  if(lightning.beginSPI(spiCS, 2000000) == false){ 
    Serial.println ("Lightning Detector did not start up, freezing!"); 
    while(1); 
  }
  if (tempSensor.beginI2C() == false) {  //Begin communication over I2C
    Serial.println("BME280 did not respond.");
    while (1)
      ;  //Freeze
  }
  if (uv.begin() == false) {
    Serial.println("VEML6075 did not respond.");
    while (1)
      ;
  }

  lightning.setIndoorOutdoor(OUTDOOR); 
}

void init_variables() {
  irqRainClicks = 0;
  irqTotalRainClicks = 0;
  lastRainClickTime = 0;

  irqWindClicks = 0;
  lastWindClickTime = 0;
  prevClientRequestTime = int(millis() / 1000);
  
  init_daily_variables();

  lastLoopCycleTime = millis();
}

void init_daily_variables() {
  windGustSpeedDailyMax = 0;
  dailyRainAmount = 0;
  hourlyRainAmount = 0; 
  
  windDirection2mAverage = 0;
  windSpeed2mAverage = 0;
  windSpeedAverage = 0;
  windDirectionAverage = 0;
  windGustSpeedMax10m = 0;

  lightningDistance = 0;
  isLightning = 0;    

  wind2mIndex = 0;
  rainDailyIndex = 0;
  rain1hIndex = 0;
  wind10mGustIndex = 0;

  memset(rainDailyArray, 0, RAIN_DAILY_ARRAY_DEPTH);
  memset(rainPerHourArray, 0, RAIN_1H_ARRAY_DEPTH);
}

void setup() {
  Serial.begin(115200);

  preferences.begin("credentials", false);

  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");

  if (ssid == "" || password == ""){
    Serial.println("No values saved for ssid or password");
  }
  else {
    Serial.printf("SSID: %s\n Password: %s", ssid, password);

    connectToWifi();
  }
  
  setup_routing();
  init_sensors();

  lastLoopCycleTime = millis();

  // Initialize battery level on startup
  //getBatteryLevel();

  delay(20);
}

void loop() {
  unsigned long currentTime;

    // Hardware has alerted us to an event, now we read the interrupt register
  if(digitalRead(lightningInt) == HIGH){
    intVal = lightning.readInterruptReg();
    if(intVal == NOISE_INT){
      //Serial.println("Noise."); 
      // Too much noise? Uncomment the code below, a higher number means better
      // noise rejection.
      lightning.setNoiseLevel(noise); 
    }
    else if(intVal == DISTURBER_INT){
      //Serial.println("Disturber."); 
      // Too many disturbers? Uncomment the code below, a higher number means better
      // disturber rejection.
      lightning.watchdogThreshold(disturber);  
    }
    else if(intVal == LIGHTNING_INT){
      byte distance = lightning.distanceToStorm(); 
      #ifdef DEBUG
        Serial.println("Lightning Strike Detected!"); 
        // Lightning! Now how far away is it? Distance estimation takes into
        // account any previously seen events in the last 15 seconds.         
        Serial.print("Approximately: "); 
        Serial.print(distance); 
        Serial.println("km away!"); 
      #endif

      lastLightningStrikeTime = millis();
      lightningDistance = distance;
      isLightning = 1;

    }
  }

  currentTime = millis();

  if (diff(currentTime, lastLoopCycleTime) > 999) {
    lastLoopCycleTime = currentTime - (currentTime - lastLoopCycleTime) % 1000;

    doEverySecond();
    
    seconds++;
    if (seconds > 59) {
      seconds = 0;

      doEveryMinute();
      
      minutes++;

      if (minutes > 59) {
        minutes = 0;
        doEveryHour();
      }
    }
  }

  server.handleClient();

  delay(20);
}

void doEverySecond() {
  // Anything that needs to be updated each second shall go here

  // Wind Speed and Direction
  windSpeed = getWindSpeed();
  windDirection = getWindDirection();

  if (windSpeed > windGustSpeedDailyMax) {
    windGustSpeedDailyMax = windSpeed;    
  }

  getSensorData();

  update2MinWindAverage();

  // Every 30 seconds get battery level
  // if (seconds % 30 == 0) {
  //   getBatteryLevel();
  // }

  if (diff(millis(), lastLightningStrikeTime) > 600000) {
    lightningDistance = 0;
    isLightning = 0;
  }

  // Update wind gusts
  update1SecWindGust();
  calcWindMetrics();
}

void doEveryMinute() {
  // Update 10 minute wind gusts
  update10MinWindGust();

  // Update hourly rainfall amounts
  updateHourlyRainfall();

  hourlyRainAmount = calculateRainAmount(rainPerHourArray, RAIN_1H_ARRAY_DEPTH);
  dailyRainAmount = calculateRainAmount(rainPerHourArray, RAIN_DAILY_ARRAY_DEPTH);

  #ifdef DEBUG
    displayArrays('m');
    displayArrays('h');
  #endif
}

void doEveryHour() {
  hourlyRainfallUpdate();
}

void update2MinWindAverage() {
  windSpeed2mAverageArray[wind2mIndex] = windSpeed;
  windDirection2mAverageArray[wind2mIndex] = windDirection;

  wind2mIndex++;

  if (wind2mIndex > WIND_2M_ARRAY_DEPTH - 1) {
    wind2mIndex = 0;
  }
}

void update1SecWindGust() {
  if (windSpeed > windGustSpeed10mArray[wind10mGustIndex]) {
    windGustSpeed10mArray[wind10mGustIndex] = windSpeed;
    windGustDirection10mArray[wind10mGustIndex] = windDirection;
  }

  if (windSpeed > windGustSpeed) {
    windGustSpeed = windSpeed;
    windGustDirection = windDirection;
  }
}

void update10MinWindGust() {
  wind10mGustIndex++;

  if (wind10mGustIndex > GUST_10M_ARRAY_DEPTH - 1) {
    wind10mGustIndex = 0;
  }

  windGustSpeed10mArray[wind10mGustIndex] = 0;
  windGustDirection10mArray[wind10mGustIndex] = 0;
}

void updateHourlyRainfall() {
  rainPerHourArray[rain1hIndex] = irqRainClicks;
  rainDailyArray[rainDailyIndex] = irqRainClicks;
}

void hourlyRainfallUpdate() {
  rainDailyIndex++;
  rain1hIndex++;

  irqRainClicks = 0;  

  if (rain1hIndex > RAIN_1H_ARRAY_DEPTH - 1) {
    rain1hIndex = 0;
  }

  if (rainDailyIndex > RAIN_DAILY_ARRAY_DEPTH) {
    rainDailyIndex = 0;
  }
}

void getSensorData() {
  tempF = tempSensor.readTempF();
  tempC = tempSensor.readTempC();
  humidity = tempSensor.readFloatHumidity();
  pressure = tempSensor.readFloatPressure() / 100; // Convert from Pascal to Hectopascal (hPa)
  uva = uv.uva();
  uvb = uv.uvb();
  uvIndex = uv.index();
}

void getBatteryLevel() {
  voltage = analogRead(BATT_VIN);
  voltage *= ((20000.0 + 10000.0) / 10000.0);
  voltage *= 3.3;
  voltage /= 4096;
}

float getWindSpeed() {
  unsigned long currentTime;
  float clicksPerSecond;

  currentTime = millis();

  clicksPerSecond = 1000.0 * float(irqWindClicks) / diff(currentTime, lastWindCheckTime);

  irqWindClicks = 0;
  lastWindCheckTime = currentTime;

  return float(clicksPerSecond * WIND_CLICK_CONVERSION_FACTOR);
}

float calculateRainAmount(int rainClicksArray[], word arraySize) {
  int rainClickCount = 0;

  for (int i = 0; i < arraySize - 1; i++) {
    rainClickCount += rainClicksArray[i];
  }

  return float(rainClickCount * RAIN_CLICK_CONVERSION_FACTOR);
}

byte getWindDirection() {
  unsigned int adc;
  adc = averageAnalogRead(WDIR);  //get the current readings from the sensor

  
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



  #ifdef DEBUG
    Serial.print("Wind Direction 'adc' ");
    Serial.println(adc);
  #endif

  return (16);
}

// Need to redo this; it's off for some odd reason. Print out arrays I guess?
void calcWindMetrics() {
  float windDirectionSum = 0;
  float windSpeedSum = 0;
  byte windDirectionCount = 0;
  windGustSpeedMax10m = 0;
  word i;

  // Find highest wind speed and direction within the last 10 minutes
  for (i = 0; i < GUST_10M_ARRAY_DEPTH; i++) {
    if (windGustSpeed10mArray[i] > windGustSpeedMax10m) {
      windGustSpeedMax10m = windGustSpeed10mArray[i];
      windGustDirectionMax10m = windGustDirection10mArray[i];
    }
  }

  for (i = 0; i < WIND_2M_ARRAY_DEPTH; i++) {
    if (windDirection2mAverageArray[i] < 16) {
      windDirectionSum += getDegreesFromDirection(windDirection2mAverageArray[i]);
      windDirectionCount++;
    }

    windSpeedSum += windSpeed2mAverageArray[i];
  }

  windSpeedAverage = windSpeedSum / WIND_2M_ARRAY_DEPTH;
  windDirectionAverage = windDirectionSum / WIND_2M_ARRAY_DEPTH;
}

#ifdef DEBUG
void displayArrays(byte sel) {
  byte i;

  switch (sel) {
    case 's':
      Serial.print(F("2m Wind: "));
      for (i = 0; i < WIND_2M_ARRAY_DEPTH; i++) {
        Serial.print(windSpeed2mAverageArray[i]);
        Serial.print(F(", "));
      }
      Serial.println();
      break;
    case 'm':
      Serial.print(F("10m gust: "));
      for (i = 0; i < 10; i++) {
        Serial.print(windGustSpeed10mArray[i]);
        Serial.print(F(", "));
      }
      Serial.println();
      break;
    case 'h':
      Serial.print(F("1h rain: "));
      for (i = 0; i < RAIN_1H_ARRAY_DEPTH; i++) {
        Serial.print(rainPerHourArray[i]);
        Serial.print(F(", "));
      }
      Serial.println();
      break;
  }
}
#endif