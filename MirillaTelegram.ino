// Librerias
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "esp_http_server.h"

//Modelo de cámara
#define PART_BOUNDARY "123456789000000000000987654321"
#define CAMERA_MODEL_AI_THINKER

//WiFi
const char* ssid = "MOVISTAR_5637"; //Nombre de la red WiFi
const char* password = "hieNwj9vNcxanaHTRAandreajPpvh"; //Contraseña de la red WiFi

//Telegram
String BOTtoken = "6116326742:AAGKbodVOG5RZtaTwVF1HK6Yv-GgUDP0wTg";  //Token del bot obtenido con BotFather
String CHAT_ID = "908196854"; // ID del chat con el bot obtenido con myidbot

// Inicialización del cliente TCP
WiFiClientSecure clientTCP;
//Inicialización del bot
UniversalTelegramBot bot(BOTtoken, clientTCP);

// Definición de los pines de la cámara

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Definición de los estados del LED
#define ON HIGH
#define OFF LOW

#define FLASH_LED_PIN   4 // Pin del LED
#define PIR_SENSOR_PIN  12 // Pin del sensor

#define EEPROM_SIZE     2 //Tamaño de la memoria interna del microcontrolador que se quiere usar

//Comprobación cada segundo de si hay un mensaje nuevo
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

//Estabilización del sensor PIR
int countdown_interval_to_stabilize_PIR_Sensor = 1000;
unsigned long lastTime_countdown_Ran;
byte countdown_to_stabilize_PIR_Sensor = 30;

bool sendPhoto = false; //Verificación de si se ha enviado la imagen

bool PIR_Sensor_is_stable = false; //Verificación de si se ha estabilizado el sensor

bool boolPIRState = false; //Estado del sensor PIR

// Estado del envío de las fotos
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// Envío de una confirmación del éxito/fracaso del envío de la imagen
void FB_MSG_is_photo_send_successfully (bool state) {
  String send_feedback_message = "";
  if(state == false) {
    send_feedback_message += "ESP32-CAM:\n\n";
    send_feedback_message += "Error enviando la imagen.\n";
    send_feedback_message += "Sugerencia:\n";
    send_feedback_message += "- Vuelve a probar, por favor.\n";
    send_feedback_message += "- Resetea ESP32-CAM.\n";
    send_feedback_message += "- Cambia el tamaño de visualización (FRAMESIZE). \n";
    Serial.print(send_feedback_message);
    send_feedback_message += "\n\n";
    send_feedback_message += "Utiliza el comando /start para ver todos los comandos.";
    bot.sendMessage(CHAT_ID, send_feedback_message, "");
  } else {
    if(boolPIRState == true) {
      Serial.println("Imagen enviada con éxito.");
      send_feedback_message += "ESP32-CAM:\n\n";
      send_feedback_message += "El sensor detecta objetos y movimientos.\n";
      send_feedback_message += "Imagen enviada con éxito.\n\n";
      send_feedback_message += "Utiliza el comando /start para ver todos los comandos.";
      bot.sendMessage(CHAT_ID, send_feedback_message, ""); 
    }
    if(sendPhoto == true) {
      Serial.println("Imagen enviada con éxito.");
      send_feedback_message += "ESP32-CAM:\n\n";
      send_feedback_message += "Imagen enviada con éxito.\n\n";
      send_feedback_message += "Utiliza el comando /start para ver todos los comandos.";
      bot.sendMessage(CHAT_ID, send_feedback_message, ""); 
    }
  }
}

// Lectura del sensor PIR (HIGH/1 LOW/0) */
bool PIR_State() {
  bool PRS = digitalRead(PIR_SENSOR_PIN);
  return PRS;
}

// Apagado/encendido del LED
void LEDFlash_State(bool ledState) {
  digitalWrite(FLASH_LED_PIN, ledState);
}

// Ajuste y guardado de ajustes para capturar fotos con flash
void enable_capture_Photo_With_Flash(bool state) {
  EEPROM.write(0, state);
  EEPROM.commit();
  delay(50);
}

// Lectura de los ajustes para capturar imágenes con flash
bool capture_Photo_With_Flash_state() {
  bool capture_Photo_With_Flash = EEPROM.read(0);
  return capture_Photo_With_Flash;
}

