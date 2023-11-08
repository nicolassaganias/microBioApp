//THERMOCOUPLE#1
int thermoDO1 = 22;
int thermoCS1 = 23;
int thermoCLK1 = 24;
Adafruit_MAX31855 thermocouple(thermoCLK1, thermoCS1, thermoDO1);

//THERMOCOUPLE #2
int thermoDO2 = 25;
int thermoCS2 = 26;
int thermoCLK2 = 27;

Adafruit_MAX31855 thermocouple2(thermoCLK2, thermoCS2, thermoDO2);

double i, c, i2, c2, result, previousResult, resultIn, resultOut, tempResult;
int t1, h1, t2, h2, d, d2;

// Ambience Temperature - Humidity
#define DHTPIN 6
double DHTTemp, DHTHumidity;
#define DHTTYPE DHT11  // DHT 11
DHT dht(DHTPIN, DHTTYPE);

void readDHT() {
  DHTTemp = dht.readTemperature();  // Read temperature as Celsius (the default)
}
void readInTemperature() {
  i = thermocouple.readInternal();
  c = thermocouple.readCelsius();
  d = c;


  i2 = thermocouple2.readInternal();
  c2 = thermocouple2.readCelsius();
  d2 = c2;

  if (isnan(c) || isnan(c2)) {
    result = previousResult;
  } else {
    result = (c + c2) / 2;
    previousResult = result;
  }

}

void readCozir() {

  lastCO2[0] = czr[0].CO2() * 10; // * czr[0].getPPMFactor(); // add  * czr.getPPMFactor();  most of time PPM = one.
  lastCO2[1] = czr[1].CO2() * 100; // * czr[1].getPPMFactor(); // add  * czr.getPPMFactor();  most of time PPM = one.

  // curva calibracion sensor 1
  
  //  if (lastCO2[0] >= 250000) { // very high concentration
  //    result1 = (float)lastCO2[0] * 1.0111 + 11184;
  //  } else if (lastCO2[0] >= 1000 && lastCO2[0] < 250000) { // high concentration
  //    result1 = (float)lastCO2[0] * 0.9099 - 52.82;
  //  } else if (lastCO2[0] < 1000) { // low concentration
  //    result1 = (float)lastCO2[0] * 1.2287 - 683.17;
  //  }

  result1 = (float)lastCO2[0] * 1.3409 - 230.59;
  if (result1 <= 0) {
    result1 = 0;
  }

  // curva calibracion sensor 2
  if (lastCO2[1] >= 250000) { // very high concentration
    result2 = (float)lastCO2[1] * 1.0111 + 11184;
  } else if (lastCO2[1] >= 1500 && lastCO2[1] < 250000) { // high concentration
    result2 = (float)lastCO2[1] * 0.9414 - 515.16;
  } else if (lastCO2[1] < 1500) { // low concentration
    result2 = (float)lastCO2[1] * 1.9354 - 1354.8;
  }

  if (result2 <= 0) {
    result2 = 0;
  }


  if (result1 >= result2) {
    gasResult = result1 - result2;
    if (gasResult <= 0) {
      gasResult = 0;
    }
  } else {
    gasResult = result2 - result1;
    if (gasResult <= 0) {
      gasResult = 0;
    }
  }
}

void readAmbGas() {
  unsigned char hexdata[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };  //Read the gas density command /Don't change the order

  Serial3.write(hexdata, 9);
  for (int i = 0, j = 0; i < 9; i++) {
    if (Serial3.available() > 0) {
      long hi, lo, CO2;
      int ch = Serial3.read();

      if (i == 2) {
        hi = ch;  //High concentration
      }
      if (i == 3) {
        lo = ch;  //Low concentration
      }
      if (i == 8) {
        CO2 = hi * 256 + lo;  //CO2 concentration
        ambCO2 = CO2;
      }
    }
  }
}
