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

  result1 = (float)lastCO2[0] * 1.3409 - 230.59;
  if (result1 <= 0) {
    result1 = 0;
  }

  gasResult =  result1;

}
