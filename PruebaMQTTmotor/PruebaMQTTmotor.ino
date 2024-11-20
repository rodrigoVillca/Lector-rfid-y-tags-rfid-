//Incluir bibliotecas
#include <WiFi.h>  //
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//Bibliotecas para el lector RFID

#include <SPI.h>      // Incluye la biblioteca SPI para la comunicación en serie
#include <MFRC522.h>  // Incluye la biblioteca MFRC522 para leer etiquetas RFID

// Constantes de RFID
#define SS_PIN 5   // Define el pin de selección de esclavo (SS) como el pin 5
#define RST_PIN 0  // Define el pin de reinicio (RST) como el pin 0

//Crear el objeto lcd  dirección  0x27 y 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x27, 16, 2);  //

// Variables
byte nuidPICC[4] = { 0, 0, 0, 0 };  // Array para almacenar el UID de la tarjeta RFID
MFRC522::MIFARE_Key key;            // Crea una variable para la clave MIFARE
MFRC522 rfid(SS_PIN, RST_PIN);      // Inicializa el lector RFID con los pines definidos

MFRC522::StatusCode status;

#define RFID_NUMERO_DE_BLOQUE 1  //Número de bloque donde se guardan el nombre de aula en la memoria del tag RFID
#define RFID_BUFFER_SIZE 18      // Longitud del buffer de lectura (16 bytes es el tamaño del bloque, pero necesita 2 bytes extra)

// crear constantes con valores de configuracion(p. ej. contraseña de WiFi)
const char* WIFI_SSID = "ETEC-UBA";        // SSID( nombre de la red WiFi)
const char* CLAVE = "ETEC-alumnos@UBA";    // Contraseña de wifi
const char* MQTT_BROKER = "10.9.121.203";  // MQTT Broker
const int PUERTO_MQTT = 1883;              //Puerto MQTT
const char* MQTT_TOPIC_ESP_SUSCRIBE = "topic-prueba";  // Este el topic donde recibo el numero de aula 
const char* MQTT_LOG_TOPIC = "logs"; //
const char* MQTT_TOPIC_ESP_PUBLICA = "aula-retirada"; 

//inicioCodigoMotor
const int MOTOR_VERDE = 33;
const int MOTOR_AZUL = 25;
const int MOTOR_NARANJA = 26;

const int MOTOR_VELOCIDAD_MAXIMA = 255;

//FinCodigoMotor

String aula = "";  // creo esta variable para guardar lo que viene de MQTT
WiFiClient Cliente_esp;
PubSubClient client(Cliente_esp);


// callback sirve para manejar y responder a los mensajes que llegan al dispositivo desde un servidor MQTT (el broker)
//Y ESTE BLOQUE DE CODIGO SE EJECUTA CUANDO LA PLACA RECIBE UN MSJ POR MQTT
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
  Serial.begin(115200);  // Inicia la comunicación serie a 115200 baudios
  Serial.println(F("Llavero ETEC-UBA"));
  // Imprime un mensaje en el monitor serie
  // Inicializar el LCD
  lcd.init();

  //Encender la luz de fondo.
  lcd.backlight();
  lcd.print("Llavero ETEC-UBA");

  // Init RFID
  SPI.begin();  // Inicia la comunicación SPI
  //preparo la clave para acceder a los TAGs (puede hacerse al momento de leer)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  // Clave predeterminada
  }
  rfid.PCD_Init();                 // Inicializa el lector RFID
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
  Serial.print("Intentando conectar a wifi...");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print(" ya se conecto a WiFi. IP: ");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_BROKER, PUERTO_MQTT);
  // ACA LE DIGO QUIEN ERA EL CALLBACK
  client.setCallback(callback);

  // Conectarse al broker MQTT
  while (!client.connected()) {
    Serial.print("conectando a MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("conectado");
      client.subscribe(MQTT_TOPIC_ESP_SUSCRIBE);                       // me suscribo al topic
      client.publish(MQTT_LOG_TOPIC, "ESP32 conectado");  // publico mensaje avisando que me conecté
    } else {
      Serial.print("Fallo en el estado");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void loop() {
  //readRFID();  // Llama a la función readRFID en el bucle principal
  revisarConectividadWiFiYMQTT();  //si se desconectó, vuelve a conectar
  client.loop();


  if (aula != "") {
    // si la variable "aula" es diferente de null  me muestra lo que guarde en la variable que
    lcd.setCursor(0, 1);
    lcd.print("Recibi: ");  // que en este caso seria el numero del aula
    lcd.print(aula);

    if (encontroAula(aula)) {
      lcd.setCursor(0, 1);
      lcd.print("Llave servida");
      lcd.print("    ");
      while (hayTagRFID()); //ToDo: no estaría funcionando (no espera a que retire la llave)

      //si llego acá es porque ya no hay llave
      //aca falta que cuando saco la llave dicha llave de se muestre en el topic: 
      client.publish(MQTT_TOPIC_ESP_PUBLICA, aula.c_str());


    } else {
      lcd.setCursor(0, 1);
      lcd.print("Llave faltante");
      lcd.print("   ");
    }
    aula = "";  // lo que hago aca es que una vez que me muestra lo que le pedi que seria el numero del aula
    //hace que la varible vuelva a estar vacia para que puedan entrar otras aulas
  }
}
void revisarConectividadWiFiYMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Se desconectó de WiFi");
  } else {
    //Serial.println("Se desconectó de MQTT");
    while (!client.connected()) {
      Serial.print("conectando a MQTT...");
      if (client.connect("ESP32Client")) {
        Serial.println("conectado");
        client.subscribe(MQTT_TOPIC_ESP_SUSCRIBE);                       // me suscribo al topic
        client.publish(MQTT_LOG_TOPIC, "ESP32 conectado");  // publico mensaje avisando que me conecté
      } else {
        Serial.print("Fallo en el estado");
        Serial.print(client.state());
        delay(2000);
      }
    }
  }
}