// Ajuste y guardado de ajustes para capturar fotos con sensor PIR
void enable_capture_Photo_with_PIR(bool state) {
  EEPROM.write(1, state);
  EEPROM.commit();
  delay(50);
}

// Lectura de los ajustes para capturar fotos con sensor PIR
bool capture_Photo_with_PIR_state() {
  bool capture_Photo_with_PIR = EEPROM.read(1);
  return capture_Photo_with_PIR;
}

// Confiuración de la cámara
void configInitCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Configuración del tamaño y la calidad de la imagen
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 30; // entre 10-80 cuanto mayor numero menor calidad
  config.fb_count = 1;

// inicialización de la cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error inicializando la camára. Error nr: 0x%x", err);
    Serial.println();
    Serial.println("Reiniciar ESP32 Cam");
    delay(1000);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SXGA);
}

// Reaccion ante un nuevo mensaje
void handleNewMessages(int numNewMessages) {
  Serial.print("Nuevo mensaje: ");
  Serial.println(numNewMessages);

  // Comprobación del contenido del nuevo mensaje
  for (int i = 0; i < numNewMessages; i++) {
    
    //Comprobación del ID de la conversación
    // en caso de no coincidir con el definido, avisa
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Usuario no autorizado", "");
      Serial.println("Usuario no autorizado");
      Serial.println("------------");
      continue;
    }
    // Impresión del mensaje recibido
    String text = bot.messages[i].text;
    Serial.println(text);
    
    // Comprobación de las condiciones a partir de los comandos recibidos por telegram
    String send_feedback_message = "";
    String from_name = bot.messages[i].from_name;
    if (text == "/start") {
      send_feedback_message += "ESP32-CAM:\n\n";
      send_feedback_message += "Bienvenido, " + from_name + "\n";
      send_feedback_message += "Utiliza estos comandos para interactuar con ESP32-CAM.\n\n";
      send_feedback_message += "/capture_photo : Hacer una foto nueva\n\n";
      send_feedback_message += "Ajustes:\n";
      send_feedback_message += "/enable_capture_Photo_With_Flash: Hacer foto con flash LED\n";
      send_feedback_message += "/disable_capture_Photo_With_Flash: Hacer foto sin flash LED\n";
      send_feedback_message += "/enable_capture_Photo_with_PIR: Hacer foto con sensor PIR\n";
      send_feedback_message += "/disable_capture_Photo_with_PIR: Hacer foto sin sensor PIR\n\n";
      send_feedback_message += "Estado de los ajustes:\n";
      if(capture_Photo_With_Flash_state() == ON) {
        send_feedback_message += "- Hacer foto con flash = ON\n";
      }
      if(capture_Photo_With_Flash_state() == OFF) {
        send_feedback_message += "- Hacer foto con flash = OFF\n";
      }
      if(capture_Photo_with_PIR_state() == ON) {
        send_feedback_message += "- Hacer foto con sensor = ON\n";
      }
      if(capture_Photo_with_PIR_state() == OFF) {
        send_feedback_message += "- Hacer foto con sensor = OFF\n";
      }
      if(PIR_Sensor_is_stable == false) {
        send_feedback_message += "\n Estado del sensor PIR:\n";
        send_feedback_message += "El sensor PIR está estabilizandose.\n";
        send_feedback_message += "Tiempo de estabilización " + String(countdown_to_stabilize_PIR_Sensor) + " segundos restantes. Por favor, espere.\n";
      }
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
      Serial.println("------------");
    }
    
    // Condición para el comando "/capture_photo".
    if (text == "/capture_photo") {
      sendPhoto = true;
      Serial.println("Petición de nueva imagen");
    }
    
    // Condición para el comando "/enable_capture_Photo_With_Flash".
    if (text == "/enable_capture_Photo_With_Flash") {
      enable_capture_Photo_With_Flash(ON);
      send_feedback_message += "ESP32-CAM:\n\n";
      if(capture_Photo_With_Flash_state() == ON) {
        Serial.println("Hacer foto con flash = ON");
        send_feedback_message += "Hacer foto con flash = ON\n\n";
      } else {
        Serial.println("Error activando el flash. Pruebe de nuevo.");
        send_feedback_message += "Error activando el flash. Pruebe de nuevo.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start: Para ver todos los comandos.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }
    
    // Condición para el comando "/disable_capture_Photo_With_Flash".
    if (text == "/disable_capture_Photo_With_Flash") {
      enable_capture_Photo_With_Flash(OFF);
      send_feedback_message += "ESP32-CAM:\n\n";
      if(capture_Photo_With_Flash_state() == OFF) {
        Serial.println("Hacer foto con flash = OFF");
        send_feedback_message += "Hacer foto con flash = OFF\n\n";
      } else {
        Serial.println("Error desactivando el flash. Pruebe de nuevo.");
        send_feedback_message += "Error desactivando el flash. Pruebe de nuevo.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start: Para ver todos los comandos.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }

    // Condición para el comando "/enable_capture_Photo_with_PIR".
    if (text == "/enable_capture_Photo_with_PIR") {
      enable_capture_Photo_with_PIR(ON);
      send_feedback_message += "ESP32-CAM :\n\n";
      if(capture_Photo_with_PIR_state() == ON) {
        Serial.println("Hacer foto con sensor = ON");
        send_feedback_message += "Hacer foto con sensor = ON\n\n";
        botRequestDelay = 20000;
      } else {
        Serial.println("Error activando el sensor PIR. Pruebe de nuevo.");
        send_feedback_message += "Error activando el sensor PIR. Pruebe de nuevo.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start: Para ver todos los comandos.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }

    // Condición para el comando "/disable_capture_Photo_with_PIR".
    if (text == "/disable_capture_Photo_with_PIR") {
      enable_capture_Photo_with_PIR(OFF);
      send_feedback_message += "ESP32-CAM :\n\n";
      if(capture_Photo_with_PIR_state() == OFF) {
        Serial.println("Hacer foto con sensor = OFF");
        send_feedback_message += "Hacer foto con sensor = OFF\n\n";
        botRequestDelay = 1000;
      } else {
        Serial.println("Error desactivando el sensor PIR. Pruebe de nuevo.");
        send_feedback_message += "Error desactivando el sensor PIR. Pruebe de nuevo.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start: Para ver todos los comandos.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }
  }
}

// Capturación y envío de imagenes
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  // Captura de imagenes
  Serial.println("Haciendo foto...");

  // Activación del flash si es necesario
  if(capture_Photo_With_Flash_state() == ON) {
    LEDFlash_State(ON);
  }
  delay(1000);
  
  // Captura de la imagen 
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Error haciendo la foto");
    Serial.println("Reiniciar ESP32 Cam");
    delay(1000);
    ESP.restart();
    return "Error haciendo la foto";
  }  
  
  // Apagado del flash tras la captra de la foto
  if(capture_Photo_With_Flash_state() == ON) {
    LEDFlash_State(OFF);
  }

  Serial.println("Éxito haciendo la foto.");
  
  //Envío de la imagen. */
  Serial.println("Conectado a " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Conexión exitosa");
    Serial.print("Envio de fotos");
    
    String head = "--Esp32Cam\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n";
    head += CHAT_ID; 
    head += "\r\n--Esp32Cam\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Esp32Cam--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=Esp32Cam");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);

    // Comprobación de si la foto se ha enviado con éxito o no.
    if(getBody.length() > 0) {
      String send_status = "";
      send_status = getValue(getBody, ',', 0);
      send_status = send_status.substring(6);
      
      if(send_status == "true") {
        FB_MSG_is_photo_send_successfully(true);
      }
      if(send_status == "false") {
        FB_MSG_is_photo_send_successfully(false);
      }
    }
    if(getBody.length() == 0) FB_MSG_is_photo_send_successfully(false);

  }
  else {
    getBody="Conexión fallida con api.telegram.org.";
    Serial.println("Conexión fallida con api.telegram.org.");
  }
  Serial.println("------------");
  delay(10000);
  return getBody;
}

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Error haciendo la foto");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("Compresión de JPEG fallida");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
  }
  return res;
}

void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
  }
}


