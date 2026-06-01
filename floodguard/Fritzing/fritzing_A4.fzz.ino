/*
 * ????????????????????????????????????????????????????????????????
 * ?         FLOOD·GUARD ? Dashboard TFT 320×240                 ?
 * ?         Sistema IoT de Alerta de Enchentes Urbanas           ?
 * ?         ODS 11 · Universidade Presbiteriana Mackenzie        ?
 * ????????????????????????????????????????????????????????????????
 * ?  Hardware:                                                   ?
 * ?    · ESP32-S3-N16R8 (DevKitC-1 ? 16MB Flash, 8MB PSRAM)    ?
 * ?    · Display TFT 2.8" ILI9341 320×240 (SPI)                 ?
 * ?    · Sensor Ultrassônico HC-SR04                             ?
 * ?    · Sensor de Chuva MH-RD (Raindrops Module)               ?
 * ?    · DHT11 (pino GPIO2 ? sinal "2G" da shield)              ?
 * ?    · Semáforo LED Open-Smart (R/Y/G)                        ?
 * ?    · Buzzer ativo 5V                                         ?
 * ?    · LED RGB WS2812 embutido (GPIO 48 ? DevKitC-1)          ?
 * ?    · Bateria 18650 + módulo carregador TP4056               ?
 * ????????????????????????????????????????????????????????????????
 * ?  LED RGB WS2812 ? Tabela de Cores:                          ?
 * ?    BRANCO   pulsante  ? Boot / Inicializando                ?
 * ?    CIANO    pulsante  ? Conectando ao WiFi                  ?
 * ?    CIANO    fixo      ? WiFi conectado (sem MQTT)           ?
 * ?    AZUL     fixo      ? WiFi OK + MQTT conectado            ?
 * ?    AZUL     pulsante  ? MQTT publicando dado                ?
 * ?    AMARELO  pulsante  ? WiFi desconectado (tentando)        ?
 * ?    MAGENTA  fixo      ? Sem WiFi ? modo offline             ?
 * ?    VERDE    fixo      ? Nível NORMAL                        ?
 * ?    AMARELO  fixo      ? Nível ALERTA                        ?
 * ?    VERMELHO pulsante  ? Nível CRÍTICO                       ?
 * ?  PRIORIDADE: Conectividade > Nível de alerta                ?
 * ????????????????????????????????????????????????????????????????
 * ?  Placa Arduino IDE: "ESP32S3 Dev Module"                    ?
 * ?  Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"       ?
 * ?  PSRAM: "OPI PSRAM"                                         ?
 * ????????????????????????????????????????????????????????????????
 * ?  Bibliotecas utilizadas:                                    ?
 * ?    · Adafruit GFX Library                                    ?
 * ?    · Adafruit ILI9341                                        ?
 * ?    · DHT sensor library (Adafruit)                           ?
 * ?    · Adafruit Unified Sensor                                 ?
 * ?    · Adafruit NeoPixel                  ? NOVO v2.1         ?
 * ?    · PubSubClient (MQTT) ? Nick O'Leary                     ?
 * ?    · ArduinoJson ? Benoit Blanchon                           ?
 * ????????????????????????????????????????????????????????????????
 * ?  Autores: João Ferreira Gomes Jr, João Victor Leiva Liceras  ?
 * ?           Raphael Campean da Silva                           ?
 * ?  Prof.: Andre Luis de Oliveira                               ?
 * ?  Versão: 2.1 · 2026                                         ?
 * ????????????????????????????????????????????????????????????????
 */


// ?????????????????????????????????????
//  INCLUDES
// ?????????????????????????????????????
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>   // LED RGB WS2812 embutido


// ?????????????????????????????????????
//  CONFIGURAÇÃO WiFi / MQTT
//  ? Altere com seus dados
// ?????????????????????????????????????
const char* WIFI_SSID     = "GOMES_HOME_2G";
const char* WIFI_PASSWORD = "Gomes_362J";


const char* MQTT_BROKER       = "broker.hivemq.com";
const int   MQTT_PORT         = 1883;
const char* MQTT_TOPIC_PUB    = "floodguard/upm/sensor1/dados";
const char* MQTT_TOPIC_CMD    = "floodguard/upm/sensor1/cmd";
const char* MQTT_TOPIC_STATUS = "floodguard/upm/sensor1/status";
const char* MQTT_CLIENT_ID    = "floodguard_N16R8_001";


// ?????????????????????????????????????????????????????????????????
//  PINAGEM ? ESP32-S3-N16R8 (DevKitC-1)
//
//  ? GPIOs PROIBIDOS no N16R8:
//     GPIO 19, 20  ? USB D- / D+
//     GPIO 26?32  ? Flash SPI interno
//     GPIO 33?37  ? OPI PSRAM (N16R8)
//     GPIO 45, 46 ? Strapping boot
//     GPIO 48     ? LED RGB WS2812 (uso dedicado abaixo)
// ?????????????????????????????????????????????????????????????????


// ?? LED RGB WS2812 embutido (DevKitC-1) ??
#define RGB_LED_PIN   48   // GPIO 48 ? LED NeoPixel da placa
#define RGB_LED_COUNT  1   // apenas 1 LED na placa


// ?? TFT ILI9341 (SPI via shield) ??
#define TFT_CS    14
#define TFT_DC    47
#define TFT_RST   21
#define TFT_MOSI  45
#define TFT_SCK    3


// ?? HC-SR04 ??
#define HCSR04_TRIG   12
#define HCSR04_ECHO   13


// ?? Sensor de Chuva MH-RD ??
#define RAIN_A0        7   // ADC1_CH6 ? analógico
#define RAIN_D0       15   // digital (HIGH = chuva)


// ?? DHT11 ? pino "2G" da shield ??
#define DHT_PIN        2   // GPIO 2
#define DHT_TYPE    DHT11


// ?? Semáforo LED Open-Smart ??
#define LED_RED        4
#define LED_YELLOW     5
#define LED_GREEN      6


// ?? Buzzer ativo ??
#define BUZZER_PIN    16


// ?????????????????????????????????????
//  LIMITES DE ALERTA
// ?????????????????????????????????????
#define DIST_MAXIMA    100   // cm ? reservatório vazio
#define NIVEL_ALERTA    50   // % ? estado ALERTA
#define NIVEL_CRITICO   80   // % ? estado CRÍTICO


// ?????????????????????????????????????
//  BRILHO DO LED RGB (0?255)
//  Reduza se o brilho incomodar
// ?????????????????????????????????????
#define RGB_BRIGHTNESS  60   // ~24% de brilho máximo


