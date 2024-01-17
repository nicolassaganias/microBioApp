

void startRemoteControl() {
  //BOTONERA
#define pinModMan 3 // 3 - 28 antes - nueva
#define pinModAut  8 // 8 - 33
#define pinModAutTimer  5 // 5 // 5- 30
#define pinModAutSensor 6 // 6 // 6 - 31
#define pinModManOn  7 // 7 - 32
#define pinModManOff  9 //9 - 29
  int estadoModoOn, estadoModoOff;

  //BOTONERA
  pinMode (pinModMan, INPUT_PULLUP);
  pinMode (pinModAut, INPUT_PULLUP);
  pinMode (pinModAutTimer, INPUT_PULLUP);
  pinMode (pinModAutSensor, INPUT_PULLUP);
  pinMode (pinModManOn, INPUT_PULLUP);
  pinMode (pinModManOff, INPUT_PULLUP);
}

void leerBotonera() {
  int estadoModoOn = digitalRead(pinModManOn);
  int estadoModoOff = digitalRead(pinModManOff);
  int estadoModoManual = digitalRead(pinModMan);
  int estadoModoAuto = digitalRead(pinModAut);
  int estadoModoTimer = digitalRead(pinModAutTimer);
  int estadoModoSensores = digitalRead(pinModAutSensor);

  if (estadoModoManual == 1 && estadoModoAuto == 0) {
    if (estadoModoOn == 1 && estadoModoOff == 0) {
      turnOnHeat();
      //Serial.println("calentando");
    } else {
      turnOffHeat();
      //Serial.println("enfriando");
    }
  }

  else if (estadoModoManual == 0 && estadoModoAuto == 1) {
    if (estadoModoTimer == 1 && estadoModoSensores == 0) {
      modoTimer();
      //Serial.println("timer");
    } else {
      modoSensores();
      // Serial.println("sensores");
    }
  }

  
    /*Serial.print("Man:");
    Serial.print(estadoModoManual);
    Serial.print(" Auto: ");
    Serial.print(estadoModoAuto);
    Serial.print(" ON:");
    Serial.print(estadoModoOn);
    Serial.print(" OFF:");
    Serial.print(estadoModoOff);
    Serial.print(" Timer:");
    Serial.print(estadoModoTimer);
    Serial.print(" Sensores:");
    Serial.println(estadoModoSensores);*/
}
