// Libraries
#include <SPI.h>      // Incluye la biblioteca SPI para la comunicación en serie
#include <MFRC522.h>  // Incluye la biblioteca MFRC522 para leer etiquetas RFID


// Constants
#define SS_PIN 5   // Define el pin de selección de esclavo (SS) como el pin 5
#define RST_PIN 0  // Define el pin de reinicio (RST) como el pin 0

//motor
const int MOTOR_VERDE = 33;
const int MOTOR_AZUL = 25;
const int MOTOR_NARANJA = 26;

const int MOTOR_VELOCIDAD_MAXIMA = 255;
//motor

// Variables
byte nuidPICC[4] = { 0, 0, 0, 0 };  // Array para almacenar el UID de la tarjeta RFID
MFRC522::MIFARE_Key key;            // Crea una variable para la clave MIFARE
MFRC522 rfid(SS_PIN, RST_PIN);      // Inicializa el lector RFID con los pines definidos


void setup() {
  // Init Serial USB
  Serial.begin(115200);                    // Inicia la comunicación serie a 115200 baudios
  Serial.println(F("Initialize System"));  // Imprime un mensaje en el monitor serie
  // Init RFID
  SPI.begin();      // Inicia la comunicación SPI
  rfid.PCD_Init();  // Inicializa el lector RFID


  Serial.print(F("Reader: "));     // Imprime "Reader: " en el monitor serie
  rfid.PCD_DumpVersionToSerial();  // Imprime la versión del lector RFID
  
  //Inicio código motor
  pinMode(MOTOR_VERDE, OUTPUT);
  pinMode(MOTOR_AZUL, OUTPUT);
  pinMode(MOTOR_NARANJA, OUTPUT);
  //Fin código motor


  //Inicio código motor
  girarMotor(MOTOR_VELOCIDAD_MAXIMA);
  //Fin código motor
}


void loop() {
  readRFID();  // Llama a la función readRFID en el bucle principal
}

//Inicio código motor
void girarMotor(int velocidad) {
  analogWrite(MOTOR_VERDE, velocidad);
  digitalWrite(MOTOR_NARANJA, LOW);
  digitalWrite(MOTOR_AZUL, HIGH);
}

void girarMotorReversa(int velocidad) {
  analogWrite(MOTOR_VERDE, velocidad);
  digitalWrite(MOTOR_NARANJA, HIGH);
  digitalWrite(MOTOR_AZUL, LOW);
}
//Fin código motor

void readRFID() {
  // Read RFID card


  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  // Establece la clave MIFARE en 0xFF para cada byte
  }


  // Look for new cards
  if (!rfid.PICC_IsNewCardPresent()) {  // Verifica si hay una nueva tarjeta presente
    return;                             // Si no hay tarjeta, sale de la función
  }


  // Verify if the NUID has been read
  if (!rfid.PICC_ReadCardSerial()) {  // Intenta leer el UID de la tarjeta
    return;                           // Si no se puede leer, sale de la función
  }


  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];  // Almacena el UID leído en el array nuidPICC
  }


  Serial.print(F("RFID in dec: "));           // Imprime el mensaje "RFID in dec: "
  printDec(rfid.uid.uidByte, rfid.uid.size);  // Llama a la función printDec para imprimir el UID en decimal
  Serial.println();                           // Imprime una nueva línea en el monitor serie


  // Halt PICC
  rfid.PICC_HaltA();  // Detiene la comunicación con la tarjeta RFID


  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();  // Detiene la encriptación en el lector
}


/**
  Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  if (buffer != nullptr) {  // Comprueba si el puntero no es nulo
    for (byte i = 0; i < bufferSize; i++) {
      Serial.print(buffer[i] < 0x10 ? " 0" : " ");  // Formatea la salida
      Serial.print(buffer[i], HEX);                 // Imprime el byte en formato hexadecimal
    }
  }
}


/**
  Helper routine to dump a byte array as dec values to Serial.
*/
void printDec(byte *buffer, byte bufferSize) {
  if (buffer != nullptr) {  // Comprueba si el puntero no es nulo
    for (byte i = 0; i < bufferSize; i++) {
      Serial.print(buffer[i] < 0x10 ? " 0" : " ");  // Formatea la salida
      Serial.print(buffer[i], DEC);                 // Imprime el byte en formato decimal
    }
  }
}