// ?????????????????????????????????????
//  CORES ILI9341 (RGB565)
// ?????????????????????????????????????
#define C_BLACK     0x0000
#define C_WHITE     0xFFFF
#define C_BGDARK    0x0208
#define C_PANEL     0x0410
#define C_BORDER    0x0821
#define C_GREEN     0x07E0
#define C_LGREEN    0x47E8
#define C_CYAN      0x07FF
#define C_DCYAN     0x03EF
#define C_YELLOW    0xFFE0
#define C_ORANGE    0xFBE0
#define C_RED       0xF800
#define C_DRED      0x8000
#define C_GRAY      0x4208
#define C_DGRAY     0x2104
#define C_PURPLE    0x801F
#define C_BLUE      0x001F
#define C_MAGENTA   0xF81F


// ?????????????????????????????????????
//  OBJETOS GLOBAIS
// ?????????????????????????????????????
Adafruit_ILI9341  tft      = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);
DHT               dht(DHT_PIN, DHT_TYPE);
WiFiClient        wifiClient;
PubSubClient      mqttClient(wifiClient);
Adafruit_NeoPixel rgbLed(RGB_LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);


// ?????????????????????????????????????
//  DADOS DOS SENSORES
// ?????????????????????????????????????
struct SensorData {
  float    distCm      = 0;
  int      nivelPct    = 0;
  int      rainA0      = 0;
  bool     rainDigital = false;
  float    tempC       = 0;
  float    humidity    = 0;
  int      rssi        = 0;
  bool     mqttOk      = false;
  uint32_t pubCount    = 0;
  uint32_t uptimeSec   = 0;
};
SensorData data;


// ?????????????????????????????????????
//  ESTADOS DO SISTEMA
// ?????????????????????????????????????
enum AlertLevel { NORMAL, ALERTA, CRITICO };
AlertLevel currentAlert = NORMAL;


enum Page { PAGE_HOME, PAGE_SENSORS, PAGE_ALERT, PAGE_MQTT };
Page currentPage = PAGE_HOME;


// ?????????????????????????????????????
//  ESTADO DO LED RGB
// ?????????????????????????????????????
enum RgbMode {
  RGB_BOOT,          // Branco pulsante   ? inicializando
  RGB_WIFI_TRYING,   // Ciano pulsante    ? conectando WiFi
  RGB_WIFI_OK,       // Ciano fixo        ? WiFi OK, MQTT offline
  RGB_MQTT_OK,       // Azul fixo         ? WiFi + MQTT conectados
  RGB_MQTT_PUB,      // Azul flash rápido ? publicando dado
  RGB_WIFI_LOST,     // Amarelo pulsante  ? WiFi caiu
  RGB_OFFLINE,       // Magenta fixo      ? sem WiFi
  RGB_ALERT_NORMAL,  // Verde fixo        ? nível normal
  RGB_ALERT_ALERTA,  // Amarelo fixo      ? nível alerta
  RGB_ALERT_CRITICO  // Vermelho pulsante ? nível crítico
};
RgbMode currentRgbMode = RGB_BOOT;


// ?????????????????????????????????????
//  HISTÓRICO NÍVEL
// ?????????????????????????????????????
#define HISTORY_LEN 60
uint8_t history[HISTORY_LEN];
uint8_t histIdx  = 0;
bool    histFull = false;


// ?????????????????????????????????????
//  TIMERS
// ?????????????????????????????????????
unsigned long tSensor   = 0;
unsigned long tDisplay  = 0;
unsigned long tMQTT     = 0;
unsigned long tUptime   = 0;
unsigned long tBuzzer   = 0;
unsigned long tRgb      = 0;   // timer do LED RGB
bool buzzerState = false;


// Controle de redesenho
AlertLevel lastAlert   = NORMAL;
Page       lastPage    = PAGE_HOME;
bool       forceRedraw = true;


// Flag de flash MQTT (pisca 1x ao publicar)
bool mqttFlashPending = false;
unsigned long tMqttFlash = 0;


// ??????????????????????????????????????????????????????????????????
//  PROTÓTIPOS
// ??????????????????????????????????????????????????????????????????
void     connectWiFi();
void     mqttLoop();
void     mqttCallback(char* topic, byte* payload, unsigned int len);
void     readSensors();
float    readHCSR04();
void     updateAlertLevel();
void     updateLEDs();
void     updateRgbLed();
void     setRgb(uint8_t r, uint8_t g, uint8_t b);
void     setRgbOff();
void     drawCurrentPage();
void     drawBootScreen();
void     drawPageHome();
void     drawPageSensors();
void     drawPageAlert();
void     drawPageMQTT();
void     drawTopbar(const char* title, uint16_t titleColor);
void     drawBottombar(Page activePage);
void     drawHBar(int x, int y, int w, int h, int pct, uint16_t fillColor, uint16_t bgColor);
void     drawVBar(int x, int y, int w, int h, int pct, uint16_t fillColor);
void     drawBigNumber(int x, int y, float val, int decimals, const char* unit, uint16_t color);
void     drawLabelValue(int x, int y, const char* label, const char* value, uint16_t valColor);
void     drawMiniChart(int x, int y, int w, int h);
uint16_t alertColor();
const char* alertName();
const char* rgbModeName();


// ??????????????????????????????????????????????????????????????????
//  SETUP
// ??????????????????????????????????????????????????????????????????
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\n[FLOOD·GUARD v2.1] ESP32-S3-N16R8 ? Iniciando..."));


  // ?? Pinos ??
  pinMode(HCSR04_TRIG, OUTPUT);
  pinMode(HCSR04_ECHO, INPUT);
  pinMode(RAIN_D0,     INPUT);
  pinMode(LED_RED,     OUTPUT);
  pinMode(LED_YELLOW,  OUTPUT);
  pinMode(LED_GREEN,   OUTPUT);
  pinMode(BUZZER_PIN,  OUTPUT);


  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);


  digitalWrite(LED_RED,    LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN,  HIGH);
  digitalWrite(BUZZER_PIN, LOW);


  // ?? LED RGB WS2812 ??
  rgbLed.begin();
  rgbLed.setBrightness(RGB_BRIGHTNESS);
  rgbLed.clear();
  rgbLed.show();


  // ?? DHT11 ??
  dht.begin();


  // ?? TFT ??
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(C_BLACK);


  // ?? Boot: LED branco pulsante ??
  currentRgbMode = RGB_BOOT;
  drawBootScreen();
  delay(1500);


  // ?? WiFi ? LED ciano pulsante durante conexão ??
  currentRgbMode = RGB_WIFI_TRYING;
  connectWiFi();


  // ?? MQTT ??
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setKeepAlive(30);
  mqttClient.setCallback(mqttCallback);


  memset(history, 0, sizeof(history));
  forceRedraw = true;
  tft.fillScreen(C_BGDARK);


  Serial.println(F("[FLOOD·GUARD] Pronto! v2.1 com LED RGB"));
}


