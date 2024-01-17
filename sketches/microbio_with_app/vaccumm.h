// Pin definition
#define EN_A 44
#define IN1_A 29
#define IN2_A 30

#define IN3_B 31
#define IN4_B 32
#define EN_B 45

#define EN_C 48
#define IN1_C 49
#define IN2_C 50

#define IN3_D 51
#define IN4_D 52
#define EN_D 53

// Initialize the four motors
L298NX2 motors1(EN_A, IN1_A, IN2_A, EN_B, IN3_B, IN4_B);
L298NX2 motors2(EN_C, IN1_A, IN2_A, EN_D, IN3_B, IN4_B);

void runVacuum(int which, int speedness) {
  switch (which) {
    case 0:
      break;

    case 1:
      motors1.setSpeedA(speedness);
      motors1.forwardA();

      if (speedness == 0) {
        motors1.stopA();
      } else {
      }

      break;

    case 2:
      motors1.setSpeedB(speedness);
      motors1.forwardB();

      if (speedness == 0) {  // si no funciona chequear si tengo qu crear speedness2..3..y 4
        motors1.stopB();
      }
      break;

    case 3:
      motors2.setSpeedA(speedness);
      motors2.forwardA();

      if (speedness == 0) {  // si no funciona chequear si tengo qu crear speedness3..3..y 4
        motors2.stopA();
      }
      break;

    case 4:
      motors2.setSpeedB(speedness);
      motors2.forwardB();

      if (speedness == 0) {  // si no funciona chequear si tengo qu crear speedness4..4..y 4
        motors2.stopB();
      }
      break;
  }
}
