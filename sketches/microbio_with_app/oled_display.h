void startOled() {
  SeeedGrayOled.init(SH1107G);
  SeeedGrayOled.clearDisplay();
  SeeedGrayOled.setNormalDisplay();
  SeeedGrayOled.setVerticalMode();
  SeeedGrayOled.setGrayLevel(1);  //Set Grayscale level. Any number between 0 - 15.
}

void printOled() {
  SeeedGrayOled.clearDisplay();

  SeeedGrayOled.setTextXY(1, 0);  //set Cursor to ith line, 0th column
  //  SeeedGrayOled.putString("Estado:");
  //  SeeedGrayOled.putNumber(currentState);

  if (currentState == 1) {
    SeeedGrayOled.putString("adsorcion");
  } else if (currentState == 2) {
    SeeedGrayOled.putString("trans ads-des");
  } else if (currentState == 3) {
    SeeedGrayOled.putString("desorcion");
  } else if (currentState == 4) {
    SeeedGrayOled.putString("trans des-ads");
  }

  SeeedGrayOled.setTextXY(3, 0);  //set Cursor to ith line, 0th column
  SeeedGrayOled.putString("T Interna:");

  if (isnan(result)) {
    SeeedGrayOled.putString("NAN");
  } else {
    SeeedGrayOled.putNumber(result);
    SeeedGrayOled.putString("C");
  }

  SeeedGrayOled.setTextXY(5, 0);  //set Cursor to ith line, 0th column
  SeeedGrayOled.putString("CO2 In:");
  SeeedGrayOled.putNumber(result1);  //co21avg // co2filt1
  SeeedGrayOled.putString("");

  SeeedGrayOled.setTextXY(7, 0);  //set Cursor to ith line, 0th column
  SeeedGrayOled.putString("CO2 Out:");
  SeeedGrayOled.putNumber(result2); // co22avg // co2filt2
  SeeedGrayOled.putString("");

  SeeedGrayOled.setTextXY(9, 0);  //set Cursor to ith line, 0th column
  SeeedGrayOled.putString("Diference:");
  SeeedGrayOled.putNumber(gasResult);

  SeeedGrayOled.setTextXY(11, 0);  //set Cursor to ith line, 0th column
  SeeedGrayOled.putString("Temp Amb:");
  SeeedGrayOled.putNumber(DHTTemp);

  if (currentState == 1) {
    SeeedGrayOled.setTextXY(13, 0);  //set Cursor to ith line, 0th column
    SeeedGrayOled.putString("setPoint:");
    SeeedGrayOled.putNumber(gasSetPointAds);
  }
  else if (currentState == 3) {
    SeeedGrayOled.setTextXY(13, 0);  //set Cursor to ith line, 0th column
    SeeedGrayOled.putString("setPoint:");
    SeeedGrayOled.putNumber(gasSetPointDes);
  }
}