// ??????????????????????????????????????????????????????????????????
//  LOOP PRINCIPAL
// ??????????????????????????????????????????????????????????????????
void loop() {
  unsigned long now = millis();


  // ?? Sensores (500ms) ??
  if (now - tSensor >= 500) {
    tSensor = now;
    readSensors();
    updateAlertLevel();
    updateLEDs();
  }


  // ?? Buzzer no CRÍTICO (600ms) ??
  if (currentAlert == CRITICO) {
    if (now - tBuzzer >= 600) {
      tBuzzer    = now;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
  }


  // ?? Uptime ??
  if (now - tUptime >= 1000) {
    tUptime = now;
    data.uptimeSec++;
  }


  // ?? LED RGB ? atualização contínua ??
  updateRgbLed();


  // ?? Display (1s) ??
  if (now - tDisplay >= 1000 || forceRedraw) {
    tDisplay   = now;
    drawCurrentPage();
    forceRedraw = false;
  }


  // ?? MQTT (5s) ??
  if (now - tMQTT >= 5000) {
    tMQTT = now;
    mqttLoop();
  }


  mqttClient.loop();
}


// ??????????????????????????????????????????????????????????????????
//  LED RGB WS2812 ? Lógica de Cores
//
//  PRIORIDADE (maior ? menor):
//    1. Conectividade (Boot > WiFi tentando > Offline)
//    2. MQTT (conectado / publicando)
//    3. Nível de alerta (crítico > alerta > normal)
// ??????????????????????????????????????????????????????????????????
void updateRgbLed() {
  unsigned long now = millis();


  // ?? Determina o modo atual com base no estado do sistema ??
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  bool mqttOk = mqttClient.connected();


  if (currentRgbMode == RGB_BOOT) {
    // Mantém boot até o setup() sair ? gerenciado externamente
  } else if (currentRgbMode == RGB_WIFI_TRYING) {
    // Gerenciado dentro de connectWiFi()
  } else if (!wifiOk) {
    // WiFi perdido após conexão
    currentRgbMode = RGB_WIFI_LOST;
  } else if (wifiOk && !mqttOk) {
    currentRgbMode = RGB_WIFI_OK;
  } else if (mqttFlashPending) {
    currentRgbMode = RGB_MQTT_PUB;
  } else if (wifiOk && mqttOk) {
    // MQTT ok ? sobrepõe cor de alerta SOMENTE para normal/alerta
    // CRÍTICO mantém vermelho pulsante mesmo com MQTT ok
    if (currentAlert == CRITICO) {
      currentRgbMode = RGB_ALERT_CRITICO;
    } else {
      currentRgbMode = RGB_MQTT_OK;
    }
  }


  // ?? Renderiza a cor conforme o modo ??
  switch (currentRgbMode) {


    // BRANCO pulsante ? Boot
    case RGB_BOOT: {
      uint8_t v = (uint8_t)(127 + 127 * sin(now / 300.0));
      setRgb(v, v, v);
      break;
    }


    // CIANO pulsante ? Tentando WiFi
    case RGB_WIFI_TRYING: {
      uint8_t v = (uint8_t)(100 + 100 * sin(now / 250.0));
      setRgb(0, v, v);   // ciano = verde + azul
      break;
    }


    // CIANO fixo ? WiFi OK, sem MQTT
    case RGB_WIFI_OK:
      setRgb(0, 80, 80);
      break;


    // AZUL fixo ? WiFi + MQTT conectados
    case RGB_MQTT_OK:
      setRgb(0, 0, 180);
      break;


    // AZUL flash rápido ? publicando dado MQTT
    case RGB_MQTT_PUB: {
      if (now - tMqttFlash < 150) {
        setRgb(0, 0, 255);   // azul brilhante
      } else {
        mqttFlashPending = false;
        currentRgbMode   = RGB_MQTT_OK;
        setRgb(0, 0, 180);
      }
      break;
    }


    // AMARELO pulsante ? WiFi caiu
    case RGB_WIFI_LOST: {
      if (now - tRgb >= 400) {
        tRgb = now;
        static bool tog = false;
        tog = !tog;
        tog ? setRgb(180, 100, 0) : setRgbOff();
      }
      break;
    }


    // MAGENTA fixo ? sem WiFi (offline)
    case RGB_OFFLINE:
      setRgb(150, 0, 150);
      break;


    // VERDE fixo ? nível NORMAL
    case RGB_ALERT_NORMAL:
      setRgb(0, 160, 0);
      break;


    // AMARELO fixo ? nível ALERTA
    case RGB_ALERT_ALERTA:
      setRgb(180, 100, 0);
      break;


    // VERMELHO pulsante ? nível CRÍTICO
    case RGB_ALERT_CRITICO: {
      uint8_t v = (uint8_t)(100 + 100 * sin(now / 200.0));
      setRgb(v, 0, 0);
      break;
    }


    default:
      setRgbOff();
  }
}


// ?? Helpers do LED RGB ??
void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  rgbLed.setPixelColor(0, rgbLed.Color(r, g, b));
  rgbLed.show();
}
void setRgbOff() {
  rgbLed.clear();
  rgbLed.show();
}


// ?? Nome do modo RGB para depuração ??
const char* rgbModeName() {
  switch (currentRgbMode) {
    case RGB_BOOT:          return "BOOT";
    case RGB_WIFI_TRYING:   return "WIFI_TRYING";
    case RGB_WIFI_OK:       return "WIFI_OK";
    case RGB_MQTT_OK:       return "MQTT_OK";
    case RGB_MQTT_PUB:      return "MQTT_PUB";
    case RGB_WIFI_LOST:     return "WIFI_LOST";
    case RGB_OFFLINE:       return "OFFLINE";
    case RGB_ALERT_NORMAL:  return "ALERT_NORMAL";
    case RGB_ALERT_ALERTA:  return "ALERT_ALERTA";
    case RGB_ALERT_CRITICO: return "ALERT_CRITICO";
    default:                return "?";
  }
}


