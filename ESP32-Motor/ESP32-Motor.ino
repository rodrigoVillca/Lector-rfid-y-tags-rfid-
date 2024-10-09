const int MOTOR_VERDE = 33;
const int MOTOR_AZUL = 25;
const int MOTOR_NARANJA = 26; 

const int MOTOR_VELOCIDAD_MAXIMA = 255;

void setup()
{
  pinMode(MOTOR_VERDE, OUTPUT);
  pinMode(MOTOR_AZUL, OUTPUT);
  pinMode(MOTOR_NARANJA, OUTPUT);
}
void loop()
{  
  girarMotor(MOTOR_VELOCIDAD_MAXIMA);
  delay(1000);
  girarMotorReversa(MOTOR_VELOCIDAD_MAXIMA);
  delay(1000);
}

void girarMotor(int velocidad)
{
  analogWrite(MOTOR_VERDE, velocidad);
  digitalWrite(MOTOR_NARANJA, LOW);
  digitalWrite(MOTOR_AZUL, HIGH);
}

void girarMotorReversa(int velocidad)
{
  analogWrite(MOTOR_VERDE, velocidad);
  digitalWrite(MOTOR_NARANJA, HIGH);
  digitalWrite(MOTOR_AZUL, LOW);
}