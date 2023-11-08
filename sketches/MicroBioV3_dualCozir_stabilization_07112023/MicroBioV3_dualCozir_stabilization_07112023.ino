#include <SPI.h>
#include <SD.h>
#include "Adafruit_MAX31855.h"
#include <SeeedGrayOLED.h>
#include <Wire.h>
#include "DHT.h"
#include <L298NX2.h>
#include <EEPROM.h>
#include "cozir.h"
String res;

// settings
int gasSetPointAds = 100; // gas set point during adsortion
int gasSetPointDes = 700; // gas set point during desortion
double tempMax = 80;
double tempMin = 75;

// starting state
int currentState = 1;  // initial state if = 0 testing // if = 1 ciclo

// timing
long heatIntervalOn = 10000;   // on interval for heat plates
long heatIntervalOff = 10000; // off interval for heat plates
long sdInterval = 60000;      // SD saving interval
long printInterval = 5000;    // Serial Print interval
long checkInterval = 300000; // Stabilization of new state interval// 300000 = 5 minutes. 600000 = 10 minutes (in miliseconds)

// for cozir sensor
COZIR czr[2] = {COZIR(&Serial1), COZIR(&Serial2)}; // Connected to Hard Serial1 and Hard Serial2
uint32_t lastCO2[2] = {0, 0};
float result1, result2, gasResult;

// for ambient co2
long ambCO2;

uint32_t lastRead = 0;

unsigned long previousMillisSD, currentMillisSD = 0; // counter for SD saving
unsigned long currentMillisPrint, previousMillisPrint = 0; //counter for Serial Print
unsigned long previousMillisHeat, currentMillisHeat = 0;  // counter for heating plates
unsigned long currentMillisCheck, previousMillisCheck;

bool calentar = true; // boolean for heat control
bool check = false;

#define writeSdOn //uncomment if you want to save into SD card
//#define oledOn //uncomment if you want to use the display

unsigned int conditionCounter = 0;  // Contador para que espere 1 minuto antes de volver a empezar el ciclo
unsigned long startTime = 0;

#include "relays.h"
#include "modes.h"
#include "sensors.h"
#include "vaccumm.h"
#include "remote_control.h"
#include "sd_shield.h"
#include "oled_display.h"

void setup() {
  Wire.begin();
  delay(200);
  Serial.begin(115200);
  Serial.println("...initializing serial ports...");
  Serial1.begin(9600);  //Cozir100
  Serial2.begin(9600);  //Cozir Sensor
  Serial3.begin(9600);  //Amb Sensor

  delay(1000);

  startCozir();

#ifdef oledOn
  startOled();
  delay(100);
  SeeedGrayOled.setTextXY(1, 0);
  SeeedGrayOled.putString("Starting");
  SeeedGrayOled.setTextXY(3, 0);
  SeeedGrayOled.putString("Calibration");
  SeeedGrayOled.setTextXY(7, 0);
#endif

  dht.begin();

#ifdef writeSdOn
  // Open serial communications and wait for port to open:
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
#ifdef oledOn
    SeeedGrayOled.clearDisplay();
    SeeedGrayOled.setTextXY(1, 0);  //set Cursor to ith line, 0th column
    SeeedGrayOled.putString("Card Failed");
    delay(2000);
#endif
    // don't do anything more:
    //while (1)
    ;
  } else {
    Serial.println("card initialized.");
#ifdef oledOn
    SeeedGrayOled.clearDisplay();
    SeeedGrayOled.setTextXY(1, 0);  //set Cursor to ith line, 0th column
    SeeedGrayOled.putString("Card");
    SeeedGrayOled.setTextXY(3, 0);  //set Cursor to ith line, 0th column
    SeeedGrayOled.putString("initialized");
    delay(1000);
#endif
  }
#endif

  //RELAYS
  for (int j = 0; j <= relayPins; j++) {
    pinMode(whichRelay[j], OUTPUT);
  }
  for (int j = 0; j <= relayPins; j++) {
    digitalWrite(whichRelay[j], HIGH);
  }
  delay(1000);
  //startRemoteControl();

#ifdef oledOn
  SeeedGrayOled.clearDisplay();
  SeeedGrayOled.setTextXY(1, 0);  
  SeeedGrayOled.putString("Starting");
  SeeedGrayOled.setTextXY(3, 0);  
  SeeedGrayOled.putString("Program");
  SeeedGrayOled.setTextXY(7, 0);  
  //SeeedGrayOled.putString("count to 10");
  delay(2000);
#endif
}

