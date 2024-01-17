/* RELAYS:
  }
    Modulo de 4 relays. 24v.
  1 = Valvula Solenoide 1
  2 = Valvula Solenoide 2
  3 = Lamina de arriba
  4 = Lamina de abajo

  Módulos de motor
  Modulo 1 al 4 = bombas de vacío controladas por L298N
*/

//Relays
const int relayPins = 4;
const int whichRelay[] = { 0, 36, 37, 35, 34 };  // 1 = V1 2 = V2 3 = D1 4 = D2 V = Valvula Solenoide D = Placa Térmica

void turnOnHeat() {
  //Serial.println("HEAT ON");
  if (currentMillisHeat - previousMillisHeat >= heatIntervalOn) { // turn on and off in intervals for the material not to break for excesive heat
    if (calentar == false) {
      previousMillisHeat = currentMillisHeat;
      calentar = true;
      digitalWrite(whichRelay[3], HIGH);
      digitalWrite(whichRelay[4], HIGH);
    }
  }

  if (currentMillisHeat - previousMillisHeat >= heatIntervalOff) { // turn on and off in intervals for the material not to break for excesive heat
    if (calentar == true) {
      previousMillisHeat = currentMillisHeat;
      calentar = false;
      digitalWrite(whichRelay[3], LOW);
      digitalWrite(whichRelay[4], LOW);
    }
  }
}

void turnOffHeat() {
  // Serial.println("HEAT OFF");
  digitalWrite(whichRelay[3], HIGH);
  digitalWrite(whichRelay[4], HIGH);
}

void openValve(int which) {
  digitalWrite(whichRelay[which], LOW);
  // Serial.print("relay ");
  // Serial.print(which);
  // Serial.println(" on");
}

void closeValve(int which) {
  digitalWrite(whichRelay[which], HIGH);
  // Serial.print("relay ");
  // Serial.print(which);
  // Serial.println(" off");
}
