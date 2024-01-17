//SD SHIELD
//USA LOS PINES 4,10,11,12 y 13
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 10;
File myFile;
void writeSD() {
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  if (dataFile) {
    Serial.println("Escribiendo en la tarjeta SD");
    dataFile.print("Tiempo(ms):,");
    dataFile.print(millis());
    dataFile.print(", CO2 IN:,");
    dataFile.print(result1);
    dataFile.print(", CO2 OUT:,");
    dataFile.print(result2);
    dataFile.print(", T In:,");
    dataFile.print(result);
    dataFile.print(", Amb CO2:,");
    dataFile.print(ambCO2);
    dataFile.print(", T Out:,");
    dataFile.print(DHTTemp);

    dataFile.print(",state, ");
    dataFile.print(currentState);
    if (currentState == 1) {
      dataFile.println(" - adsorcion,");
    } else if (currentState == 2) {
      dataFile.println(" - transicion ads-des,");
    } else if (currentState == 3) {
      dataFile.println(" - desorcion,");
    } else if (currentState == 4) {
      dataFile.println(" - transicion des-ads,");
    } else if (currentState == 0) {
      dataFile.println(" - testing,");
    }

    dataFile.close();  //cerramos el archivo
  } else {
    Serial.println("Error al abrir el archivo");
  }
}
