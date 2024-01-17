/*
            Ardunio MEGA and HM-10 wiring diagram
            -------------------------------------
            Arduino Pin (A12)   |   HM-10(TX)
            Arduino Pin (A13)   |   HM-10(RX)
            Arduino Pin (3.3V)  |   HM-10(VCC)
            Arduino Pin (GND)   |   HM-10(GND)


  ===================================================================================================================================================
  The following variables are used to control the state of pumps and valves from the mobile app to the Arduino MEGA. That means, these variables are used to manually turn on/off pumps and valves
  mode and are used to display the current status of pumps and valves in mobile applications in current state mode. To modify the state of pumps and valves, these variables can be adjusted in the manual mode section of the application as required.
  The varibales are pump1State, pump2State,valve1State and valve2State.

  #Notes
      pump1 status = 0; means pump 1 is off
      pump1 status = 1; Means pump 1 is on
      Similarly, pump 2 works like pump 1

      valve 1 status = 0; Means valve 1 is open
      valve1 status = 1; Means valve 1 is closed
      Similarly, valve 2 works like valve 1
  ====================================================================================================================================================
  The following function(sendSensorData()) is used to send the sensors data from arduino MEGA to the mobile application. To modify the sensor value change the send data argument.
*/

#include <SPI.h>
#include <SD.h>
#include "Adafruit_MAX31855.h"
#include <SeeedGrayOLED.h>
#include <Wire.h>
#include "DHT.h"
#include <L298NX2.h>
#include <EEPROM.h>
#include "cozir.h"

// settings
int gasSetPointAds = 250;  // gas set point during adsortion
int gasSetPointDes = 4000;  // gas set point during desortion
double tempMax = 80;
double tempMin = 75;

// starting state
int currentState = 1;  // initial state if = 0 testing // if = 1 ciclo

// timing
long heatIntervalOn = 20000;   // on interval for heat plates
long heatIntervalOff = 3000;  // off interval for heat plates
long sdInterval = 5000;       // SD saving interval
long printInterval = 5000;     // Serial Print interval
long checkInterval = 300000;   // Stabilization of new state interval// 300000 = 5 minutes. 600000 = 10 minutes (in miliseconds)

// for cozir sensor
COZIR czr[3] = {COZIR(&Serial1), COZIR(&Serial2), COZIR(&Serial3)}; // Connected to Hard Serial1, Hard Serial2 and Hard Serial3
uint32_t lastCO2[3] = {0, 0, 0};
float result1, result2, result3, gasResult;

// for ambient co2
long ambCO2;

uint32_t lastRead = 0;

bool calentar = true;  // boolean for heat control
bool check = false;

#define writeSdOn  //uncomment if you want to save into SD card
//#define oledOn //uncomment if you want to use the display

unsigned int conditionCounter = 0;  // Contador para que espere 1 minuto antes de volver a empezar el ciclo
unsigned long startTime = 0;
unsigned long previousMillisSD, currentMillisSD = 0;        // counter for SD saving
unsigned long currentMillisPrint, previousMillisPrint = 0;  //counter for Serial Print
unsigned long previousMillisHeat, currentMillisHeat = 0;    // counter for heating plates
unsigned long currentMillisCheck, previousMillisCheck;

#include "relays.h"
#include "modes.h"
#include "sensors.h"
#include "vaccumm.h"
#include "remote_control.h"
#include "sd_shield.h"
#include "oled_display.h"
#include <SoftwareSerial.h>

String receivedData;  // store receive data from BLE as a string
String sendData;      // a varibale to store to send data from device to mobile app
bool sending = true;

// Serial pin for Arduino mega and HM-10 BLE connection
const byte rxPin = A12;
const byte txPin = A13;

// Set up a new SoftwareSerial object for HM-10 BLE
SoftwareSerial HM10Serial(rxPin, txPin);

// define the pump and valves current status
bool pump1Status = 0;
bool pump2Status = 0;
bool valve1Status = 0;
bool valve2Status = 0;