// ??????????????????????????????????????????????????????????????????
//  LEITURA DE SENSORES
// ??????????????????????????????????????????????????????????????????
void readSensors() {
  data.distCm = readHCSR04();
  if (data.distCm <= 0 || data.distCm > DIST_MAXIMA) data.distCm = DIST_MAXIMA;
  data.nivelPct = constrain(
    (int)(((float)(DIST_MAXIMA - data.distCm) / DIST_MAXIMA) * 100.0f),
    0, 100
  );


  data.rainA0      = analogRead(RAIN_A0);
  data.rainDigital = (digitalRead(RAIN_D0) == HIGH);


  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) data.tempC    = t;
  if (!isnan(h)) data.humidity = h;


  data.rssi  = WiFi.RSSI();
  data.mqttOk = mqttClient.connected();


  history[histIdx] = (uint8_t)data.nivelPct;
  histIdx = (histIdx + 1) % HISTORY_LEN;
  if (histIdx == 0) histFull = true;
}


float readHCSR04() {
  digitalWrite(HCSR04_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(HCSR04_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(HCSR04_TRIG, LOW);
  long dur = pulseIn(HCSR04_ECHO, HIGH, 25000);
  if (dur == 0) return -1;
  return dur * 0.0343f / 2.0f;
}


// ??????????????????????????????????????????????????????????????????
//  NÍVEL DE ALERTA
// ??????????????????????????????????????????????????????????????????
void updateAlertLevel() {
  AlertLevel prev = currentAlert;


  if      (data.nivelPct >= NIVEL_CRITICO) currentAlert = CRITICO;
  else if (data.nivelPct >= NIVEL_ALERTA)  currentAlert = ALERTA;
  else                                     currentAlert = NORMAL;


  if (currentAlert != prev) {
    forceRedraw = true;
    Serial.printf("[ALERTA] %s ? %s | nivel=%d%%\n",
      prev == CRITICO ? "CRITICO" : prev == ALERTA ? "ALERTA" : "NORMAL",
      alertName(), data.nivelPct);
  }
}


// ??????????????????????????????????????????????????????????????????
//  LEDs SEMÁFORO OPEN-SMART
// ??????????????????????????????????????????????????????????????????
void updateLEDs() {
  digitalWrite(LED_RED,    currentAlert == CRITICO ? HIGH : LOW);
  digitalWrite(LED_YELLOW, currentAlert == ALERTA  ? HIGH : LOW);
  digitalWrite(LED_GREEN,  currentAlert == NORMAL  ? HIGH : LOW);
}


// ??????????????????????????????????????????????????????????????????
//  WiFi
// ??????????????????????????????????????????????????????????????????
void connectWiFi() {
  Serial.printf("[WiFi] Conectando a: %s\n", WIFI_SSID);
  currentRgbMode = RGB_WIFI_TRYING;


  tft.fillRect(0, 140, 320, 20, C_BGDARK);
  tft.setTextColor(C_CYAN); tft.setTextSize(1);
  tft.setCursor(10, 145);
  tft.print("WiFi: "); tft.print(WIFI_SSID);


  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    updateRgbLed();   // mantém pulsação ciano durante a espera
    delay(500);
    Serial.print(".");
    tries++;
  }


  tft.fillRect(0, 160, 320, 20, C_BGDARK);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] OK ? IP: %s  RSSI: %d dBm\n",
      WiFi.localIP().toString().c_str(), WiFi.RSSI());
    tft.setTextColor(C_GREEN); tft.setCursor(10, 165);
    tft.print("IP: "); tft.print(WiFi.localIP());
    currentRgbMode = RGB_WIFI_OK;   // Ciano fixo
  } else {
    Serial.println(F("\n[WiFi] FALHOU ? modo offline"));
    tft.setTextColor(C_YELLOW); tft.setCursor(10, 165);
    tft.print("Offline ? MQTT desabilitado");
    currentRgbMode = RGB_OFFLINE;   // Magenta fixo
  }
  updateRgbLed();
  delay(600);
}


// ??????????????????????????????????????????????????????????????????
//  MQTT
// ??????????????????????????????????????????????????????????????????
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  payload[len] = '\0';
  Serial.printf("[MQTT] CMD '%s': %s\n", topic, (char*)payload);
  StaticJsonDocument<128> cmd;
  if (deserializeJson(cmd, payload) == DeserializationError::Ok) {
    if (cmd.containsKey("cmd") && strcmp(cmd["cmd"], "reset") == 0)
      ESP.restart();
  }
}


void mqttLoop() {
  if (WiFi.status() != WL_CONNECTED) {
    currentRgbMode = RGB_WIFI_LOST;
    return;
  }


  if (!mqttClient.connected()) {
    Serial.print(F("[MQTT] Conectando a broker.hivemq.com..."));
    if (mqttClient.connect(MQTT_CLIENT_ID,
                            nullptr, nullptr,
                            MQTT_TOPIC_STATUS, 0, true, "offline")) {
      Serial.println(F(" OK"));
      mqttClient.publish(MQTT_TOPIC_STATUS, "online", true);
      mqttClient.subscribe(MQTT_TOPIC_CMD);
      currentRgbMode = RGB_MQTT_OK;   // Azul fixo
    } else {
      Serial.printf(" FALHOU rc=%d\n", mqttClient.state());
      currentRgbMode = RGB_WIFI_OK;   // Volta para ciano (WiFi ok, MQTT falhou)
      return;
    }
  }


  // ?? Monta JSON ??
  StaticJsonDocument<256> doc;
  doc["dispositivo"] = MQTT_CLIENT_ID;
  doc["nivel_pct"]   = data.nivelPct;
  doc["dist_cm"]     = data.distCm;
  doc["chuva"]       = data.rainDigital;
  doc["rain_a0"]     = data.rainA0;
  doc["temp_c"]      = data.tempC;
  doc["umidade"]     = data.humidity;
  doc["alerta"]      = alertName();
  doc["uptime_s"]    = data.uptimeSec;
  doc["rssi"]        = data.rssi;
  doc["rgb_mode"]    = rgbModeName();   // ? visível no dashboard MQTT


  char buf[256];
  serializeJson(doc, buf);


  if (mqttClient.publish(MQTT_TOPIC_PUB, buf)) {
    data.pubCount++;
    data.mqttOk = true;


    // Dispara flash azul brilhante por 150ms
    mqttFlashPending = true;
    tMqttFlash       = millis();
    currentRgbMode   = RGB_MQTT_PUB;


    Serial.printf("[MQTT] PUB #%lu ? nivel=%d%% alerta=%s rgb=%s\n",
      data.pubCount, data.nivelPct, alertName(), rgbModeName());
  } else {
    data.mqttOk = false;
    Serial.println(F("[MQTT] Falha ao publicar"));
  }
}


