//Librerias y definición de variables
#include "esp_camera.h" // https://github.com/espressif/esp32-camera/blob/e689c3b082985ee7b90198be32d330ce51ac5367/driver/include/esp_camera.h
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h" // Se puede obtener en Archivo > ESP32 > Camera > CameraWebServer o en
                         // https://github.com/espressif/arduino-esp32/blob/72c41d09538663ebef80d29eb986cd5bc3395c2d/libraries/ESP32/examples/Camera/CameraWebServer/camera_pins.h
#define USE_DMA
#include <TJpg_Decoder.h> //https://github.com/Bodmer/TJpg_Decoder

#ifdef USE_DMA // permite el uso del acceso directo a la memoria de la placa (Direct Memory Access)
  uint16_t  dmaBuffer1[16*16]; // bloque de conmutación
  uint16_t  dmaBuffer2[16*16];
  uint16_t* dmaBufferPtr = dmaBuffer1;
  bool dmaBufferSel = 0;
#endif

// Include the TFT library 
#include "SPI.h"
#include <TFT_eSPI.h> // https://github.com/Bodmer/TFT_eSPI
TFT_eSPI tft = TFT_eSPI(); // cambio de nombre para su uso

// Función llamada durante la decodificación de los archivos
// jpeg para renderizar las imagenes de 16x16 u 8x8 para la pantalla TFT
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  if ( y >= tft.height() ) return 0; // Si la imagen se sale de la pantalla, se para la decodificación

#ifdef USE_DMA
  
  if (dmaBufferSel) dmaBufferPtr = dmaBuffer2;
  else dmaBufferPtr = dmaBuffer1;
  dmaBufferSel = !dmaBufferSel;
  tft.pushImageDMA(x, y, w, h, bitmap, dmaBufferPtr); // El mapa de bits se copia en el buffer para actualizarlo con e ldecodificacor de jpeg
#else
  tft.pushImage(x, y, w, h, bitmap); // recorta la imagen dentro de los límites de la pantalla
#endif
  return 1; // devuelve 1 para decodificar el siguiente bloque
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10; // calidad de la imagen, cuanto menor sea el número, mayor será la calidad
    config.fb_count = 2;

  // Inicialización de la cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Inicialización de cámara fallido con error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_contrast(s, 2); // Ajuste del contraste de la imagen (-2, 2)
  s->set_saturation(s, -2); // Ajuste de la saturación de la imagen (-2, 2)

  // Inicialización de pantalla TFT
  tft.begin();
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setRotation(3);

#ifdef USE_DMA
  tft.initDMA(); // Inicialización del DMA
#endif

  // Escalado de la imagen para hacerla más pequeña (factores posibles: 1, 2, 4 u 8)
  TJpgDec.setJpgScale(1);

  tft.setSwapBytes(true); // Para invertir los colores de la imagen o no en el decodificador

  // El decodificador necesita conocer el nombre de la función de renderizado
  TJpgDec.setCallback(tft_output);
}

// Función que se repite en bucle para que se pueda actualizar la imagen en tiempo real
void loop() {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  
  fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Captura de cámara fallida");
    esp_camera_fb_return(fb);
    return;
  }

  size_t fb_len = 0;
  if(fb->format != PIXFORMAT_JPEG){
    Serial.println("Datos Non-JPEG no implementada");
    return;
  }

  #ifdef USE_DMA
  // Inicialización del pin CS de la pantalla en nivel bajo mientras se configuran los ajustes entre DMA y SPI
  tft.startWrite();
  #endif

  TJpgDec.drawJpg(0, 0,  fb->buf, fb->len); // Impresión de la imagen con el punto inicial el (0,0)

  #ifdef USE_DMA
  tft.endWrite();
  #endif
  
  esp_camera_fb_return(fb);
}