bool encontroAula(String aula) {
  Serial.print("Buscando: ");
  Serial.println(aula);

  // Iniciar el motor para girar
  girarMotor(MOTOR_VELOCIDAD_MAXIMA);  // Hago girar el motor
  int contadorDeVueltas = 0;

  // Repetir las próximas instrucciones MIENTRAS NO encuentre la tarjeta Y NO haya dado una vuelta completa
  while (1 == 1) {  // Bucle infinito, se detiene solo si se encuentra la llave o la condición de timeout
    while (!hayTagRFID())
      ;  // Esperar a que se detecte un RFID

    Serial.print("Hay una tarjeta, con datos: ");
    byte datosLeidosDelTag[RFID_BUFFER_SIZE];

    // Leo el tag RFID
    ReadDataFromBlock(RFID_NUMERO_DE_BLOQUE, datosLeidosDelTag);

    // Cierro la comunicación con el tag para poder leer otro
    tagRFIDCerrarComunicacion();

    // Convertir los datos leídos del tag a String
    String datosLeidosDelTagString = String((char*)datosLeidosDelTag);
    String datosRecortados = datosLeidosDelTagString.substring(0, 8);  // Recorto los datos leídos (mejorar esta parte)

    Serial.println(datosRecortados);

    if (datosRecortados.equals("inicio  ")) {
      contadorDeVueltas++;  // Incremento el contador
      if (contadorDeVueltas == 2) {
        Serial.println("TAG de inicio detectada dos veces. Deteniendo motor.");
        delay(1200);     // para que se pase del tag de inicio y llegue a un hueco (asi no retiran el tag de inicio)
        detenerMotor();  // Detengo el motor

        return false;  // Regreso verdadero indicando que se encontró la llave dos veces
      }
    }
    // Si la aula leída del tag coincide con el aula que busco, incremento el contador
    else if (aula.equals(datosRecortados)) {  //si es el aula del tag es la que estoy buscando  (comparo dos variables Strings que serian aula y datosLeidosDelTag)
      delay(150);                             //ToDo: ajustar tiempo desde que detecta llave hasta que detiene motor. Mejorar método (que no dependa de un tiempo fijo)
      detenerMotor();
      return true;
    }
  }
  // Si no se encontró la llave, detengo el motor
  detenerMotor();
  return false;
}

//inicioCodigoMotor
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

void detenerMotor() {
  digitalWrite(MOTOR_NARANJA, LOW);
  digitalWrite(MOTOR_AZUL, LOW);
}
//FinCodigoMotor

bool hayTagRFID() {
  // Comprobar si hay una nueva tarjeta presente
  if (!rfid.PICC_IsNewCardPresent()) {
    return false;
  }

  // Leer la tarjeta seleccionada
  if (!rfid.PICC_ReadCardSerial()) {
    return false;
  }
  return true;
}

void readRFID() {
  // Read RFID card
  if (!hayTagRFID())
    return;

  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];  // Almacena el UID leído en el array nuidPICC
  }

  Serial.print(F("RFID in dec: "));           // Imprime el mensaje "RFID in dec: "
  printDec(rfid.uid.uidByte, rfid.uid.size);  // Llama a la función printDec para imprimir el UID en decimal
  Serial.println();                           // Imprime una nueva línea en el monitor serie

  tagRFIDCerrarComunicacion();
}

void tagRFIDCerrarComunicacion() {
  // Halt PICC
  rfid.PICC_HaltA();  // Detiene la comunicación con la tarjeta RFID
  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();  // Detiene la encriptación en el lector
}

/**
  Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte* buffer, byte bufferSize) {
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
void printDec(byte* buffer, byte bufferSize) {
  if (buffer != nullptr) {  // Comprueba si el puntero no es nulo
    for (byte i = 0; i < bufferSize; i++) {
      Serial.print(buffer[i] < 0x10 ? " 0" : " ");  // Formatea la salida
      Serial.print(buffer[i], DEC);                 // Imprime el byte en formato decimal
    }
  }
}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  byte bufferLen = RFID_BUFFER_SIZE;
  // Autenticación del bloque para lectura
  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Fallo en la autenticación para lectura: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  } else {
    Serial.println("Autenticación exitosa para lectura");
  }

  // Leer datos del bloque
  status = rfid.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Fallo al leer: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  } else {
    Serial.println("Bloque leído exitosamente");
  }
}