// ??????????????????????????????????????????????????????????????????
//  HELPERS
// ??????????????????????????????????????????????????????????????????
uint16_t alertColor() {
  if (currentAlert == CRITICO) return C_RED;
  if (currentAlert == ALERTA)  return C_YELLOW;
  return C_GREEN;
}
const char* alertName() {
  if (currentAlert == CRITICO) return "CRITICO";
  if (currentAlert == ALERTA)  return "ALERTA";
  return "NORMAL";
}


// ??????????????????????????????????????????????????????????????????
//  ROTEADOR DE PÁGINAS
// ??????????????????????????????????????????????????????????????????
void drawCurrentPage() {
  bool pageChanged  = (currentPage != lastPage);
  bool alertChanged = (currentAlert != lastAlert);


  if (pageChanged || alertChanged || forceRedraw)
    tft.fillScreen(C_BGDARK);


  switch (currentPage) {
    case PAGE_HOME:    drawPageHome();    break;
    case PAGE_SENSORS: drawPageSensors(); break;
    case PAGE_ALERT:   drawPageAlert();   break;
    case PAGE_MQTT:    drawPageMQTT();    break;
  }


  lastPage  = currentPage;
  lastAlert = currentAlert;


  static unsigned long tAutoPage = 0;
  if (millis() - tAutoPage >= 15000) {
    tAutoPage   = millis();
    currentPage = (Page)((currentPage + 1) % 4);
    tft.fillScreen(C_BGDARK);
  }
}


// ??????????????????????????????????????????????????????????????????
//  COMPONENTES DE DESENHO
// ??????????????????????????????????????????????????????????????????
void drawTopbar(const char* title, uint16_t titleColor) {
  tft.fillRect(0, 0, 320, 20, C_PANEL);
  tft.drawFastHLine(0, 20, 320, C_BORDER);
  tft.setTextColor(titleColor); tft.setTextSize(1);
  tft.setCursor(6, 6); tft.print(title);


  // Pill status (direita)
  uint16_t pillColor = alertColor();
  const char* s = alertName();
  int sw = strlen(s) * 6 + 8;
  int sx = 316 - sw;
  tft.drawRect(sx, 3, sw, 14, pillColor);
  tft.setTextColor(pillColor); tft.setCursor(sx + 4, 7); tft.print(s);


  // Uptime (centro)
  char clk[10];
  snprintf(clk, sizeof(clk), "%02lu:%02lu:%02lu",
    data.uptimeSec/3600, (data.uptimeSec%3600)/60, data.uptimeSec%60);
  tft.setTextColor(C_GRAY); tft.setCursor(134, 6); tft.print(clk);


  // Ícone de status do LED RGB (canto esquerdo após título)
  // Pequeno indicador colorido mostrando o estado da conectividade
  uint16_t dotColor;
  bool     wifiOk = (WiFi.status() == WL_CONNECTED);
  bool     mqOk   = mqttClient.connected();
  if      (!wifiOk)      dotColor = C_MAGENTA;
  else if (!mqOk)        dotColor = C_CYAN;
  else                   dotColor = C_BLUE;
  tft.fillCircle(116, 10, 4, dotColor);
  tft.drawCircle(116, 10, 4, C_BORDER);
}


void drawBottombar(Page activePage) {
  tft.fillRect(0, 218, 320, 22, C_PANEL);
  tft.drawFastHLine(0, 218, 320, C_BORDER);


  const char* labels[4] = { "HOME", "SENSOR", "ALERTA", "MQTT" };
  int w = 80;
  for (int i = 0; i < 4; i++) {
    int x = i * w;
    if (i == (int)activePage) {
      tft.fillRect(x, 219, w - 1, 21, C_BORDER);
      tft.setTextColor(alertColor());
    } else {
      tft.setTextColor(C_DGRAY);
    }
    tft.setCursor(x + (w - strlen(labels[i]) * 6) / 2, 225);
    tft.setTextSize(1); tft.print(labels[i]);
    if (i < 3) tft.drawFastVLine(x + w - 1, 218, 22, C_BORDER);
  }
  tft.drawFastHLine((int)activePage * w, 218, w - 1, alertColor());
}


void drawHBar(int x, int y, int w, int h, int pct, uint16_t fillColor, uint16_t bgColor) {
  tft.fillRect(x, y, w, h, bgColor);
  tft.drawRect(x, y, w, h, C_BORDER);
  if (pct > 0) {
    int fw = constrain((int)(pct / 100.0f * (w - 2)), 0, w - 2);
    tft.fillRect(x + 1, y + 1, fw, h - 2, fillColor);
  }
}


void drawVBar(int x, int y, int w, int h, int pct, uint16_t fillColor) {
  tft.fillRect(x, y, w, h, C_DGRAY);
  tft.drawRect(x, y, w, h, C_GRAY);
  int yWarn = y + h - (int)(h * (NIVEL_ALERTA  / 100.0f));
  int yCrit = y + h - (int)(h * (NIVEL_CRITICO / 100.0f));
  tft.drawFastHLine(x - 2, yWarn, w + 4, C_YELLOW);
  tft.drawFastHLine(x - 2, yCrit, w + 4, C_RED);
  if (pct > 0) {
    int fh = constrain((int)(pct / 100.0f * (h - 2)), 0, h - 2);
    tft.fillRect(x + 1, y + h - 1 - fh, w - 2, fh, fillColor);
  }
}


void drawBigNumber(int x, int y, float val, int decimals, const char* unit, uint16_t color) {
  tft.setTextColor(color); tft.setTextSize(3);
  char buf[12];
  if (decimals == 0) snprintf(buf, sizeof(buf), "%d", (int)val);
  else               snprintf(buf, sizeof(buf), "%.1f", val);
  tft.setCursor(x, y); tft.print(buf);
  tft.setTextSize(1); tft.setTextColor(C_GRAY);
  tft.setCursor(x + strlen(buf) * 18 + 2, y + 14); tft.print(unit);
}


void drawLabelValue(int x, int y, const char* label, const char* value, uint16_t valColor) {
  tft.setTextColor(C_DGRAY); tft.setTextSize(1);
  tft.setCursor(x, y); tft.print(label);
  tft.setTextColor(valColor);
  tft.setCursor(x + strlen(label) * 6 + 4, y); tft.print(value);
}