unsigned long lastStatusCheckTime = 0;

// Function to read response from HM-10
String readBleResponse() {
  String msg = "";
  if (HM10Serial.available() > 0) {
    msg = HM10Serial.readStringUntil('\n');
    msg.trim();
    return msg;
  }
  return "";
}

// Function to write AT commands to HM-10
bool sendAtCommand(const char* command) {
  HM10Serial.print(command);
  String msg = readBleResponse();
  Serial.println(msg);
  return msg.indexOf("OK") >= 0;
}

// Initialize HM-10 settings
void initializeHM10() {
  sendAtCommand("AT");
  sendAtCommand("AT+RESET");
  sendAtCommand("AT+IMME1");
  sendAtCommand("AT+NOTI1");
  sendAtCommand("AT+NOTP1");
  sendAtCommand("AT+NAME=MicroBio");
  sendAtCommand("AT+UUID0x3935");
  sendAtCommand("AT+CHAR0xC29C");
  sendAtCommand("AT+ROLE0");
  sendAtCommand("AT+ADVI0");
  sendAtCommand("AT+ADTY0");
  sendAtCommand("AT+FLAG0");
  sendAtCommand("AT+IMME0");
  sendAtCommand("AT+MODE2");
}

// Send CO2 sensor data via HM-10 BLE
void sendSensorData(String dataFormat, int CO2sensorData) {
  sendData = dataFormat + String(CO2sensorData);
  sendAtCommand(sendData.c_str());
  Serial.println("Sent: " + sendData);
}

// Send Temperature sensor data via HM-10 BLE
void sendTempData(String dataFormat, float tempSensorData) {
  sendData = dataFormat + String(tempSensorData);
  sendAtCommand(sendData.c_str());
  Serial.println("Sent: " + sendData);
}

// Send acknowledge via HM-10 BLE to mobile app
void sendAcknowledgeData(String acknowledgeData) {
  sendAtCommand(acknowledgeData.c_str());
  Serial.println("Acknowledge Data Sent: " + acknowledgeData);
}

// Send the pumps status via HM-10 BLE
void sendPumpAndValveStatus(String sendpvData) {
  sendAtCommand(sendpvData.c_str());
  Serial.println("Sent: " + sendData);
}

void setup() {
  Wire.begin();
  delay(200);
  Serial.begin(115200);
  Serial.println("...initializing serial ports...");
  Serial1.begin(9600);     //CozirIn 100%
  Serial2.begin(9600);     //CozirOut 20%
  Serial3.begin(9600);     //CozirOut 20%
  delay(1000);
  startCozir();
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

  Serial.println("Sensors and Display Initialize completed!");
  Serial.println("HM-10 initializing...");
  HM10Serial.begin(9600);  // HM-10
  Serial.println("HM-10 is ready to pair to the app. Now Connect the App. ");
}

