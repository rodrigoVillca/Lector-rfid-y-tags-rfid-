#include <WiFi.h>             // Biblioteca para conectar el ESP32 a una red WiFi
#include <PubSubClient.h>      // Biblioteca para implementar el protocolo MQTT, utilizado para la comunicación en IoT.
#include <Wire.h>              // Biblioteca para comunicación I2C, generalmente usada con dispositivos como sensores y pantallas en este caso el displayLCD
#include <LiquidCrystal_I2C.h> // Biblioteca para controlar pantallas LCD con interfaz I2C

#include <SPI.h>       // Biblioteca SPI para la comunicación en serie
#include <MFRC522.h>   // Biblioteca MFRC522 para leer etiquetas RFID

// Definir pines de comunicación para el lector RFID
#define SS_PIN 5   // Pin de selección de esclavo (SS) como el pin 5
#define RST_PIN 0  // Pin de reinicio (RST) como el pin 0

// Crear el objeto LCD con la dirección 0x27 y 16 columnas x 2 filas
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Configuración de la pantalla LCD

// Variables globales
byte nuidPICC[4] = { 0, 0, 0, 0 };  // Array para almacenar el UID de la tarjeta RFID
MFRC522::MIFARE_Key key;             // Variable para la clave MIFARE
MFRC522 rfid(SS_PIN, RST_PIN);       // Inicializa el lector RFID

MFRC522::StatusCode status;

// Definir configuraciones
#define NUMERO_DE_BLOQUE_RFID 1    // Número de bloque donde se guardan los datos en el tag RFID
#define TAMANO_BUFFER_RFID 18      // Tamaño del buffer de lectura (16 bytes + 2 bytes extra)

// Configuración de la red WiFi y MQTT
const char* SSID_WIFI = "ETEC-UBA";           // SSID de la red WiFi
const char* CLAVE_WIFI = "ETEC-alumnos@UBA";  // Contraseña de WiFi
const char* BROKER_MQTT = "10.9.120.171";    // Dirección del broker MQTT
const int PUERTO_MQTT = 1883;                // Puerto del broker MQTT
const char* TOPICO_SUSCRIBIR = "topic-prueba";  // Topic para suscripción
const char* TOPICO_LOG = "logs";               // Topic para logs
const char* TOPICO_PUBLICAR = "aula-retirada"; // Topic para publicar información de retiro de llave

// Definir pines para los motores
const int MOTOR_VERDE = 33;
const int MOTOR_AZUL = 25;
const int MOTOR_NARANJA = 26;
const int VELOCIDAD_MAXIMA_MOTOR = 255;   // Velocidad máxima para los motores

// Variables para gestionar el aula y la conexión MQTT
String aula = "";  // Variable para almacenar el número de aula
WiFiClient clienteWiFi;
PubSubClient clienteMQTT(clienteWiFi);

// Callback para manejar los mensajes recibidos por MQTT
void callback(char* topic, byte* message, unsigned int length) {
  String mensaje;
  
  // Recorrer el mensaje recibido y almacenarlo
  for (int i = 0; i < length; i++) {
    mensaje += (char)message[i];
  }
  
  aula = mensaje;  // Guardamos el mensaje en la variable aula
}

// Función de configuración inicial
void setup() {
  // Inicialización del puerto serie
  Serial.begin(115200);
  Serial.println(F("Llavero ETEC-UBA"));
  
  // Inicializar LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Llavero ETEC-UBA");

  // Inicialización del lector RFID
  SPI.begin();
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  // Establecer clave predeterminada
  }
  rfid.PCD_Init();  // Inicializar el lector RFID
  Serial.print(F("Reader: "));
  rfid.PCD_DumpVersionToSerial();

  // Configuración de los pines de los motores
  pinMode(MOTOR_VERDE, OUTPUT);
  pinMode(MOTOR_AZUL, OUTPUT);
  pinMode(MOTOR_NARANJA, OUTPUT);

  // Conexión WiFi
  Serial.print("Conectando a WiFi...");
  WiFi.begin(SSID_WIFI, CLAVE_WIFI);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print(" Conectado a WiFi. IP: ");
  Serial.println(WiFi.localIP());

  // Configuración del cliente MQTT
  clienteMQTT.setServer(BROKER_MQTT, PUERTO_MQTT);
  clienteMQTT.setCallback(callback);

  // Conexión al broker MQTT
  while (!clienteMQTT.connected()) {
    Serial.print("Conectando a MQTT...");
    if (clienteMQTT.connect("ESP32Client")) {
      Serial.println(" Conectado");
      clienteMQTT.subscribe(TOPICO_SUSCRIBIR);  // Suscribirse al topic
      clienteMQTT.publish(TOPICO_LOG, "ESP32 conectado");  // Publicar log de conexión
    } else {
      Serial.print("Error: ");
      Serial.print(clienteMQTT.state());
      delay(2000);
    }
  }
}