void drawMiniChart(int x, int y, int w, int h) {
  tft.fillRect(x, y, w, h, 0x0208);
  tft.drawRect(x, y, w, h, C_BORDER);
  int yw = y + h - (int)(h * (NIVEL_ALERTA  / 100.0f));
  int yc = y + h - (int)(h * (NIVEL_CRITICO / 100.0f));
  tft.drawFastHLine(x + 1, yw, w - 2, 0x4400);
  tft.drawFastHLine(x + 1, yc, w - 2, 0x4000);
  int total = histFull ? HISTORY_LEN : histIdx;
  if (total < 2) return;
  float step = (float)(w - 2) / (float)(total - 1);
  int prevX = -1, prevY = -1;
  for (int i = 0; i < total; i++) {
    int hi = (histIdx - total + i + HISTORY_LEN) % HISTORY_LEN;
    int px = x + 1 + (int)(i * step);
    int py = y + h - 1 - (int)(history[hi] / 100.0f * (h - 2));
    if (prevX >= 0) tft.drawLine(prevX, prevY, px, py, C_DCYAN);
    prevX = px; prevY = py;
  }
  tft.fillCircle(prevX, prevY, 2, C_CYAN);
}


// ??????????????????????????????????????????????????????????????????
//  PÁGINA 1 ? HOME
// ??????????????????????????????????????????????????????????????????
void drawPageHome() {
  drawTopbar("FLOOD GUARD", C_LGREEN);


  drawVBar(8, 28, 18, 155, data.nivelPct, alertColor());
  drawBigNumber(34, 32, data.distCm, 0, "cm", C_CYAN);


  tft.setTextColor(alertColor()); tft.setTextSize(2);
  char pctStr[6]; snprintf(pctStr, sizeof(pctStr), "%d%%", data.nivelPct);
  tft.setCursor(34, 62); tft.print(pctStr);
  tft.setTextColor(C_DGRAY); tft.setTextSize(1);
  tft.setCursor(34, 82); tft.print("NIVEL RELATIVO");
  tft.setCursor(34, 95); tft.print("RISCO:");
  drawHBar(34, 104, 132, 8, data.nivelPct, alertColor(), C_DGRAY);
  tft.setTextColor(C_GREEN);  tft.setCursor(34,  116); tft.print("OK");
  tft.setTextColor(C_YELLOW); tft.setCursor(68,  116); tft.print("ALERT");
  tft.setTextColor(C_RED);    tft.setCursor(116, 116); tft.print("CRIT");


  // Indicador RGB no canto inferior esquerdo (abaixo do tanque)
  tft.setTextColor(C_DGRAY); tft.setTextSize(1);
  tft.setCursor(4, 190); tft.print("RGB:");
  tft.setCursor(4, 202); tft.print(rgbModeName());


  tft.drawFastVLine(175, 22, 193, C_BORDER);
  int rx = 182;


  tft.setTextColor(C_GRAY); tft.setTextSize(1);
  tft.setCursor(rx, 26); tft.print("CHUVA  MH-RD");
  tft.drawFastHLine(rx, 35, 130, C_BORDER);


  if (data.rainDigital) {
    tft.setTextColor(C_CYAN); tft.setTextSize(2);
    tft.setCursor(rx, 40); tft.print("DETEC.");
    int pct = (int)(data.rainA0 / 4095.0f * 100);
    tft.setTextColor(C_DCYAN); tft.setTextSize(1); tft.setCursor(rx, 64);
    char rStr[20]; snprintf(rStr, sizeof(rStr), "A0:%d (%d%%)", data.rainA0, pct);
    tft.print(rStr);
    drawHBar(rx, 74, 128, 6, pct, C_CYAN, C_DGRAY);
  } else {
    tft.setTextColor(C_DGRAY); tft.setTextSize(2);
    tft.setCursor(rx, 40); tft.print("SECO");
    tft.setTextSize(1); tft.setCursor(rx, 64); tft.setTextColor(C_DGRAY);
    tft.print("Sem precipitacao");
    drawHBar(rx, 74, 128, 6, 0, C_CYAN, C_DGRAY);
  }


  tft.drawFastHLine(rx, 90, 130, C_BORDER);
  tft.setTextColor(C_GRAY); tft.setTextSize(1);
  tft.setCursor(rx, 94); tft.print("AMBIENTE  DHT11  G2");


  tft.setTextColor(C_ORANGE); tft.setTextSize(2); tft.setCursor(rx, 102);
  char tStr[8]; snprintf(tStr, sizeof(tStr), "%.0fC", data.tempC);
  tft.print(tStr);
  tft.setTextColor(C_DGRAY); tft.setTextSize(1);
  tft.setCursor(rx + strlen(tStr)*12 + 4, 110); tft.print("TEMP");


  tft.setTextColor(C_DCYAN); tft.setTextSize(2); tft.setCursor(rx + 70, 102);
  char hStr[6]; snprintf(hStr, sizeof(hStr), "%.0f%%", data.humidity);
  tft.print(hStr);
  tft.setTextColor(C_DGRAY); tft.setTextSize(1);
  tft.setCursor(rx + 70 + strlen(hStr)*12, 110); tft.print("UR");


  drawHBar(rx,      120, 60, 5, (int)(data.tempC / 50.0f * 100), C_ORANGE, C_DGRAY);
  drawHBar(rx + 68, 120, 60, 5, (int)data.humidity,              C_DCYAN,  C_DGRAY);


  float hi = data.tempC + 0.33f * data.humidity - 4.0f;
  tft.setTextColor(C_DGRAY); tft.setCursor(rx, 130); tft.print("SENSACAO:");
  char hiStr[8]; snprintf(hiStr, sizeof(hiStr), "%.0fC", hi);
  tft.setTextColor(C_ORANGE); tft.setCursor(rx + 60, 130); tft.print(hiStr);


  tft.setTextColor(C_DGRAY); tft.setCursor(rx, 144); tft.print("HISTORICO 60s");
  drawMiniChart(rx, 152, 128, 56);


  drawBottombar(PAGE_HOME);
}


