#include <Arduino.h>
#include "esp_camera.h"

// -------------------------------------------------------------
// PINES PROTOBOARD (LEDs y BUZZER)
// -------------------------------------------------------------
const int LED_VERDE = 14;
const int LED_ROJO  = 15;
const int BUZZER    = 13;

// -------------------------------------------------------------
// PINES FREENOVE ESP32-WROVER-CAM
// -------------------------------------------------------------
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    21
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      19
#define Y4_GPIO_NUM      18
#define Y3_GPIO_NUM       5
#define Y2_GPIO_NUM       4
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

void capturarYSemparar();
void ejecutarAprobado();
void ejecutarAlertaDanado();
void apagarIndicadores();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configurar pines de actuadores e indicadores
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  apagarIndicadores();

  // Configuración de hardware de la cámara
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA; // 640x480
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    return;
  }
}

void loop() {
  capturarYSemparar();
  delay(35000); // Frecuencia de muestreo (3.5 segundos)
}

void capturarYSemparar() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;

  // 1. Limpiar datos residuales del buffer Serial RX antes de enviar
  while (Serial.available() > 0) {
    Serial.read();
  }

  // 2. Transmitir señal de inicio y tamaño de imagen a Python
  Serial.printf("START:%d\n", fb->len);
  Serial.write(fb->buf, fb->len);
  Serial.println();

  esp_camera_fb_return(fb);

  // 3. Esperar la respuesta de veredicto retornada por Python (Timeout de 12s)
  unsigned long startTime = millis();
  String respuesta = "";
  
  while (millis() - startTime < 12000) {
    if (Serial.available() > 0) {
      respuesta = Serial.readStringUntil('\n');
      respuesta.trim();
      respuesta.toUpperCase();
      break;
    }
  }

  // 4. Evaluar el veredicto y activar actuadores correspondientes
  if (respuesta.indexOf("DANADO") >= 0 || respuesta.indexOf("DAÑADO") >= 0) {
    ejecutarAlertaDanado();
  } 
  else if (respuesta.indexOf("PASA") >= 0) {
    ejecutarAprobado();
  } 
  else {
    apagarIndicadores(); // Caso 'VACIO' o fallo de lectura
  }
}

void ejecutarAprobado() {
  apagarIndicadores();
  digitalWrite(LED_VERDE, HIGH);
  delay(2000);
  digitalWrite(LED_VERDE, LOW);
}

void ejecutarAlertaDanado() {
  apagarIndicadores();
  digitalWrite(LED_ROJO, HIGH);
  digitalWrite(BUZZER, HIGH);
  delay(2000);
  apagarIndicadores();
}

void apagarIndicadores() {
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(BUZZER, LOW);
}