void loop() {
  currentMillisHeat = millis();
  currentMillisPrint = millis();
  currentMillisSD = millis();
  currentMillisCheck = millis();

  switch (currentState) {
    case 0:  //for testing
      readCozir();
      readDHT();
      //readInTemperature();
      readAmbGas(); // read ambient gas sensor
      turnOffHeat();  // heat OFF

      runVacuum(1, 120);  //vacuum pump 1 ON
      runVacuum(2, 120);    // vacuum pump 2 OFF

      openValve(1);  // solenoid valve 1 open
      openValve(2);  // solenoid valve 2 open

      if (currentMillisCheck - previousMillisCheck >= checkInterval) { // this state won't change until 5 minutes have passed to let all the parameters settle.
        check = true;
      } else {
        check = false;
      }

      if (check == true) { // if 5 minutes have passed check if the conditions for passing to the next case are met
        conditionCounter++;  // Increment the counter
        if (conditionCounter >= 750) {  // Check if the condition has been met for 5min // 150 = 1 min
          previousMillisCheck = currentMillisCheck;
          conditionCounter = 0;
          check = false;
        }
      } else {  // Reset the timer and counter if the condition is not met
        startTime = 0;
        conditionCounter = 0;
      }
      break;

    case 1:           // adsorción
      readCozir();
      readDHT();
      readInTemperature();
      readAmbGas();
      turnOffHeat();  // heat OFF

      runVacuum(1, 100);  //vacuum pump 1 ON
      runVacuum(2, 0);    // vacuum pump 2 OFF

      openValve(1);  // solenoid valve 1 open
      openValve(2);  // solenoid valve 2 open

      if (currentMillisCheck - previousMillisCheck >= checkInterval) { / this state won't change until 5 minutes have passed to let all the parameters settle.
        check = true;
      } else {
        check = false;
      }

      if (check == true) {
        if (gasResult <= gasSetPointAds) {   // check if this happens for 5 minutes
          if (startTime == 0) {    // Check if timer is not running, start it
            startTime = millis();  // Record start time
          }
          conditionCounter++;  // Increment the counter
          Serial.print("counter: ");
          Serial.println(conditionCounter);

          if (conditionCounter >= 750) {  // Check if the condition has been met for 5min // 150 = 1 min
            currentState = 2;
            check = false;
            previousMillisCheck = currentMillisCheck;
          }
        } else {  // Reset the timer and counter if the condition is not met
          startTime = 0;
          conditionCounter = 0;
        }
      }
      break;

    case 2:             // transición adsorción desorción
      readCozir();
      readDHT();
      readInTemperature();
      readAmbGas();
      runVacuum(1, 0);  //vacuum pump 1 OFF
      runVacuum(2, 0);  // vacuum pump 2 OFF

      closeValve(1);  // solenoid valve 1 closed
      closeValve(2);  // solenoid valve 2 closed

      turnOnHeat();

      if (currentMillisCheck - previousMillisCheck >= 120000) { / this state won't change until 5 minutes have passed to let all the parameters settle.
        check = true;
      } else {
        check = false;
      }

      if (check == true) {
        if (result >= tempMax) {
          if (startTime == 0) {
            startTime = millis();  // Record start time
          }
          conditionCounter++;  // Increment the counter

          if (conditionCounter >= 4500) {  // Hold 30 min at max temp
            currentState = 3;
            check = false;
            previousMillisCheck = currentMillisCheck;
          }
        } else {  // Reset the timer and counter if the condition is not met
          startTime = 0;
          conditionCounter = 0;
        }
      }
      break;

    case 3:               // desorción
      readCozir();
      readDHT();
      readInTemperature();
      readAmbGas();
      runVacuum(1, 0);    //vacuum pump 1 OFF
      runVacuum(2, 100);  // vacuum pump 2 ON (ENCENDER BOMBA NITROGENO)

      closeValve(1);   // solenoid valve 1 closed (cerrar valvula entrada aire)
      openValve(2);  // solenoid valve 2 open (abrir valvula salida aire)

      if (result <= tempMax && result >= tempMin) { // 10ºC = false // 40ºC = true // 90ºC = false
        turnOnHeat();
      }

      if (result <= tempMax && result <= tempMin) { // 10ºC = true // 40ºC = false // 90ºC = false
        turnOnHeat();
      }

      if (result >= tempMax ) {
        turnOffHeat();
      }

      if (currentMillisCheck - previousMillisCheck >= checkInterval) {
        check = true;
        //Serial.println("True");
      } else {
        check = false;
        //Serial.println("False");
      }

      if (check == true) {
        if (result2 <= gasSetPointDes) {   // check if this happens for 1 minute
          if (startTime == 0) {    // Check if timer is not running, start it
            startTime = millis();  // Record start time
          }
          conditionCounter++;  // Increment the counter

          if (conditionCounter >= 750) {  // Check if the condition has been met for 5 minutes
            currentState = 4;
            check = false;
            previousMillisCheck = currentMillisCheck;
          }
        } else {  // Reset the timer and counter if the condition is not met
          startTime = 0;
          conditionCounter = 0;
        }
      }
      break;

    case 4:  // transición des - ads
      readCozir();

      readDHT();
      readInTemperature();
      readAmbGas();
      turnOffHeat();

      runVacuum(1, 0);
      runVacuum(2, 150);  // vacuum pump 2 ON (ENCENDER BOMBA NITROGENO)

      closeValve(1);   // solenoid valve 1 closed
      openValve(2);  // solenoid valve 2 open (abrir valvula salida aire)

      if (currentMillisCheck - previousMillisCheck >= checkInterval) {
        check = true;
      } else {
        check = false;
      }

      if (check == true) {
        if (result <= 30) {   // check if this happens for 5 minutes, then restart the cycle, DHTTemp
          if (startTime == 0) {    // Check if timer is not running, start it
            startTime = millis();  // Record start time
          }
          conditionCounter++;  // Increment the counter

          if (conditionCounter >= 750) {  // Check if the condition has been met for 5 minutes
            currentState = 1;
            check = false;
            previousMillisCheck = currentMillisCheck;
          }
        } else {  // Reset the timer and counter if the condition is not met
          startTime = 0;
          conditionCounter = 0;
        }
      }
      break;
  }

  if (currentMillisPrint - previousMillisPrint >= printInterval) {
    previousMillisPrint = currentMillisPrint;
    serialPrintData();
#ifdef oledOn
    printOled();
#endif
  }

  if (currentMillisSD - previousMillisSD >= sdInterval) {
    previousMillisSD = currentMillisSD;
#ifdef writeSdOn
    writeSD();
#endif
  }
}