void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

   //Inicialización de la velocidad de comuniación serial
  Serial.begin(115200);
  delay(1000);
  
  Serial.setDebugOutput(false);
  
  Serial.println();
  Serial.println();
  Serial.println("------------");

  // Inicialización de la EEPROM, escritura y lectura de los ajustes guardados en la memoria
  EEPROM.begin(EEPROM_SIZE);

  enable_capture_Photo_With_Flash(OFF);
  enable_capture_Photo_with_PIR(OFF);
  delay(500);

  Serial.println("Estableciendo estado:");
  if(capture_Photo_With_Flash_state() == ON) {
    Serial.println("- Hacer foto con flash = ON");
  }
  if(capture_Photo_With_Flash_state() == OFF) {
    Serial.println("- Hacer foto con flash = OFF");
  }
  if(capture_Photo_with_PIR_state() == ON) {
    Serial.println("- Hacer foto con sensor = ON");
    botRequestDelay = 20000;
  }
  if(capture_Photo_with_PIR_state() == OFF) {
    Serial.println("- Hacer foto con sensor = OFF");
    botRequestDelay = 1000;
  }

  // Inicializar el estado del flash en OFF
  pinMode(FLASH_LED_PIN, OUTPUT);
  LEDFlash_State(OFF);

  //Configuración e inicialización de la cámara
  Serial.println();
  Serial.println("Inicio de la configuración e inicialización de la camara...");
  configInitCamera();
  Serial.println("Configuración e inicialización de la cámara correctas.");
  Serial.println();

  // Conexión a la red WiFi
  WiFi.mode(WIFI_STA);
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  //Conexión de la ESP32-Cam con la red WiFi
  int connecting_process_timed_out = 20;
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    LEDFlash_State(ON);
    delay(250);
    LEDFlash_State(OFF);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }

  startCameraServer();
  
  LEDFlash_State(OFF);
  Serial.println();
  Serial.print("Conectado a  ");
  Serial.println(ssid);
  Serial.print("ESP32-CAM IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("El sensor PIR está siendo estabilizado.");
  Serial.printf("Tiempo de estabilización restante %d segundos. Por favor, espere.\n", countdown_to_stabilize_PIR_Sensor);
  
  Serial.println("------------");
  Serial.println();
}

