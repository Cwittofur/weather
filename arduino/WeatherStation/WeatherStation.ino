/*
 * MicroMod Weather Carrier Board Example
 * 
 * This sketch tests all of the weather sensors on the carrier board:
 * atmospheric sensor - BME280, UV sensor - VEML6075, lightning detector - AS3935,
 * soil moisture sensor, wind and rain meters.
 * 
 * Priyanka Makin @ SparkX Labs
 * Original Creation Date: August 20, 2020
 * 
 * This code is Lemonadeware; if you see me (or any other SparkFun employee) at the
 * local, and you've found our code helpful, please buy us a round!
 * 
 * Hardware Connections:
 * Insert MicroMod Processor Board of your choice into the M.2 connector of the SparkFun Weather carrier
 *  Screw into place
 * Connect Weather carrier board to power useing USB-C cable
 * Connect SparkFun Soil Moisture Sensor to Weather carrier using latching terminals
 * Connect both wind and rain meters to Weather carrier using the RJ11 connectors
 */

#include <Wire.h>
#include <SPI.h>
#include "SparkFunBME280.h"
#include <SparkFun_VEML6075_Arduino_Library.h>
#include "SparkFun_AS3935.h"

BME280 tempSensor;
VEML6075 uv;
SparkFun_AS3935 lightning;

#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

#if defined(ESP_PLATFORM)
//int LED_BUILTIN = 5;
//int A0 = 34;
//int G0 = 4;
//int D0 = 23;
//int A1 = 35;
//int D1 = 27;
//const int G3 = 17;
//int G1 = 12;
#elif defined(ARDUINO_ARCH_SAMD)
int G0 = 2;
int D0 = 0;
int D1 = 1;
//const int lightningInt = G3;  //SPI does not currently work on the SAMD51 proto
//int spiCS = G1;
#endif

int soilPin = A0;  //Pin number that measures analog moisture signal
int soilPower = G0;  //Pin number that will power the soil moisture sensor
int WSPEED = D0; //Digital I/O pin for wind speed
int WDIR = A1; //Analog pin for wind direction
int RAIN = D1;   //Digital I/O pin for rain fall
const int lightningInt = G3; // Interrupt pin for lightning detection
int spiCS = G1; //SPI chip select pin

volatile bool rainFlag = false;
volatile bool windFlag = false;

//Function is called every time the rain bucket tips
void rainIRQ()
{
  rainFlag = true;
}

//Function is called when the magnet in the anemometer is activated
void wspeedIRQ()
{
  windFlag = true;
}

// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector. 
int intVal = 0;
int noise = 2; // Value between 1-7 
int disturber = 2; // Value between 1-10

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("MicroMod Weather Carrier Board Test");
  Serial.println();

  Wire.begin();
  SPI.begin();

  if (tempSensor.beginI2C() == false) { //Begin communication over I2C
    Serial.println("BME280 did not respond.");
    while(1); //Freeze
  }
  if (uv.begin() == false) {
    Serial.println("VEML6075 did not respond.");
    while(1);
  }

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(soilPower, OUTPUT);
  digitalWrite(soilPower, LOW);
  // When lightning is detected the interrupt pin goes HIGH.
  pinMode(lightningInt, INPUT);

  //Initialization for weather meter
  pinMode(WSPEED, INPUT_PULLUP);  //Input from wind meters windspeed sensor
  pinMode(RAIN, INPUT_PULLUP);    //Input from wind meters rain gauge sensor
  //attach external interrupt pins to IRQ functions
  attachInterrupt(digitalPinToInterrupt(RAIN), rainIRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(WSPEED), wspeedIRQ, FALLING);
  //turn on interrupts
  interrupts();

  if(lightning.beginSPI(spiCS, 2000000) == false){ 
    Serial.println ("Lightning Detector did not start up, freezing!"); 
    while(1); 
  }
  else
    Serial.println("Schmow-ZoW, Lightning Detector Ready!");

  // The lightning detector defaults to an indoor setting at 
  // the cost of less sensitivity, if you plan on using this outdoors 
  // uncomment the following line:
  lightning.setIndoorOutdoor(OUTDOOR); 
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)

  Serial.println();
  Serial.print("Temperature: ");
  Serial.println(tempSensor.readTempF(), 2);
  Serial.print("Humidity: ");
  Serial.println(tempSensor.readFloatHumidity(), 0);
  Serial.print("Pressure: ");
  Serial.println(tempSensor.readFloatPressure(), 0);
  Serial.print("Altitude: ");
  Serial.println(tempSensor.readFloatAltitudeFeet(), 1);

  Serial.print("UV A, B, index: ");
  Serial.println(String(uv.uva()) + ", " + String(uv.uvb()) + ", "+ String(uv.index()));

  Serial.print("Soil Moisture = ");
  Serial.println(readSoil());

  Serial.print("Wind direction: ");
  Serial.print(getWindDirection());
  Serial.println(" degrees");
  //Check interrupt flags
  if (rainFlag == true){
    Serial.println("Rain click!");
    rainFlag = false;
  }
  if (windFlag == true){
    Serial.println("Wind click!");
    windFlag = false;
  }

  // Hardware has alerted us to an event, now we read the interrupt register
  if(digitalRead(lightningInt) == HIGH){
    intVal = lightning.readInterruptReg();
    if(intVal == NOISE_INT){
      Serial.println("Noise."); 
      // Too much noise? Uncomment the code below, a higher number means better
      // noise rejection.
      //lightning.setNoiseLevel(noise); 
    }
    else if(intVal == DISTURBER_INT){
      Serial.println("Disturber."); 
      // Too many disturbers? Uncomment the code below, a higher number means better
      // disturber rejection.
      //lightning.watchdogThreshold(disturber);  
    }
    else if(intVal == LIGHTNING_INT){
      Serial.println("Lightning Strike Detected!"); 
      // Lightning! Now how far away is it? Distance estimation takes into
      // account any previously seen events in the last 15 seconds. 
      byte distance = lightning.distanceToStorm(); 
      Serial.print("Approximately: "); 
      Serial.print(distance); 
      Serial.println("km away!"); 
    }
  }

  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

  delay(3000);
}

int readSoil() {
  int moistVal = 0;  //Variable for storing moisture value
  //Power Senor
  digitalWrite(soilPower, HIGH);
  delay(10);
  moistVal = analogRead(soilPin);  //Read the SIG value from sensor
  digitalWrite(soilPower, LOW); //Turn the sensor off
  return moistVal; //Return current moisture value
}

double getWindDirection()
{
  unsigned int adc;
  adc = analogRead(WDIR); //get the current readings from the sensor

  Serial.print("Wind Direction 'adc' ");
  Serial.println(adc);

  if (inRange(adc, 100, 106)) return 112.5;
  if (inRange(adc, 171, 176)) return 67.5;
  if (inRange(adc, 205, 209)) return 90;
  if (inRange(adc, 330, 336)) return 157.5;
  if (inRange(adc, 562, 565)) return 135;
  if (inRange(adc, 790, 795)) return 202.5;
  if (inRange(adc, 959, 962)) return 180;
  if (inRange(adc, 1423, 1428)) return 22.5;
  if (inRange(adc, 1646, 1650)) return 45;
  if (inRange(adc, 2186, 2190)) return 247.5;
  if (inRange(adc, 2303, 2306)) return 225;
  if (inRange(adc, 2592, 2595)) return 337.5;
  if (inRange(adc, 2928, 2933)) return 0;
  if (inRange(adc, 3122, 3129)) return 292.5;
  if (inRange(adc, 3449, 3456)) return 315;
  if (inRange(adc, 3838, 3842)) return 270;

  return (-1);
}

bool inRange(int val, int min, int max)
{
  return ((min <= val) && (val <= max));
}