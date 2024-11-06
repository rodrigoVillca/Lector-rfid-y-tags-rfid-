//Incluir bibliotecas
#include <WiFi.h>  //
#include <PubSubClient.h>

//Bibliotecas para el lector RFID

#include <SPI.h>  // Incluye la biblioteca SPI para la comunicación en serie
#include <MFRC522.h> // Incluye la biblioteca MFRC522 para leer etiquetas RFID

// Constantes de RFID
#define SS_PIN 5   // Define el pin de selección de esclavo (SS) como el pin 5
#define RST_PIN 0  // Define el pin de reinicio (RST) como el pin 0

// Variables
byte nuidPICC[4] = { 0, 0, 0, 0 };  // Array para almacenar el UID de la tarjeta RFID
MFRC522::MIFARE_Key key;            // Crea una variable para la clave MIFARE
MFRC522 rfid(SS_PIN, RST_PIN);      // Inicializa el lector RFID con los pines definidos
#define RFID_NUMERO_DE_BLOQUE 1     //Número de bloque donde se guardan el nombre de aula en la memoria del tag RFID

// crear constantes con valores de configuracion(p. ej. contraseña de WiFi)
const char* WIFI_SSID = "ETEC-UBA";       // SSID( nombre de la red WiFi)
const char* CLAVE = "ETEC-alumnos@UBA";   // Contraseña de wifi
const char* MQTT_BROKER = "10.9.121.244";  // MQTT Broker
const int PUERTO_MQTT = 1883;             //Puerto MQTT
const char* MQTT_TOPIC = "topic-prueba";  //Topic sin "#" y
const char* MQTT_LOG_TOPIC = "logs";

//inicioCodigoMotor
const int MOTOR_VERDE = 33;
const int MOTOR_AZUL = 25;
const int MOTOR_NARANJA = 26; 

const int MOTOR_VELOCIDAD_MAXIMA = 255;
//FinCodigoMotor

//crear objetos para gestionar las conexiones

String aula = "";  // creo esta variable para guardar lo que viene de MQTT
WiFiClient Cliente_esp;
PubSubClient client(Cliente_esp);


// callback sirve para manejar y responder a los mensajes que llegan al dispositivo desde un servidor MQTT (el broker)
//Y ESTE BLOQUE DE CODIGO SE EJECUTA CUANDO LA PLACA RESIVE UN MSJ POR MQTT
void callback(char* topic, byte* message, unsigned int length) {
  // Imprimir el mensaje recibido en el topic
  Serial.print("Mensaje recibido en el topic: ");
  Serial.print(topic);
  Serial.print(".Mensaje: ");
  // Crear una cadena de caracteres (String) para almacenar el mensaje
  String mensaje;

  // Recorrer cada byte del mensaje recibido
  for (int i = 0; i < length; i++) {
    // Imprimir cada caracter del mensaje en el monitor serial
    Serial.print((char)message[i]);
    // Concatenar cada caracter al mensaje String
    mensaje += (char)message[i];
  }
  aula = mensaje;  // la variable mensaje del callback que guarda lo que viene por MQTT se pierde una vez que finaliza la funcion
                   // entonces lo que hago aca es guardarla en "aula" q es la variable que ya cree arriba

  // Imprimir un salto de línea para separar el mensaje recibido
  Serial.println();
}

void setup() {

// Init Serial USB
  Serial.begin(115200);                    // Inicia la comunicación serie a 115200 baudios
  Serial.println(F("Initialize System"));  // Imprime un mensaje en el monitor serie
  // Init RFID
  SPI.begin();      // Inicia la comunicación SPI
  rfid.PCD_Init();  // Inicializa el lector RFID


  Serial.print(F("Reader: "));     // Imprime "Reader: " en el monitor serie
  rfid.PCD_DumpVersionToSerial();  // Imprime la versión del lector RFID


  //inicioCodigoMotor
  pinMode(MOTOR_VERDE, OUTPUT);
  pinMode(MOTOR_AZUL, OUTPUT);
  pinMode(MOTOR_NARANJA, OUTPUT);
  //FinCodigoMotor

  //  Iniciar puerto serie (para enviar mensajes informando el estado de la conexión de WiFi, los mensajes que recibimos por MQTT, etc.)
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, CLAVE);
  Serial.print("Intentando conectar a wifi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Ya se conecto a WiFi");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_BROKER, PUERTO_MQTT);
  // ACA LE DIGO QUIEN ERA EL CALLBACK
  client.setCallback(callback);


  // Conectarse al broker MQTT
  while (!client.connected()) {
    Serial.println("conectando a MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("conectado");
      client.subscribe(MQTT_TOPIC);                   // me suscribo al topic
      client.publish(MQTT_LOG_TOPIC, "ESP32 conectado");  // publico mensaje avisando que me conecté


    } else {
      Serial.print("Fallo en el estado");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {
   readRFID();  // Llama a la función readRFID en el bucle principal

  client.loop();

  if (aula != "") {                    // si la variable "aula" es diferente de null  me muestra lo que guarde en la variable que
    Serial.print("Recibi: ");  // que en este caso seria el numero del aula
    Serial.println(aula);
    
    
    if(encontroAula(aula))
      Serial.println("Llave servida");
      
    else
      Serial.println("No se encontro la llave");

    aula = "";  // lo que hago aca es que una vez que me muestra lo que le pedi que seria el numero del aula
    //hace que la varible vuelva a estar vacia para que puedan entrar otras aulas
    
    
  }
}

bool encontroAula(String aula)
{
  Serial.print("Buscando: ");
  Serial.println(aula);
  //aca tengo que programar un bucle while que haga girar el carrusel asta que encuentre el aula que esta buscando en el tag (como por ejemplo "aula 314") 
    
     girarMotor(MOTOR_VELOCIDAD_MAXIMA);   //hago girar el motor
    //repetir las próximas instrucciones MIENTRAS QUE no encuentre la tarjeta Y NO haya dado una vuelta completa
    while(1 == 0) //infinito. ToDo: hasta que de vuelta completa (por si la llave no está) o timeout
    {
      byte datosLeidosDelTag[16]; //los bloques son de 16 bytes
      //Leo el tag
      ReadDataFromBlock(RFID_NUMERO_DE_BLOQUE, datosLeidosDelTag);
      //convertir los datosLeidosDelTag a String: 
      String datosLeidosDelTagString = String((char*) datosLeidosDelTag);

      //if(aula == datosLeidosDelTagString)
      if(aula.equals(datosLeidosDelTagString))
      {//si es el aula del tag es la que estoy buscando  (comparo dos variables Strings que serian aula y datosLeidosDelTag)
        detenerMotor();
        return true;
      }
    }
    return false;
  
  
  //inicioCodigoMotor
  //girarMotor(MOTOR_VELOCIDAD_MAXIMA);
 // delay(1000);
//  detenerMotor();
  //FinCodigoMotor

  return true;
}
//inicioCodigoMotor
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

void detenerMotor()
{
  digitalWrite(MOTOR_NARANJA, LOW);
  digitalWrite(MOTOR_AZUL, LOW);
}
//FinCodigoMotor

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

