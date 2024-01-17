// MODO SENSORES temperatura máxima a la que se apaga y minima a la que se vuelve a prender
// JUST TO USE IF Remote Control is Enabled and Connected

// MODO TEMPORIZADOR 5min
long heatInterval = 2000;//300000;

void modoTimer() {
  //TEMPORIZADOR
  if (currentMillisHeat - previousMillisHeat >= heatInterval) {
    previousMillisHeat = currentMillisHeat;

    if (calentar == true) {
      calentar = false;
      turnOffHeat();
      Serial.println("timer: enfriando");
    }
    else if (calentar == false) {
      calentar = true;
      turnOnHeat();
      Serial.println("timer: calentando");
    }
  }
}

void modoSensores() {

  /*if (currentMillisSensores - previousMillisSensores >= sensoresInterval) {
    previousMillisSensores = currentMillisSensores;
    readInTemperature();
    readSHT31(1);
    }

    if (result <= tempMax && result >= tempMin) { // 10 = false // 40 = true // 90 = false
    turnOnHeat();
    //Serial.println("sensores: calentando");
    }

    if (result <= tempMax && result <= tempMin) { // 10 = true // 40 = false // 90 = false
    turnOnHeat();
    // Serial.println("sensores: enfriando");
    }

    if (result >= tempMax ) {
    turnOffHeat();
    // Serial.println("sensores: enfriando"); // 10 = false // 40 = false // 90 = true
    }*/
}
