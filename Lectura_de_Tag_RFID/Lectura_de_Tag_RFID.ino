


#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21  // Pin de selección de chip
#define RST_PIN 22 // Pin de reset

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;          

const int TAG_RFID_NRO_BLOQUE_DE_DATOS = 1;  // Puedes cambiar esto a otro bloque si es necesario
byte blockData[16];  // Array para datos a escribir
byte bufferLen = 18; // Longitud del buffer de lectura
byte readBlockData[18];

MFRC522::StatusCode status;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.print("Acerque el tag para leer el bloque ");
  Serial.println(TAG_RFID_NRO_BLOQUE_DE_DATOS);
}

void loop() {
  // Preparar la llave para la autenticación
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF; // Clave predeterminada
  }

  // Comprobar si hay una nueva tarjeta presente
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Leer la tarjeta seleccionada
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
 
  Serial.println("** Tarjeta detectada **");
  Serial.print(F("Card UID: "));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();


  // Leer los datos del bloque
  Serial.println("Leyendo del bloque de datos...");
  ReadDataFromBlock(TAG_RFID_NRO_BLOQUE_DE_DATOS, readBlockData);

  // Imprimir los datos leídos del bloque
  Serial.print("Datos en el bloque ");
  Serial.print(TAG_RFID_NRO_BLOQUE_DE_DATOS);
  Serial.print(" --> ");
  for (int j = 0; j < 16; j++) {
    Serial.write(readBlockData[j]);
  }
  Serial.println();
}

void WriteDataToBlock(int blockNum, byte blockData[]) {
  // Autenticación del bloque para escritura
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Fallo en la autenticación para escritura: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println("Autenticación exitosa para escritura");
  }

  // Escribir datos en el bloque
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Fallo al escribir en el bloque: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println("Datos escritos exitosamente en el bloque");
  }
}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  // Autenticación del bloque para lectura
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Fallo en la autenticación para lectura: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println("Autenticación exitosa para lectura");
  }

  // Leer datos del bloque
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Fallo al leer: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println("Bloque leído exitosamente");
  }
}