void loop() {
  // Condiciones para hacer y enviar las fotos
  if(sendPhoto) {
    Serial.println("Preparando la imagen...");
    sendPhotoTelegram(); 
    sendPhoto = false; 
  }

  // Comprobación de si hay mensajes nuevos
  if(millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println();
      Serial.println("------------");
      Serial.println("Respuesta obtenida");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  // Estabilización del sensor PIR
 
  if(PIR_Sensor_is_stable == false) {
    if(millis() > lastTime_countdown_Ran + countdown_interval_to_stabilize_PIR_Sensor) {
      if(countdown_to_stabilize_PIR_Sensor > 0) countdown_to_stabilize_PIR_Sensor--;
      if(countdown_to_stabilize_PIR_Sensor == 0) {
        PIR_Sensor_is_stable = true;
        Serial.println();
        Serial.println("------------");
        Serial.println("El tiempo de estabilización del sensor ha finalizado.");
        Serial.println("El sensor ya puede funcionar.");
        Serial.println("------------");
        String send_Status_PIR_Sensor = "";
        send_Status_PIR_Sensor += "ESP32-CAM :\n\n";
        send_Status_PIR_Sensor += "El tiempo de estabilización del sensor ha finalizado.\n";
        send_Status_PIR_Sensor += "El sensor ya puede funcionar.";
        bot.sendMessage(CHAT_ID, send_Status_PIR_Sensor, "");
      }
      lastTime_countdown_Ran = millis();
    }
  }

  // Lectura del sensor y ejecución de sus comandos
  if(capture_Photo_with_PIR_state() == ON) {
    if(PIR_State() == true && PIR_Sensor_is_stable == true) {
      Serial.println("------------");
      Serial.println("El sensor PIR detecta movimientos y objetos.");
      boolPIRState = true;
      sendPhotoTelegram();
      boolPIRState = false;
    }
  }

}