// Función principal
void loop() {
  comprobarConexiones();  // Verifica la conectividad WiFi y MQTT
  clienteMQTT.loop();  // Mantener la conexión MQTT activa

  if (aula != "") {  // Si se recibe un número de aula por MQTT
    lcd.setCursor(0, 1);
    lcd.print("Recibi: ");
    lcd.print(aula);

    if (encontrarAula(aula)) {  // Si se encuentra el aula en el RFID
      lcd.setCursor(0, 1);
      lcd.print("Llave servida");
      delay(200);

      while (rfid.PICC_IsNewCardPresent()) {  // Espera a que el usuario retire la llave
        esperarRetiroDeLlave();
      }

      // Publica el mensaje de retiro de la llave en el topic MQTT
      clienteMQTT.publish(TOPICO_PUBLICAR, aula.c_str());
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Llave faltante");
      delay(200);
    }
    aula = "";  // Limpiar la variable de aula después de mostrar el mensaje
  }
}

// Verificar la conectividad WiFi y MQTT
void comprobarConexiones() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Desconectado de WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    Serial.println("Conectado a WiFi");
  }

  while (!clienteMQTT.connected()) {
    Serial.print("Conectando a MQTT...");
    if (clienteMQTT.connect("ESP32Client")) {
      Serial.println(" Conectado");
      clienteMQTT.subscribe(TOPICO_SUSCRIBIR);  // Suscribirse al topic
      clienteMQTT.publish(TOPICO_LOG, "ESP32 reconectado");
    } else {
      Serial.print("Error: ");
      Serial.print(clienteMQTT.state());
      delay(2000);
    }
  }
}

// Función para encontrar el aula en el RFID
bool encontrarAula(String aula) {
  Serial.print("Buscando: ");
  Serial.println(aula);

  girarMotor(VELOCIDAD_MAXIMA_MOTOR);  // Girar el motor para buscar el aula
  int contadorVueltas = 0;

  while (true) {
    while (!hayTagRFID());  // Esperar hasta que se detecte un RFID

    byte datosLeidos[RFID_BUFFER_SIZE];
    leerDatosDeBloque(NUMERO_DE_BLOQUE_RFID, datosLeidos);  // Leer datos del RFID
    cerrarComunicacionRFID();

    String datosLeidosString = String((char*)datosLeidos);
    String datosRecortados = datosLeidosString.substring(0, 8);

    if (datosRecortados.equals("inicio  ")) {
      contadorVueltas++;
      if (contadorVueltas == 2) {
        detenerMotor();
        return false;
      }
    } else if (aula.equals(datosRecortados)) {
      detenerMotor();
      return true;
    }
  }
  detenerMotor();
  return false;
}

// Funciones de motor
void girarMotor(int velocidad) {
  analogWrite(MOTOR_VERDE, velocidad);
  digitalWrite(MOTOR_NARANJA, LOW);
  digitalWrite(MOTOR_AZUL, HIGH);
}

void detenerMotor() {
  digitalWrite(MOTOR_NARANJA, LOW);
  digitalWrite(MOTOR_AZUL, LOW);
}

// Leer el RFID
bool hayTagRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return false;
  return rfid.PICC_ReadCardSerial();
}

void leerDatosDeBloque(int bloque, byte datos[]) {
  byte bufferLen = RFID_BUFFER_SIZE;
  status = rfid.PCD_Authenticate(MFRC522::PICC_AUTHENT1A, bloque, &key, &nuidPICC[0]);

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Error de autenticación: "));
    Serial.println(rfid.GetStatusCodeName(status));
    return;
  }

  status = rfid.MIFARE_Read(bloque, datos, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Error al leer el bloque: "));
    Serial.println(rfid.GetStatusCodeName(status));
  }
}

void esperarRetiroDeLlave() {
  if (rfid.PICC_IsNewCardPresent()) {
    rfid.PICC_ReadCardSerial();
    Serial.println("Llave retirada");
  }
}