// ??????????????????????????????????????????????????????????????????
//  PÁGINA 2 ? SENSORES
// ??????????????????????????????????????????????????????????????????
void drawPageSensors() {
  drawTopbar("SENSORES", C_CYAN);


  int y = 28, bx = 100, bw = 166, bh = 7;


  auto drawSensorRow = [&](const char* icon, const char* name,
                            int pct, uint16_t barColor, const char* valStr) {
    tft.fillRect(0, y, 320, 28, C_PANEL);
    tft.drawFastHLine(0, y, 320, C_BORDER);
    tft.setTextColor(C_GRAY);  tft.setTextSize(1); tft.setCursor(6, y + 4);  tft.print(icon);
    tft.setTextColor(C_DGRAY);                     tft.setCursor(6, y + 15); tft.print(name);
    drawHBar(bx, y + 10, bw, bh, pct, barColor, C_DGRAY);
    tft.setTextColor(barColor); tft.setTextSize(1); tft.setCursor(272, y + 8); tft.print(valStr);
    y += 30;
  };


  char s1[12]; snprintf(s1, sizeof(s1), "%.0fcm", data.distCm);
  drawSensorRow("[~]", "HC-SR04 NIVEL  ", data.nivelPct, alertColor(), s1);


  int p2 = (int)(data.rainA0 / 4095.0f * 100);
  char s2[10]; snprintf(s2, sizeof(s2), "%d", data.rainA0);
  drawSensorRow("[R]", "MH-RD   CHUVA  ", p2, C_CYAN, s2);


  int p3 = (int)(data.tempC / 50.0f * 100);
  char s3[8]; snprintf(s3, sizeof(s3), "%.0fC", data.tempC);
  drawSensorRow("[T]", "DHT11   TEMP G2", p3, C_ORANGE, s3);


  int p4 = (int)data.humidity;
  char s4[6]; snprintf(s4, sizeof(s4), "%d%%", p4);
  drawSensorRow("[H]", "DHT11   UMID G2", p4, C_DCYAN, s4);


  int rssiPct = constrain((int)((data.rssi + 100) / 60.0f * 100), 0, 100);
  char s5[10]; snprintf(s5, sizeof(s5), "%ddBm", data.rssi);
  drawSensorRow("[W]", "WiFi    RSSI   ", rssiPct, C_LGREEN, s5);


  // LED RGB status row
  tft.fillRect(0, y, 320, 28, C_PANEL);
  tft.drawFastHLine(0, y, 320, C_BORDER);
  tft.setTextColor(C_GRAY); tft.setTextSize(1);
  tft.setCursor(6, y + 4); tft.print("[L]");
  tft.setTextColor(C_DGRAY);
  tft.setCursor(6, y + 15); tft.print("RGB G48  MODO   ");


  // Cor representativa no TFT
  bool wOk = (WiFi.status() == WL_CONNECTED);
  bool mOk = mqttClient.connected();
  uint16_t dotC = wOk ? (mOk ? C_BLUE : C_CYAN) : C_MAGENTA;
  tft.fillRect(bx, y + 8, bw, bh, C_DGRAY);
  tft.drawRect( bx, y + 8, bw, bh, C_BORDER);
  tft.fillRect( bx + 1, y + 9, bw - 2, bh - 2, dotC);
  tft.setTextColor(dotC); tft.setCursor(272, y + 8);
  tft.print(wOk ? (mOk ? "AZ" : "CI") : "MG");
  y += 30;


  tft.drawFastHLine(0, y + 2, 320, C_BORDER);
  tft.setTextColor(C_DGRAY); tft.setCursor(6, y + 6); tft.print("HISTORICO NIVEL (60s)");
  drawMiniChart(6, y + 16, 308, 36);


  drawBottombar(PAGE_SENSORS);
}


// ??????????????????????????????????????????????????????????????????
//  PÁGINA 3 ? ALERTA / SEMÁFORO
// ??????????????????????????????????????????????????????????????????
void drawPageAlert() {
  drawTopbar("ALERTA", alertColor());


  tft.drawFastVLine(80, 22, 193, C_BORDER);
  int cx = 40;
  bool rOn = (currentAlert == CRITICO);
  bool yOn = (currentAlert == ALERTA);
  bool gOn = (currentAlert == NORMAL);


  tft.fillCircle(cx,  65, 20, rOn ? C_RED    : C_DGRAY); tft.drawCircle(cx,  65, 21, C_DRED);
  tft.fillCircle(cx, 120, 20, yOn ? C_YELLOW : C_DGRAY); tft.drawCircle(cx, 120, 21, 0x4400);
  tft.fillCircle(cx, 175, 20, gOn ? C_GREEN  : C_DGRAY); tft.drawCircle(cx, 175, 21, 0x0440);


  tft.setTextSize(1);
  tft.setCursor(26,  90); tft.setTextColor(rOn ? C_RED    : C_DGRAY); tft.print("CRIT");
  tft.setCursor(21, 145); tft.setTextColor(yOn ? C_YELLOW : C_DGRAY); tft.print("ALERT");
  tft.setCursor(25, 200); tft.setTextColor(gOn ? C_GREEN  : C_DGRAY); tft.print("NORM");


  int rx = 88;
  tft.setTextColor(alertColor()); tft.setTextSize(3);
  tft.setCursor(rx, 30); tft.print(alertName());
  tft.drawFastHLine(rx, 62, 225, C_BORDER);


  tft.setTextSize(1); tft.setCursor(rx, 68);
  if (currentAlert == CRITICO) {
    tft.setTextColor(C_RED);
    tft.print("RISCO DE ENCHENTE!");
    tft.setCursor(rx, 80); tft.print("Acionar equipe de emergencia");
    tft.setCursor(rx, 92); tft.print("Emitir alerta a populacao");
  } else if (currentAlert == ALERTA) {
    tft.setTextColor(C_YELLOW);
    tft.print("Nivel elevando. Monitorar!");
    tft.setCursor(rx, 80); tft.print("Monitoramento intensificado.");
    tft.setCursor(rx, 92); tft.print("Equipes em standby.");
  } else {
    tft.setTextColor(C_GREEN);
    tft.print("Sistema operando normalmente.");
    tft.setCursor(rx, 80); tft.print("Monitoramento padrao ativo.");
    tft.setCursor(rx, 92); tft.print("Sem risco detectado.");
  }


  tft.drawFastHLine(rx, 108, 225, C_BORDER);


  auto drawInfoCell = [&](int x, int y, int w, int h,
                          const char* k, const char* v, uint16_t vc) {
    tft.fillRect(x, y, w, h, C_PANEL); tft.drawRect(x, y, w, h, C_BORDER);
    tft.setTextColor(C_DGRAY); tft.setTextSize(1); tft.setCursor(x+4, y+4); tft.print(k);
    tft.setTextColor(vc); tft.setTextSize(2); tft.setCursor(x+4, y+16); tft.print(v);
  };


  char vN[6], vD[8], vC[4], vT[6];
  snprintf(vN, sizeof(vN), "%d%%",  data.nivelPct);
  snprintf(vD, sizeof(vD), "%.0fcm", data.distCm);
  snprintf(vC, sizeof(vC), "%s", data.rainDigital ? "SIM" : "NAO");
  snprintf(vT, sizeof(vT), "%.0fC", data.tempC);


  drawInfoCell(rx,       112, 107, 44, "NIVEL", vN, alertColor());
  drawInfoCell(rx + 113, 112, 112, 44, "DIST",  vD, C_CYAN);
  drawInfoCell(rx,       160, 107, 44, "CHUVA", vC, data.rainDigital ? C_CYAN : C_DGRAY);
  drawInfoCell(rx + 113, 160, 112, 44, "TEMP",  vT, C_ORANGE);


  if (currentAlert == CRITICO) {
    tft.setTextColor(C_RED); tft.setTextSize(1);
    tft.setCursor(rx, 210); tft.print("BUZZER ATIVO + RGB VERMELHO");
  }


  drawBottombar(PAGE_ALERT);
}


