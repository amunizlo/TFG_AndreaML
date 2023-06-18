//Librerias
#include <Adafruit_GFX.h> // Librería de gráficos para la pantalla
#include <Adafruit_SSD1306.h> // Librería de funcionamiento de la pantalla
//#include <SPI.h> // Librería para la comunicación 
#include <Wire.h>

// Definir constantes
#define ANCHO_PANTALLA 128 // ancho pantalla OLED
#define ALTO_PANTALLA 64 // alto pantalla OLED
Adafruit_SSD1306 display(128, 32, &Wire, -1);
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

// Imagen mostrada por la pantalla de una huella
const unsigned char icon [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf8, 0x60, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x18, 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4e, 0x0f, 0xc1, 0x80, 0x00, 0x00, 
  0x00, 0x00, 0x67, 0x80, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x01, 0x30, 0x7c, 0xf1, 0x30, 0x00, 0x00, 
  0x00, 0x00, 0xc6, 0x00, 0x07, 0x8c, 0x00, 0x00, 0x00, 0x03, 0x11, 0x80, 0x0c, 0x62, 0x00, 0x00, 
  0x00, 0x04, 0x4c, 0x00, 0x3b, 0x19, 0x80, 0x00, 0x00, 0x09, 0xb3, 0x1f, 0xe6, 0x82, 0x40, 0x00, 
  0x00, 0x12, 0x44, 0x80, 0x0f, 0x69, 0x00, 0x00, 0x00, 0x24, 0x9a, 0x30, 0x63, 0x34, 0x80, 0x00, 
  0x00, 0x09, 0x24, 0xcf, 0x08, 0x94, 0x40, 0x00, 0x00, 0x0b, 0x49, 0xa0, 0x44, 0xda, 0x20, 0x00, 
  0x00, 0x12, 0x19, 0x65, 0x22, 0x0a, 0xa0, 0x00, 0x00, 0x12, 0x93, 0x48, 0x32, 0x4a, 0x90, 0x00, 
  0x00, 0x04, 0x92, 0x49, 0x32, 0x5a, 0x80, 0x00, 0x00, 0x04, 0xf2, 0x40, 0x52, 0xd2, 0x40, 0x00, 
  0x00, 0x04, 0x4e, 0x4f, 0x24, 0x96, 0x60, 0x00, 0x00, 0x04, 0xb4, 0x8f, 0x44, 0xa4, 0xa0, 0x00, 
  0x00, 0x00, 0xac, 0x90, 0x09, 0x64, 0x90, 0x00, 0x00, 0x01, 0x99, 0x31, 0x90, 0x88, 0x90, 0x00, 
  0x00, 0x01, 0xb2, 0x22, 0x28, 0x99, 0xa0, 0x00, 0x00, 0x00, 0x44, 0xcc, 0xc9, 0x91, 0x00, 0x00, 
  0x00, 0x00, 0x19, 0x11, 0x11, 0x2c, 0x40, 0x00, 0x00, 0x00, 0x06, 0x22, 0x32, 0x61, 0x00, 0x00, 
  0x00, 0x00, 0x08, 0x44, 0x44, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x98, 0x99, 0x18, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x13, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00
};

//Declaración del sensor de huellas y de su comunicación con la placa Arduino
SoftwareSerial mySerial(10, 11);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
int fingerprintID = 0;

//Relé
#define RELAY_PIN       4

#define ACCESS_DELAY    5000 

void setup()
{                
  Serial.begin(9600);
  delay(100);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 no encontrado"));
    for(;;);
  }
  display.clearDisplay();  // Borrado del contenico de la pantalla
  display.setTextColor(WHITE); // Definición a nivel alto del texto (color blanco)
  display.setRotation(0);  // Declaración de la orientación de la pantalla (0 = horizontal)
  display.setTextWrap(false);  // Alineación del texto (libre)

  display.dim(0);  //Ajuste del brillo de la pantalla (0 = máximo brillo)

  startFingerprintSensor();
  displayLockScreen(); 

  delay(5);

  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);

}

//Función bucle que comprueba si se está posicionando una huella válida
void loop()
{

fingerprintID = getFingerprintID();
  delay(50);
  if(fingerprintID == 1)
  {
    displayHuella1();
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000);
    displayLockScreen();
    digitalWrite(RELAY_PIN, LOW);
  }
   if(fingerprintID == 2)
  {
    displayHuella2();
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000);
    displayLockScreen();
    digitalWrite(RELAY_PIN, LOW);
  }
  if(fingerprintID == 3)
  {
    displayHuella3();
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000);
    displayLockScreen();
    digitalWrite(RELAY_PIN, LOW);
  }
  if(fingerprintID == 4)
  {
    displayHuella4();
    digitalWrite(RELAY_PIN, HIGH);
    delay(5000);
    displayLockScreen();
    digitalWrite(RELAY_PIN, LOW);
  }
}

// Declaración del mensaje según las huellas registradas
void displayHuella1()
{
  display.clearDisplay(); // Borrado de la pantalla
  display.setTextSize(0); 
  display.setCursor(48, 5); // Posición del cursor
  display.println("ABIERTO"); // Texto
  display.setCursor(48, 15); 
  display.println("Bienvenida!");
  display.setCursor(48, 25);
  display.println("ANDREA");
   display.display(); // Imprimir por pantalla el mensaje previo
}

void displayHuella2()
{
  display.clearDisplay(); // Borrado de la pantalla
  display.setTextSize(0); 
  display.setCursor(48, 5); // Posición del cursor
  display.println("ABIERTO"); // Texto
  display.setCursor(48, 15); 
  display.println("Bienvenido!");
  display.setCursor(48, 25);
  display.println("ALEX");
   display.display(); // Imprimir por pantalla el mensaje previo
}

void displayHuella3()
{
  display.clearDisplay(); // Borrado de la pantalla
  display.setTextSize(0); 
  display.setCursor(48, 5); // Posición del cursor
  display.println("ABIERTO"); // Texto
  display.setCursor(48, 15); 
  display.println("Bienvenido!");
  display.setCursor(48, 25);
  display.println("LUCAS");
   display.display(); // Imprimir por pantalla el mensaje previo
}

void displayHuella4()
{
  display.clearDisplay(); // Borrado de la pantalla
  display.setTextSize(0); 
  display.setCursor(48, 5); // Posición del cursor
  display.println("ABIERTO"); // Texto
  display.setCursor(48, 15);
  display.println("Bienvenida!");
  display.setCursor(48, 25);
  display.println("ARIEL");
   display.display(); // Imprimir por pantalla el mensaje previo
}

// Visualización del dibujo de huella en la pantalla
void displayLockScreen()
{
  display.clearDisplay();
  display.drawBitmap(32, 0, icon, 64, 32, 1);
  display.display();
}

// Inicialización del sensor 
void startFingerprintSensor()
{
  Serial.begin(9600);
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Sensor encontrado!");
  } else {
    Serial.println("Sensor no encontrado");
  }
  Serial.println("Esperando una huella válida...");
}

// Procesado de la imagen de la huella posicionada
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  
  // Huella coincidente
  Serial.print("Encontrado ID #"); Serial.print(finger.fingerID); 
  Serial.print(" con una coincidencia de "); Serial.println(finger.confidence);
  return finger.fingerID; 
}