void startCozir() {

  Serial.print("...initializing COZIR objects...");
  for (int i = 0; i < 2; i++) {
    czr[i].init();
  }
  Serial.println();
  delay(2000);

  // set to polling explicitly.
  Serial.print("...set POLLING mode...");
  for (int i = 0; i < 2; i++) {
    czr[i].setOperatingMode(CZR_POLLING);
  }
  Serial.println();
  // set digital filter

  Serial.print("...set digital filter...");
  for (int i = 0; i < 2; i++) {
    czr[i].setDigiFilter(32);
  }
  Serial.println();
  delay(2000);
}

void serialPrintData() {

  Serial.println();

  Serial.print("state ");
  Serial.print(currentState);

  if (currentState == 1) {
    Serial.println(" - adsorción");
  } else if (currentState == 2) {
    Serial.println(" - transición ads - des");
  } else if (currentState == 3) {
    Serial.println(" - desorción");
  } else if (currentState == 4) {
    Serial.println(" - transición des - ads");
  } else if (currentState == 0) {
    Serial.println(" - testing");
  }

  Serial.print("CO2 1: ");
  Serial.print(lastCO2[0]); Serial.println(" PPM");
  Serial.print("CO2 2: ");
  Serial.print(lastCO2[1]); Serial.println(" PPM");

  Serial.print("result 1: ");
  Serial.print(result1); Serial.println(" PPM");
  Serial.print("result 2: ");
  Serial.print(result2); Serial.println(" PPM");

  Serial.print("Gas compare: ");
  Serial.print("Difference = ");
  Serial.print(gasResult);
  Serial.println(" PPM");

  Serial.print("Ambient CO2: ");
  Serial.print(ambCO2);
  Serial.println(" PPM");

  Serial.print("Ambience Temperature: ");

  if (isnan(DHTTemp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  } else {
    Serial.print(DHTTemp);
    Serial.println("°C ");
  }
  
  Serial.print("Internal Parameters: ");
  Serial.print(" Average temp: ");
  Serial.print(result);
  Serial.println("ºC");

  if (isnan(c)) {
    Serial.print("T / C Problem with 1");
    Serial.println(" ");
  } else {
    //        Serial.print("Internal Temp 1: ");
    //        Serial.print(c);
    //        Serial.print("ºC - ");
  }

  if (isnan(c2)) {
    Serial.print("T / C Problem with 2");
    Serial.println(" ");
  } else {
    //        Serial.print("Internal Temp 2: ");
    //        Serial.print(c2);
    //        Serial.println("ºC - ");
  }
  Serial.print("Check ");
  Serial.println(check);

}