// ??????????????????????????????????????????????????????????????????
//  PÁGINA 4 ? MQTT / SISTEMA
// ??????????????????????????????????????????????????????????????????
void drawPageMQTT() {
  drawTopbar("MQTT  SISTEMA", C_PURPLE);


  int y = 28, x = 8;
  auto drawSysRow = [&](const char* k, const char* v, uint16_t vc, int barPct = -1) {
    tft.fillRect(0, y, 320, 22, C_PANEL);
    tft.drawFastHLine(0, y, 320, C_BORDER);
    tft.setTextColor(C_DGRAY); tft.setTextSize(1); tft.setCursor(x, y+7); tft.print(k);
    if (barPct >= 0) drawHBar(x+68, y+8, 160, 5, barPct, C_PURPLE, C_DGRAY);
    tft.setTextColor(vc); tft.setTextSize(1);
    tft.setCursor(barPct >= 0 ? (x+234) : (x+72), y+7); tft.print(v);
    y += 23;
  };


  bool wOk = (WiFi.status() == WL_CONNECTED);
  bool mOk = mqttClient.connected();


  drawSysRow("WiFi STATUS", wOk ? "CONECTADO" : "OFFLINE", wOk ? C_GREEN : C_RED);


  char ip[20]; WiFi.localIP().toString().toCharArray(ip, sizeof(ip));
  drawSysRow("IP LOCAL   ", ip, C_GRAY);


  char rssiStr[12]; snprintf(rssiStr, sizeof(rssiStr), "%d dBm", data.rssi);
  int rssiPct = constrain((int)((data.rssi + 100) / 60.0f * 100), 0, 100);
  drawSysRow("WiFi RSSI  ", rssiStr, C_LGREEN, rssiPct);


  drawSysRow("MQTT BROKER", "hivemq.com:1883", C_GRAY);
  drawSysRow("MQTT STATUS", mOk ? "CONECTADO" : "OFFLINE", mOk ? C_GREEN : C_RED);


  char pubStr[12]; snprintf(pubStr, sizeof(pubStr), "%lu", data.pubCount);
  int pubPct = constrain((int)(data.pubCount / 200.0f * 100), 0, 100);
  drawSysRow("PUB COUNT  ", pubStr, C_PURPLE, pubPct);


  // LED RGB status
  uint16_t rgbTftColor = !wOk ? C_MAGENTA : (!mOk ? C_CYAN : C_BLUE);
  drawSysRow("LED RGB G48", rgbModeName(), rgbTftColor);


  char upStr[12];
  snprintf(upStr, sizeof(upStr), "%02lu:%02lu:%02lu",
    data.uptimeSec/3600, (data.uptimeSec%3600)/60, data.uptimeSec%60);
  drawSysRow("UPTIME     ", upStr, C_GRAY);


  tft.fillRect(0, y, 320, 8, C_BGDARK); tft.drawFastHLine(0, y, 320, C_BORDER); y += 4;
  tft.setTextColor(C_DGRAY); tft.setCursor(x, y+4); tft.print("ULTIMA LEITURA:"); y += 14;
  char logStr[64];
  snprintf(logStr, sizeof(logStr), "nivel=%d%% dist=%.0fcm r=%d t=%.0f h=%.0f",
    data.nivelPct, data.distCm, data.rainDigital ? 1 : 0, data.tempC, data.humidity);
  tft.setTextColor(C_PURPLE); tft.setCursor(x, y+4); tft.print(logStr);


  drawBottombar(PAGE_MQTT);
}


// ??????????????????????????????????????????????????????????????????
//  TELA DE BOOT
// ??????????????????????????????????????????????????????????????????
void drawBootScreen() {
  tft.fillScreen(C_BLACK);


  tft.setTextColor(C_GREEN);  tft.setTextSize(3); tft.setCursor(42,  30); tft.print("FLOOD");
  tft.setTextColor(C_CYAN);                        tft.setCursor(138, 30); tft.print("GUARD");
  tft.drawFastHLine(20, 58, 280, C_BORDER);


  tft.setTextColor(C_DGRAY); tft.setTextSize(1);
  tft.setCursor(40, 68); tft.print("Sistema IoT de Alerta de Enchentes");
  tft.setCursor(88, 80); tft.print("ODS 11  UPM  2026");
  tft.drawFastHLine(20, 92, 280, C_BORDER);


  tft.setTextColor(C_GRAY);
  tft.setCursor(10, 100); tft.print("MCU:  ESP32-S3-N16R8 (16MB Flash / 8MB PSRAM)");
  tft.setCursor(10, 112); tft.print("TFT:  ILI9341 320x240 SPI");
  tft.setCursor(10, 124); tft.print("SENS: HC-SR04 + MH-RD + DHT11 (GPIO2)");
  tft.setCursor(10, 136); tft.print("LED:  Semaforo Open-Smart + RGB WS2812 G48");
  tft.setCursor(10, 148); tft.print("COM:  WiFi + MQTT HiveMQ | BAT: 18650");


  // Legenda do RGB no boot
  tft.drawFastHLine(20, 156, 280, C_BORDER);
  tft.setTextColor(C_DGRAY); tft.setCursor(10, 159); tft.print("RGB G48:");
  tft.setTextColor(0xFFFF);  tft.setCursor(60, 159); tft.print("BRANCO=boot");
  tft.setTextColor(C_CYAN);  tft.setCursor(140,159); tft.print("CIANO=wifi");
  tft.setTextColor(C_BLUE);  tft.setCursor(204,159); tft.print("AZUL=mqtt");


  tft.drawRect(20, 168, 280, 10, C_BORDER);
  for (int i = 0; i <= 280; i += 14) {
    tft.fillRect(21, 169, i, 8, C_GREEN);
    updateRgbLed();   // pulsação branca durante barra de progresso
    delay(30);
  }


  tft.setTextColor(C_GREEN); tft.setCursor(108, 184); tft.print("Inicializando...");
  tft.setTextColor(C_DGRAY); tft.setTextSize(1);
  tft.setCursor(10, 210); tft.print("Autores: J.Gomes  J.Liceras  R.Campean");
  tft.setCursor(56, 222); tft.print("Prof.: Andre Luis de Oliveira");
}