void loop() {
  receivedData = readBleResponse();   // read the BLE response from mobile application

  // Check the mobile app response
  if (receivedData.length() > 0) {
    Serial.println("Device connected");
    Serial.println(receivedData);
    if (receivedData == "pair") {     // handshaking device to the mobile application
      sendAcknowledgeData("s:pair");  // shared acknowledge bit: request accepted
    }

    if (receivedData == "od") {     // receive on-demand data from the mobile application
      while (1) {
        if (millis() - lastStatusCheckTime > 500) {   //wait every 500 seconds
          receivedData = readBleResponse();   // read the ble response again
          checkOthers(receivedData);
          if (receivedData == "stop") {       // stop sending on-demand data
            break;
          }
          if (receivedData == "pair") {     // handshaking check, if failed the connection while sending data
            sendAcknowledgeData("s:pair");
            break;
          }
          sendSensorData("co:", int(result1));      //send CO data to the mobile app, don't change the first parameter "co" CO2 OUT
          delay(100);
          receivedData = readBleResponse();
          checkOthers(receivedData);
          if (receivedData == "stop") {
            break;
          }
          if (receivedData == "pair") {
            sendAcknowledgeData("s:pair");
            break;
          }
          sendSensorData("ci:", int(result2));     //send CI data to the mobile app, don't change the first parameter "ci" CO2 IN
          delay(100);
          receivedData = readBleResponse();
          checkOthers(receivedData);
          if (receivedData == "stop") {
            break;
          }
          if (receivedData == "pair") {
            sendAcknowledgeData("s:pair");
            break;
          }
          sendSensorData("cm:", result3);    //send CO2-AMB data to the mobile app, don't change the first parameter "cm" CO2 AMB
          delay(100);
          receivedData = readBleResponse();
          checkOthers(receivedData);
          if (receivedData == "stop") {
            break;
          }
          if (receivedData == "pair") {
            sendAcknowledgeData("s:pair");
            break;
          }
          //sendTempData("tm:", DHTTemp);        //send Temperature data to the mobile app, don't change the first parameter "tm" TEMP
          sendTempData("tm:", result);
          delay(100);
          receivedData = readBleResponse();
          checkOthers(receivedData);
          if (receivedData == "stop") {
            break;
          }
          if (receivedData == "pair") {
            sendAcknowledgeData("s:pair");
            break;
          }
          lastStatusCheckTime = millis();
        }
        receivedData = readBleResponse();
        checkOthers(receivedData);
        if (receivedData == "stop") {
          break;
        }
        if (receivedData == "pair") {
          sendAcknowledgeData("s:pair");
          break;
        }
      }
    }

    // Handle other commands as needed.
    checkOthers(receivedData);
  }
}
// this function is used to send the current state of pumps and valves
void checkOthers(String receivedData) {
  if (receivedData == "cs") {
    String pvStatus = "p1:" + String(pump1Status) + "|p2:" + String(pump2Status) + "|v1:" + String(valve1Status) + "|v2:" + String(valve2Status);
    sendPumpAndValveStatus(pvStatus);
    delay(100);
    Serial.println(pvStatus);
  }


  // modify the pumps and valve status according to the BLE response.
  if (receivedData == "p1:0") {
    runVacuum(1, 0);    // Vacumm pump 1 OFF
    pump1Status = 0;
    String pvStatus = "p1:" + String(pump1Status) + "|p2:" + String(pump2Status) + "|v1:" + String(valve1Status) + "|v2:" + String(valve2Status);
    sendPumpAndValveStatus(pvStatus);
    delay(100);
    Serial.println("Pump 1 OFF");
  }
  if (receivedData == "p1:1") {
    runVacuum(1, 120);    // Vacumm pump 1 ON
    pump1Status = 1;
    String pvStatus = "p1:" + String(pump1Status) + "|p2:" + String(pump2Status) + "|v1:" + String(valve1Status) + "|v2:" + String(valve2Status);
    sendPumpAndValveStatus(pvStatus);
    delay(100);
    Serial.println("Pump 1 ON");
  }
  if (receivedData == "p2:0") {
    runVacuum(2, 0);    // Vacumm pump 2 OFF
    pump2Status = 0;
    String pvStatus = "p1:" + String(pump1Status) + "|p2:" + String(pump2Status) + "|v1:" + String(valve1Status) + "|v2:" + String(valve2Status);
    sendPumpAndValveStatus(pvStatus);
    delay(100);
    Serial.println("Pump 2 OFF");
  }
  if (receivedData == "p2:1") {
    runVacuum(2, 120);    // Vacumm pump 2 ON
    pump2Status = 1;
    String pvStatus = "p1:" + String(pump1Status) + "|p2:" + String(pump2Status) + "|v1:" + String(valve1Status) + "|v2:" + String(valve2Status);
    sendPumpAndValveStatus(pvStatus);
    delay(100);
    Serial.println("Pump 2 ON");
  }
  if (receivedData == "v1:0") {
    openValve(1); // valve 1 Open
    valve1Status = 0;
    String pvStatus = "p1:" + String(pump1Status) + "|p2:" + String(pump2Status) + "|v1:" + String(valve1Status) + "|v2:" + String(valve2Status);
    sendPumpAndValveStatus(pvStatus);
    delay(100);
    Serial.println("Valve 1 OFF");
  }
  if (receivedData == "v1:1") {
    closeValve(1); // valve 1 is closed
    valve1Status = 1;
    String pvStatus = "p1:" + String(pump1Status) + "|p2:" + String(pump2Status) + "|v1:" + String(valve1Status) + "|v2:" + String(valve2Status);
    sendPumpAndValveStatus(pvStatus);
    delay(100);
    Serial.println("Valve 1 ON");
  }
  if (receivedData == "v2:0") {
    openValve(2); // valve 2 Open
    valve2Status = 0;
    String pvStatus = "p1:" + String(pump1Status) + "|p2:" + String(pump2Status) + "|v1:" + String(valve1Status) + "|v2:" + String(valve2Status);
    sendPumpAndValveStatus(pvStatus);
    delay(100);
    Serial.println("Valve 2 OFF");
  }
  if (receivedData == "v2:1") {
    closeValve(2); // valve 2 is closed
    valve2Status = 1;
    String pvStatus = "p1:" + String(pump1Status) + "|p2:" + String(pump2Status) + "|v1:" + String(valve1Status) + "|v2:" + String(valve2Status);
    sendPumpAndValveStatus(pvStatus);
    delay(100);
    Serial.println("Valve 2 ON");
  }

  //HM-10 BLE operation End
  currentMillisHeat = millis();
  currentMillisPrint = millis();
  currentMillisSD = millis();
  currentMillisCheck = millis();

  switch (currentState) {
    case 0:  //for testing
      readCozir();
      readDHT();
      //readInTemperature();
      readAmbGas();   // read ambient gas sensor
      turnOffHeat();  // heat OFF

      runVacuum(1, 120);  //vacuum pump 1 ON
      pump1Status = 1; // pump 1 turn on
      runVacuum(2, 120);  // vacuum pump 2 OFF
      pump2Status = 0; // pump 2 turn off

      openValve(1);  // solenoid valve 1 open
      valve1Status = 0;
      openValve(2);  // solenoid valve 2 open
      valve2Status = 0;

      if (currentMillisCheck - previousMillisCheck >= checkInterval) {  // this state won't change until 5 minutes have passed to let all the parameters settle.
        check = true;
      } else {
        check = false;
      }

      if (check == true) {              // if 5 minutes have passed check if the conditions for passing to the next case are met
        conditionCounter++;             // Increment the counter
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

    case 1:  // adsorción
      readCozir();
      readDHT();
      readInTemperature();
      //readAmbGas();
      turnOffHeat();  // heat OFF


      runVacuum(1, 80);  //vacuum pump 1 ON
      pump1Status = 1; // pump 1 turn on
      runVacuum(2, 0);    // vacuum pump 2 OFF
      pump2Status = 0; // pump 2 turn off

      openValve(1);  // solenoid valve 1 open
      valve1Status = 0;
      openValve(2);  // solenoid valve 2 open
      valve2Status = 0;

      if (currentMillisCheck - previousMillisCheck >= checkInterval) {  // this state won't change until 5 minutes have passed to let all the parameters settle.
        check = true;
      } else {
        check = false;
      }

      if (check == true) {
        if (gasResult <= gasSetPointAds) {  // check if this happens for 5 minutes
          if (startTime == 0) {             // Check if timer is not running, start it
            startTime = millis();           // Record start time
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

    case 2:  // transición adsorción desorción
      readCozir();
      readDHT();
      readInTemperature();
      //readAmbGas();
      runVacuum(1, 0);  //vacuum pump 1 OFF
      pump1Status = 0; // pump 1 turn off
      runVacuum(2, 0);  // vacuum pump 2 OFF
      pump2Status = 0; // pump 2 turn off

      closeValve(1);  // solenoid valve 1 closed
      valve1Status = 1;
      closeValve(2);  // solenoid valve 2 closed
      valve2Status = 1;

      turnOnHeat();

      if (currentMillisCheck - previousMillisCheck >= 120000) {  // this state won't change until 2 minutes have passed to let all the parameters settle.
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

          if (conditionCounter >= 42000) {  // Hold 30 min at max temp
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

    case 3:  // desorción
      readCozir();
      readDHT();
      readInTemperature();
      //readAmbGas();
      runVacuum(1, 0);    //vacuum pump 1 OFF
      pump1Status = 0; // pump 1 turn off
      runVacuum(2, 70);  // vacuum pump 2 ON (ENCENDER BOMBA NITROGENO)
      pump2Status = 1; // pump 1 turn off

      closeValve(1);  // solenoid valve 1 closed (cerrar valvula entrada aire)
      valve1Status = 1;
      openValve(2);   // solenoid valve 2 open (abrir valvula salida aire)
      valve2Status = 0;

      if (result <= tempMax && result >= tempMin) {  // 10ºC = false // 40ºC = true // 90ºC = false
        turnOnHeat();
      }

      if (result <= tempMax && result <= tempMin) {  // 10ºC = true // 40ºC = false // 90ºC = false
        turnOnHeat();
      }

      if (result >= tempMax) {
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
        if (result2 <= gasSetPointDes) {  // check if this happens for 1 minute
          if (startTime == 0) {           // Check if timer is not running, start it
            startTime = millis();         // Record start time
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
      //readAmbGas();
      turnOffHeat();

      runVacuum(1, 0);
      pump1Status = 0; // pump 1 turn off
      runVacuum(2, 70);  // vacuum pump 2 ON (ENCENDER BOMBA NITROGENO)
      pump2Status = 1; // pump 1 turn off

      closeValve(1);  // solenoid valve 1 closed
      valve1Status = 1;
      openValve(2);   // solenoid valve 2 open (abrir valvula salida aire)
      valve2Status = 0;

      if (currentMillisCheck - previousMillisCheck >= checkInterval) {
        check = true;
      } else {
        check = false;
      }

      if (check == true) {
        if (result <= 30) {        // check if this happens for 5 minutes, then restart the cycle, DHTTemp
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
    sendAcknowledgeData("mds:1");
    Serial.println(" - adsorción");
  } else if (currentState == 2) {
    sendAcknowledgeData("mds:2");
    Serial.println(" - transición ads - des");
  } else if (currentState == 3) {
    sendAcknowledgeData("mds:3");
    Serial.println(" - desorción");
  } else if (currentState == 4) {
    sendAcknowledgeData("mds:4");
    Serial.println(" - transición des - ads");
  } else if (currentState == 0) {
    sendAcknowledgeData("mds:0");
    Serial.println(" - testing");
  }

  Serial.print("CO2 1: ");
  Serial.print(lastCO2[0]);
  Serial.println(" PPM");
  Serial.print("CO2 2: ");
  Serial.print(lastCO2[1]);
  Serial.println(" PPM");
  Serial.print("CO2 3: ");
  Serial.print(lastCO2[2]);
  Serial.println(" PPM");

  Serial.print("result 1: ");
  Serial.print(result1);
  Serial.println(" PPM");
  Serial.print("result 2: ");
  Serial.print(result2);
  Serial.println(" PPM");
  Serial.print("result 3: ");
  Serial.print(result3);
  Serial.println(" PPM");

  Serial.print("Gas compare: ");
  Serial.print("Difference = ");
  Serial.print(gasResult);
  Serial.println(" PPM");

  //  Serial.print("Ambient CO2: ");
  //  Serial.print(ambCO2);
  //  Serial.println(" PPM");

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
