/*
 * ╔══════════════════════════════════════════════════════════════════╗
 * ║          ESTACIÓN METEOROLÓGICA — ESP32 T-Display               ║
 * ║  Normas y Estándares / Protocolos de Telecomunicaciones         ║
 * ║  Alec Michele Marsicovetere  |  Carnet: 1311060                 ║
 * ╚══════════════════════════════════════════════════════════════════╝
 *
 *  Pantalla : LilyGo T-Display (135 x 240 px, horizontal = 240 x 135)
 *  Lib TFT  : TFT_eSPI
 *  Sensores : BME680, GY-30 (BH1750), MQ-2, MQ-7, MQ-8
 *  Extras   : WiFi, WebServer, ArduinoJson, NTP
 *
 *  Botón derecho (GPIO 35) → avanza de página
 */

// ─────────────────────────────────────────────────────────────────────────────
//  LIBRERÍAS
//  IMPORTANTE: FS.h debe ir ANTES de WebServer.h y TFT_eSPI.h para evitar
//  conflicto de namespace fs::FS en esp32-arduino 3.x
// ─────────────────────────────────────────────────────────────────────────────
#include <FS.h>           // ← primero, resuelve el namespace fs::FS
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>

// Sensores
#include <Adafruit_BME680.h>
#include <BH1750.h>

// ─────────────────────────────────────────────────────────────────────────────
//  ① CONFIGURACIÓN DE RED
// ─────────────────────────────────────────────────────────────────────────────
const char* WIFI_SSID     = "TU_RED_WIFI";        // ← tu red WiFi
const char* WIFI_PASSWORD = "TU_CONTRASENA";      // ← tu contraseña
const int   HTTP_PORT     = 80;

// NTP
const char* NTP_SERVER    = "pool.ntp.org";
const long  GMT_OFFSET_SEC  = -6 * 3600;     // UTC-6 Guatemala
const int   DAYLIGHT_OFFSET = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  ② PRESENCIA DE SENSORES  (true = instalado o con valor fijo, false = offline)
// ─────────────────────────────────────────────────────────────────────────────
#define SENSOR_BME680_PRESENTE   true
#define SENSOR_GY30_PRESENTE     true
#define SENSOR_MQ2_PRESENTE      true
#define SENSOR_MQ7_PRESENTE      false
#define SENSOR_MQ8_PRESENTE      false

// ─────────────────────────────────────────────────────────────────────────────
//  ③ VALORES FIJOS (para pruebas sin sensor real)
//     Pon USAR_VALOR_FIJO_xxx en true y define el valor deseado
// ─────────────────────────────────────────────────────────────────────────────
// BME680
#define USAR_VALOR_FIJO_BME680        true
#define VALOR_FIJO_PRESION            843.0f    // hPa — la presión baja ~12 hPa por cada 100m
#define VALOR_FIJO_TEMPERATURA        22.0f     // °C  — "ciudad de la eterna primavera"
#define VALOR_FIJO_HUMEDAD            72.0f     // %   — húmedo, especialmente en época lluviosa
#define VALOR_FIJO_GAS                40000.0f   // Ohms — aire relativamente limpio

// GY-30
#define USAR_VALOR_FIJO_GY30          true
#define VALOR_FIJO_LUX                420.0f     // lx

// MQ-2
#define USAR_VALOR_FIJO_MQ2           true
#define VALOR_FIJO_MQ2                512        // ADC 0-4095

// MQ-7
#define USAR_VALOR_FIJO_MQ7           false
#define VALOR_FIJO_MQ7                2047       // ADC 0-4095

// MQ-8
#define USAR_VALOR_FIJO_MQ8           false
#define VALOR_FIJO_MQ8                1024       // ADC 0-4095

// ─────────────────────────────────────────────────────────────────────────────
//  PINES
// ─────────────────────────────────────────────────────────────────────────────
#define BTN_RIGHT   35
#define PIN_MQ2     36   // ADC1_CH0  (ajusta según tu cableado)
#define PIN_MQ7     39   // ADC1_CH3
#define PIN_MQ8     34   // ADC1_CH6
#define TFT_BL      4    // Backlight T-Display

// ─────────────────────────────────────────────────────────────────────────────
//  COLORES (RGB565)
// ─────────────────────────────────────────────────────────────────────────────
#define COL_BME680   0x2AA5   // #2B50AA
#define COL_GY30     0xF480   // #EA9010  (aproximado RGB565)
#define COL_MQ2      0x7449   // #748E54
#define COL_MQ7      0x601F   // #6000F8  Morado
#define COL_MQ8      0x0493   // #049393  Teal/Verde-Azul oscuro
#define COL_OFFLINE  0x2104   // #272727
#define COL_SERVER   0x2104   // #272727
#define COL_STUDENT  0xB5B6   // #B5B682 (aproximado)
#define COL_WHITE    0xFFFF
#define COL_BLACK    0x0000
#define COL_GREEN    0x07E0
#define COL_DARKGRAY 0x4208
#define COL_YELLOW   0xFFE0
#define COL_CYAN     0x07FF
#define COL_ORANGE   0xFCA0
#define COL_RED      0xF800
#define COL_PRESION  0x5D1C   // azul claro para barra presión
#define COL_TEMP     0xFA40   // naranja para barra temperatura
#define COL_HUM      0x041F   // azul para barra humedad
#define COL_GAS_BAR  0x07E0   // verde para barra gas

// ─────────────────────────────────────────────────────────────────────────────
//  PÁGINAS DEL MENÚ
// ─────────────────────────────────────────────────────────────────────────────
enum Page {
  PAGE_BME680 = 0,
  PAGE_GY30,
  PAGE_MQ2,
  PAGE_MQ7,
  PAGE_MQ8,
  PAGE_SERVER,
  PAGE_STUDENT,
  PAGE_COUNT
};

// ─────────────────────────────────────────────────────────────────────────────
//  OBJETOS GLOBALES
// ─────────────────────────────────────────────────────────────────────────────
TFT_eSPI  tft  = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);   // sprite de doble buffer

Adafruit_BME680 bme;
BH1750          lightMeter;
WebServer       server(HTTP_PORT);

// ─────────────────────────────────────────────────────────────────────────────
//  ESTADO DE SENSORES (detectado en setup)
// ─────────────────────────────────────────────────────────────────────────────
bool bme680_ok  = false;
bool gy30_ok    = false;
bool mq2_ok     = false;  // MQ analógicos siempre "ok" si están presentes
bool mq7_ok     = false;
bool mq8_ok     = false;
bool wifi_ok    = false;

// ─────────────────────────────────────────────────────────────────────────────
//  DATOS DE SENSORES
// ─────────────────────────────────────────────────────────────────────────────
float bme_presion     = 0, bme_temp = 0, bme_hum = 0, bme_gas = 0;
float gy30_lux        = 0;
int   mq2_raw         = 0;
int   mq7_raw         = 0;
int   mq8_raw         = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  MENÚ
// ─────────────────────────────────────────────────────────────────────────────
int  currentPage    = 0;
bool btnWasPressed  = false;
unsigned long lastDraw   = 0;
unsigned long lastSensor = 0;

// ─────────────────────────────────────────────────────────────────────────────
//  HELPER: convierte hex color 24-bit a RGB565
// ─────────────────────────────────────────────────────────────────────────────
uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ─────────────────────────────────────────────────────────────────────────────
//  OBTENER HORA STRING  "12:34pm"
// ─────────────────────────────────────────────────────────────────────────────
String getTimeStr() {
  struct tm ti;
  if (!getLocalTime(&ti)) return "--:--";
  int h = ti.tm_hour;
  int m = ti.tm_min;
  const char* ampm = (h < 12) ? "am" : "pm";
  if (h == 0) h = 12;
  else if (h > 12) h -= 12;
  char buf[10];
  snprintf(buf, sizeof(buf), "%d:%02d%s", h, m, ampm);
  return String(buf);
}

// ─────────────────────────────────────────────────────────────────────────────
//  DIBUJA BARRA DE PROGRESO
//  x,y = esquina sup-izq, w,h = tamaño, val/mn/mx = rango, col = color relleno
// ─────────────────────────────────────────────────────────────────────────────
void drawBar(TFT_eSprite &s, int x, int y, int w, int h,
             float val, float mn, float mx, uint16_t colBar) {
  s.drawRect(x, y, w, h, COL_WHITE);
  int fill = constrain((int)((val - mn) / (mx - mn) * (w - 2)), 0, w - 2);
  s.fillRect(x + 1, y + 1, fill, h - 2, colBar);
}

// ─────────────────────────────────────────────────────────────────────────────
//  INDICADOR ESTADO  (circulo online/offline + hora) en esquina sup-der
// ─────────────────────────────────────────────────────────────────────────────
void drawStatusBar(TFT_eSprite &s, bool online, uint16_t bgColor) {
  // hora
  String t = getTimeStr();
  s.setTextColor(COL_WHITE, bgColor);
  s.setTextSize(1);
  s.setTextFont(1);
  int tw = s.textWidth(t);
  s.drawString(t, 240 - tw - 14, 3);
  // circulo
  uint16_t dotCol = online ? COL_GREEN : COL_DARKGRAY;
  s.fillCircle(240 - 6, 7, 4, dotCol);
  s.drawCircle(240 - 6, 7, 4, COL_WHITE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  PÁGINA: OFFLINE genérica
// ─────────────────────────────────────────────────────────────────────────────
void drawOfflinePage(TFT_eSprite &s, const char* sensorName) {
  s.fillSprite(COL_OFFLINE);
  drawStatusBar(s, false, COL_OFFLINE);
  s.setTextColor(COL_WHITE, COL_OFFLINE);
  s.setTextFont(4);
  s.setTextSize(1);
  int tw = s.textWidth(sensorName);
  s.drawString(sensorName, (240 - tw) / 2, 20);
  s.setTextFont(2);
  s.setTextColor(rgb(180,180,180), COL_OFFLINE);
  String msg = "OFFLINE";
  tw = s.textWidth(msg);
  s.drawString(msg, (240 - tw) / 2, 65);
  // icono X
  s.drawLine(95, 90, 145, 115, rgb(200,50,50));
  s.drawLine(145, 90, 95, 115, rgb(200,50,50));
}

// ─────────────────────────────────────────────────────────────────────────────
//  PÁGINA: BME680
// ─────────────────────────────────────────────────────────────────────────────
void drawBME680(TFT_eSprite &s) {
  if (!bme680_ok) { drawOfflinePage(s, "BME680"); return; }
  uint16_t BG = COL_BME680;
  s.fillSprite(BG);
  drawStatusBar(s, bme680_ok, BG);

  // Título
  s.setTextFont(4); s.setTextColor(COL_WHITE, BG);
  s.drawString("BME680", 6, 2);

  // Descripción
  s.setTextFont(1); s.setTextColor(rgb(200,210,255), BG);
  s.drawString("Presion, Temp, Humedad y Gas", 6, 22);

  // Separador
  s.drawLine(0, 32, 240, 32, rgb(100,130,200));

  // Icono simple (nube con ondas) en esquina derecha
  s.fillCircle(210, 20, 6, rgb(180,200,255));
  s.fillCircle(220, 16, 8, rgb(180,200,255));
  s.fillRect(202, 20, 26, 6, rgb(180,200,255));

  // ── MATRIZ 2×2 ──
  // Celda W/H
  const int CW = 116, CH = 44;
  const int X0 = 2, Y0 = 36;
  const int X1 = X0 + CW + 2;
  const int Y1 = Y0 + CH + 2;

  // fondo celdas
  uint16_t cellBg = rgb(20, 40, 100);
  s.fillRoundRect(X0, Y0, CW, CH, 4, cellBg);
  s.fillRoundRect(X1, Y0, CW, CH, 4, cellBg);
  s.fillRoundRect(X0, Y1, CW, CH, 4, cellBg);
  s.fillRoundRect(X1, Y1, CW, CH, 4, cellBg);

  // ── PRESIÓN ──
  s.setTextFont(1); s.setTextColor(rgb(150,170,255), cellBg);
  s.drawString("Presion (hPa)", X0+4, Y0+3);
  s.setTextFont(4); s.setTextColor(COL_WHITE, cellBg);
  char buf[20];
  dtostrf(bme_presion, 6, 1, buf);
  s.drawString(String(buf), X0+4, Y0+14);
  drawBar(s, X0+4, Y0+36, CW-8, 5, bme_presion, 870, 1084, COL_PRESION);

  // ── TEMPERATURA ──
  s.setTextFont(1); s.setTextColor(rgb(255,200,150), cellBg);
  s.drawString("Temperatura (C)", X1+4, Y0+3);
  s.setTextFont(4); s.setTextColor(COL_WHITE, cellBg);
  dtostrf(bme_temp, 4, 1, buf);
  s.drawString(String(buf) + "°", X1+4, Y0+14);
  drawBar(s, X1+4, Y0+36, CW-8, 5, bme_temp, -40, 85, COL_TEMP);

  // ── HUMEDAD ──
  s.setTextFont(1); s.setTextColor(rgb(150,210,255), cellBg);
  s.drawString("Humedad (%)", X0+4, Y1+3);
  s.setTextFont(4); s.setTextColor(COL_WHITE, cellBg);
  dtostrf(bme_hum, 4, 1, buf);
  s.drawString(String(buf) + "%", X0+4, Y1+14);
  drawBar(s, X0+4, Y1+36, CW-8, 5, bme_hum, 0, 100, COL_HUM);

  // ── GAS ──
  s.setTextFont(1); s.setTextColor(rgb(150,255,180), cellBg);
  s.drawString("Gas (KOhms)", X1+4, Y1+3);
  s.setTextFont(4); s.setTextColor(COL_WHITE, cellBg);
  float gasK = bme_gas / 1000.0f;
  dtostrf(gasK, 5, 1, buf);
  s.drawString(String(buf), X1+4, Y1+14);
  drawBar(s, X1+4, Y1+36, CW-8, 5, bme_gas, 0, 500000, COL_GAS_BAR);
}

// ─────────────────────────────────────────────────────────────────────────────
//  PÁGINA: GY-30 (BH1750)
// ─────────────────────────────────────────────────────────────────────────────
void drawGY30(TFT_eSprite &s) {
  if (!gy30_ok) { drawOfflinePage(s, "GY-30"); return; }
  uint16_t BG = COL_GY30;
  s.fillSprite(BG);
  drawStatusBar(s, gy30_ok, BG);

  s.setTextFont(4); s.setTextColor(COL_WHITE, BG);
  s.drawString("GY-30", 6, 2);
  s.setTextFont(1); s.setTextColor(rgb(255,220,150), BG);
  s.drawString("Sensor de Luminosidad BH1750", 6, 22);
  s.drawLine(0, 32, 240, 32, rgb(220,150,50));

  // Icono sol
  int sx = 210, sy = 18, sr = 7;
  s.fillCircle(sx, sy, sr, COL_YELLOW);
  for (int a = 0; a < 360; a += 45) {
    float rad = a * DEG_TO_RAD;
    int x1 = sx + cos(rad) * (sr + 2);
    int y1 = sy + sin(rad) * (sr + 2);
    int x2 = sx + cos(rad) * (sr + 5);
    int y2 = sy + sin(rad) * (sr + 5);
    s.drawLine(x1, y1, x2, y2, COL_YELLOW);
  }

  // Valor grande central
  char buf[20];
  dtostrf(gy30_lux, 6, 1, buf);
  s.setTextFont(7); s.setTextColor(COL_WHITE, BG);
  String luxStr = String(buf);
  int tw = s.textWidth(luxStr);
  s.drawString(luxStr, (240 - tw) / 2, 42);

  s.setTextFont(2); s.setTextColor(rgb(255,220,150), BG);
  s.drawString("lux", 120 - s.textWidth("lux")/2, 88);

  // Barra de nivel
  s.setTextFont(1); s.setTextColor(rgb(255,220,150), BG);
  s.drawString("0 lx", 4, 100);
  s.drawString("65535 lx", 160, 100);
  drawBar(s, 4, 110, 232, 10, gy30_lux, 0, 65535, COL_YELLOW);

  // Clasificación
  s.setTextFont(2); s.setTextColor(COL_WHITE, BG);
  const char* clase;
  if (gy30_lux < 1)       clase = "Oscuridad";
  else if (gy30_lux < 100) clase = "Interior Bajo";
  else if (gy30_lux < 500) clase = "Interior Normal";
  else if (gy30_lux < 5000) clase = "Nublado";
  else                     clase = "Luz Solar";
  s.drawString(clase, (240 - s.textWidth(clase)) / 2, 124);
}

// ─────────────────────────────────────────────────────────────────────────────
//  HELPER: dibuja página MQ genérica
// ─────────────────────────────────────────────────────────────────────────────
void drawMQPage(TFT_eSprite &s, bool online, uint16_t BG,
                const char* nombre, const char* desc,
                const char* gas1, const char* gas2, const char* gas3,
                int rawVal) {
  if (!online) { drawOfflinePage(s, nombre); return; }
  s.fillSprite(BG);
  drawStatusBar(s, true, BG);

  s.setTextFont(4); s.setTextColor(COL_WHITE, BG);
  s.drawString(nombre, 6, 2);
  s.setTextFont(1); s.setTextColor(rgb(255,230,200), BG);
  s.drawString(desc, 6, 22);
  s.drawLine(0, 32, 240, 32, rgb(200,200,200));

  // Voltage calculado (3.3V ref, 12-bit ADC)
  float voltage = rawVal * 3.3f / 4095.0f;
  // Porcentaje aproximado
  float pct = rawVal / 4095.0f * 100.0f;

  // Valor ADC grande
  uint16_t cellBg = rgb(20, 20, 30);
  s.fillRoundRect(4, 36, 150, 50, 4, cellBg);
  s.setTextFont(1); s.setTextColor(rgb(200,200,200), cellBg);
  s.drawString("Valor ADC (0-4095)", 8, 40);
  s.setTextFont(6); s.setTextColor(COL_WHITE, cellBg);
  s.drawString(String(rawVal), 8, 54);

  // Voltaje
  s.fillRoundRect(160, 36, 76, 50, 4, cellBg);
  s.setTextFont(1); s.setTextColor(rgb(200,200,200), cellBg);
  s.drawString("Voltaje", 164, 40);
  s.setTextFont(4); s.setTextColor(COL_YELLOW, cellBg);
  char vbuf[10]; dtostrf(voltage, 4, 2, vbuf);
  s.drawString(String(vbuf) + "V", 164, 58);

  // Barra
  s.setTextFont(1); s.setTextColor(COL_WHITE, BG);
  s.drawString("Nivel de Gas:", 4, 92);
  drawBar(s, 4, 104, 232, 12, rawVal, 0, 4095, BG == COL_MQ7 ? COL_CYAN : (BG == COL_MQ8 ? COL_YELLOW : COL_GREEN));

  // Porcentaje
  char pbuf[8]; dtostrf(pct, 4, 1, pbuf);
  s.setTextFont(2); s.setTextColor(COL_WHITE, BG);
  s.drawString(String(pbuf) + "%", 200, 90);

  // Gases detectados
  s.setTextFont(1); s.setTextColor(rgb(200,230,200), BG);
  s.drawString("Detecta:", 4, 120);
  s.setTextColor(COL_WHITE, BG);
  s.drawString(gas1, 55, 120);
  if (gas2[0]) { s.drawString(" | ", 55 + s.textWidth(gas1), 120); s.drawString(gas2, 55 + s.textWidth(gas1) + s.textWidth(" | "), 120); }
}

// ─────────────────────────────────────────────────────────────────────────────
//  PÁGINA: SERVIDOR HTTP
// ─────────────────────────────────────────────────────────────────────────────
void drawServerPage(TFT_eSprite &s) {
  uint16_t BG = COL_SERVER;
  s.fillSprite(BG);
  drawStatusBar(s, wifi_ok, BG);

  s.setTextFont(4); s.setTextColor(COL_WHITE, BG);
  s.drawString("Servidor HTTP", 6, 2);
  s.setTextFont(1); s.setTextColor(rgb(180,180,180), BG);
  s.drawString("Conectividad y endpoints", 6, 22);
  s.drawLine(0, 32, 240, 32, rgb(80,80,80));

  uint16_t rowBg = rgb(35,35,35);
  String ipStr  = wifi_ok ? WiFi.localIP().toString() : "No conectado";
  String ssidStr = wifi_ok ? String(WIFI_SSID) : "---";
  String portStr = String(HTTP_PORT);

  // Fila 1: Estado
  s.fillRect(4, 35, 232, 14, rowBg);
  s.setTextFont(1); s.setTextColor(rgb(160,160,160), rowBg);
  s.drawString("Estado:", 8, 38);
  s.setTextColor(wifi_ok ? COL_GREEN : COL_RED, rowBg);
  s.drawString(wifi_ok ? "ONLINE" : "OFFLINE", 60, 38);

  // Fila 2: IP  +  Puerto en la misma línea
  s.fillRect(4, 50, 232, 14, rowBg);
  s.setTextFont(1); s.setTextColor(rgb(160,160,160), rowBg);
  s.drawString("IP:", 8, 53);
  s.setTextColor(COL_CYAN, rowBg);
  s.drawString(ipStr, 30, 53);
  s.setTextColor(rgb(160,160,160), rowBg);
  s.drawString("Puerto:", 148, 53);
  s.setTextColor(COL_CYAN, rowBg);
  s.drawString(portStr, 192, 53);

  // Fila 3: Red SSID + círculo de estado
  s.fillRect(4, 65, 232, 14, rowBg);
  s.setTextFont(1); s.setTextColor(rgb(160,160,160), rowBg);
  s.drawString("Red:", 8, 68);
  s.setTextColor(COL_YELLOW, rowBg);
  s.drawString(ssidStr, 38, 68);
  // Círculo indicador conectado/desconectado
  uint16_t dotC = wifi_ok ? COL_GREEN : COL_RED;
  s.fillCircle(228, 72, 5, dotC);
  s.drawCircle(228, 72, 5, COL_WHITE);

  // Endpoints como lista
  s.fillRect(4, 81, 232, 50, rowBg);
  s.setTextFont(1); s.setTextColor(rgb(120,200,120), rowBg);
  s.drawString("Endpoints:", 8, 84);
  s.setTextColor(rgb(180,180,180), rowBg);
  if (wifi_ok) {
    String base = "http://" + WiFi.localIP().toString();
    s.drawString("/ -> Pagina HTML (auto-refresh 5s)", 12, 95);
    s.drawString("/data -> JSON todos los sensores",    12, 106);
    s.drawString("/health -> OK ping",                  12, 117);
  } else {
    s.setTextColor(COL_RED, rowBg);
    s.drawString("WiFi desconectado — sin endpoints", 8, 100);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  PÁGINA: DATOS DE ESTUDIANTE
// ─────────────────────────────────────────────────────────────────────────────
void drawStudentPage(TFT_eSprite &s) {
  uint16_t BG = COL_STUDENT;
  s.fillSprite(BG);
  drawStatusBar(s, true, BG);

  // Título grande
  s.setTextFont(4); s.setTextColor(COL_BLACK, BG);
  s.drawString("Weather Station", 6, 2);

  // Subtítulo
  s.setTextFont(1); s.setTextColor(rgb(60,60,40), BG);
  s.drawString("Estacion Meteorologica  |  ESP32 T-Display", 6, 24);
  s.drawLine(0, 33, 240, 33, rgb(100,100,70));

  // Datos compactos sin fondo de fila, solo texto pequeño
  uint16_t labelCol = rgb(70,70,40);
  uint16_t valCol   = COL_BLACK;

  struct { const char* label; const char* val; } rows[] = {
    { "Proyecto:", "Estacion Meteorologica"      },
    { "",          "Normas y Estandares Telecom." },
    { "",          "Protocolos de Telecom."       },
    { "Nombre:",   "Alec M. Marsicovetere"        },
    { "Carnet:",   "1311060"                      },
  };

  int yStart = 38;
  for (auto &r : rows) {
    s.setTextFont(1);
    if (r.label[0] != '\0') {
      s.setTextColor(labelCol, BG);
      s.drawString(r.label, 6, yStart);
    }
    s.setTextColor(valCol, BG);
    s.drawString(r.val, 60, yStart);
    yStart += 17;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  LEER SENSORES
// ─────────────────────────────────────────────────────────────────────────────
void readSensors() {
  // BME680
  // Si usa valor fijo, forzar ok=true y cargar valores directamente
  if (SENSOR_BME680_PRESENTE) {
    if (USAR_VALOR_FIJO_BME680) {
      bme680_ok   = true;
      bme_presion = VALOR_FIJO_PRESION;
      bme_temp    = VALOR_FIJO_TEMPERATURA;
      bme_hum     = VALOR_FIJO_HUMEDAD;
      bme_gas     = VALOR_FIJO_GAS;
    } else if (bme680_ok) {
      if (bme.performReading()) {
        bme_presion = bme.pressure / 100.0f;
        bme_temp    = bme.temperature;
        bme_hum     = bme.humidity;
        bme_gas     = bme.gas_resistance;
      }
    }
  }

  // GY-30
  if (SENSOR_GY30_PRESENTE) {
    if (USAR_VALOR_FIJO_GY30) {
      gy30_ok  = true;
      gy30_lux = VALOR_FIJO_LUX;
    } else if (gy30_ok) {
      gy30_lux = lightMeter.readLightLevel();
    }
  }

  // MQ-2
  if (SENSOR_MQ2_PRESENTE) {
    mq2_ok  = true;
    mq2_raw = USAR_VALOR_FIJO_MQ2 ? VALOR_FIJO_MQ2 : analogRead(PIN_MQ2);
  }

  // MQ-7
  if (SENSOR_MQ7_PRESENTE || USAR_VALOR_FIJO_MQ7) {
    mq7_ok  = true;
    mq7_raw = USAR_VALOR_FIJO_MQ7 ? VALOR_FIJO_MQ7 : analogRead(PIN_MQ7);
  }

  // MQ-8
  if (SENSOR_MQ8_PRESENTE || USAR_VALOR_FIJO_MQ8) {
    mq8_ok  = true;
    mq8_raw = USAR_VALOR_FIJO_MQ8 ? VALOR_FIJO_MQ8 : analogRead(PIN_MQ8);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  RENDERIZAR PÁGINA ACTUAL
// ─────────────────────────────────────────────────────────────────────────────
void renderPage() {
  spr.createSprite(240, 135);
  switch (currentPage) {
    case PAGE_BME680:  drawBME680(spr); break;
    case PAGE_GY30:    drawGY30(spr);   break;
    case PAGE_MQ2:
      drawMQPage(spr, mq2_ok, COL_MQ2, "MQ-2",
        "Gas, Combustible y Humo",
        "GLP", "Butano", "Humo", mq2_raw);
      break;
    case PAGE_MQ7:
      drawMQPage(spr, mq7_ok, COL_MQ7, "MQ-7",
        "Monoxido de Carbono (CO)",
        "CO", "", "", mq7_raw);
      break;
    case PAGE_MQ8:
      drawMQPage(spr, mq8_ok, COL_MQ8, "MQ-8",
        "Detector de Hidrogeno H2",
        "H2", "GLP", "", mq8_raw);
      break;
    case PAGE_SERVER:  drawServerPage(spr);  break;
    case PAGE_STUDENT: drawStudentPage(spr); break;
  }
  spr.pushSprite(0, 0);
  spr.deleteSprite();
}

// ─────────────────────────────────────────────────────────────────────────────
//  ENDPOINTS HTTP
// ─────────────────────────────────────────────────────────────────────────────
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta http-equiv='refresh' content='5'>"
    "<title>Estacion Meteorologica</title>"
    "<style>body{font-family:sans-serif;background:#1a1a2e;color:#eee;margin:20px}"
    "h1{color:#5bc0eb}.card{background:#16213e;border-radius:8px;padding:15px;margin:10px 0}"
    ".val{font-size:1.4em;font-weight:bold;color:#5bc0eb}"
    ".off{color:#888}</style></head><body>"
    "<h1>Estacion Meteorologica</h1>"
    "<p>Alec Michele Marsicovetere | Carnet: 1311060</p>";

  if (bme680_ok) {
    html += "<div class='card'><b>BME680</b><br>"
      "<span class='val'>" + String(bme_presion, 1) + " hPa</span> Presion &nbsp; "
      "<span class='val'>" + String(bme_temp, 1) + " °C</span> Temp &nbsp; "
      "<span class='val'>" + String(bme_hum, 1) + "%</span> Humedad &nbsp; "
      "<span class='val'>" + String(bme_gas / 1000.0f, 1) + " KΩ</span> Gas</div>";
  }
  if (gy30_ok) {
    html += "<div class='card'><b>GY-30</b><br>"
      "<span class='val'>" + String(gy30_lux, 1) + " lx</span> Luminosidad</div>";
  }
  if (mq2_ok) {
    html += "<div class='card'><b>MQ-2</b><br>"
      "<span class='val'>" + String(mq2_raw) + "</span> ADC</div>";
  }
  if (mq7_ok) {
    html += "<div class='card'><b>MQ-7</b><br>"
      "<span class='val'>" + String(mq7_raw) + "</span> ADC</div>";
  }
  if (mq8_ok) {
    html += "<div class='card'><b>MQ-8</b><br>"
      "<span class='val'>" + String(mq8_raw) + "</span> ADC</div>";
  }
  html += "<p style='color:#555'>Auto-refresh cada 5s | IP: " + WiFi.localIP().toString() + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleData() {
  StaticJsonDocument<512> doc;
  doc["device"]    = "ESP32-T-Display";
  doc["proyecto"]  = "Estacion Meteorologica";
  doc["estudiante"] = "Alec Michele Marsicovetere";
  doc["carnet"]    = "1311060";

  JsonObject bmeObj = doc.createNestedObject("BME680");
  bmeObj["online"]      = bme680_ok;
  bmeObj["presion_hPa"] = bme680_ok ? bme_presion : 0;
  bmeObj["temp_C"]      = bme680_ok ? bme_temp    : 0;
  bmeObj["humedad_pct"] = bme680_ok ? bme_hum     : 0;
  bmeObj["gas_ohms"]    = bme680_ok ? bme_gas     : 0;

  JsonObject gy30Obj = doc.createNestedObject("GY30");
  gy30Obj["online"] = gy30_ok;
  gy30Obj["lux"]    = gy30_ok ? gy30_lux : 0;

  JsonObject mq2Obj = doc.createNestedObject("MQ2");
  mq2Obj["online"] = mq2_ok;
  mq2Obj["adc"]    = mq2_ok ? mq2_raw : 0;

  JsonObject mq7Obj = doc.createNestedObject("MQ7");
  mq7Obj["online"] = mq7_ok;
  mq7Obj["adc"]    = mq7_ok ? mq7_raw : 0;

  JsonObject mq8Obj = doc.createNestedObject("MQ8");
  mq8Obj["online"] = mq8_ok;
  mq8Obj["adc"]    = mq8_ok ? mq8_raw : 0;

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleHealth() {
  server.send(200, "text/plain", "OK");
}

// ─────────────────────────────────────────────────────────────────────────────
//  PANTALLA DE ARRANQUE
// ─────────────────────────────────────────────────────────────────────────────
void showBootScreen(const char* msg, uint16_t col = COL_WHITE) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(col, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.drawString(msg, 10, 60);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Botón
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  // Backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // TFT
  tft.init();
  tft.setRotation(1);   // horizontal, 240x135
  tft.fillScreen(TFT_BLACK);

  // ── Pantalla de boot ──
  showBootScreen("Estacion Meteorologica", COL_CYAN);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(1);
  tft.drawString("Alec M. Marsicovetere  |  1311060", 10, 80);
  tft.drawString("Inicializando sensores...", 10, 95);
  delay(800);

  // ── Wire (I2C) ──
  Wire.begin();

  // ── BME680 ──
  tft.drawString("BME680...", 10, 110);
  if (SENSOR_BME680_PRESENTE) {
    if (bme.begin()) {
      bme.setTemperatureOversampling(BME680_OS_8X);
      bme.setHumidityOversampling(BME680_OS_2X);
      bme.setPressureOversampling(BME680_OS_4X);
      bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
      bme.setGasHeater(320, 150);
      bme680_ok = true;
      tft.setTextColor(COL_GREEN, TFT_BLACK);
      tft.drawString("BME680 OK", 10, 110);
    } else {
      tft.setTextColor(COL_RED, TFT_BLACK);
      tft.drawString("BME680 NO ENCONTRADO", 10, 110);
    }
  } else {
    tft.setTextColor(COL_DARKGRAY, TFT_BLACK);
    tft.drawString("BME680 OMITIDO", 10, 110);
  }

  // ── GY-30 ──
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("GY-30...", 130, 110);
  if (SENSOR_GY30_PRESENTE) {
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
      gy30_ok = true;
      tft.setTextColor(COL_GREEN, TFT_BLACK);
      tft.drawString("GY-30 OK  ", 130, 110);
    } else {
      tft.setTextColor(COL_RED, TFT_BLACK);
      tft.drawString("GY-30 FAIL", 130, 110);
    }
  }

  // ── MQ analógicos ──
  mq2_ok = SENSOR_MQ2_PRESENTE;
  mq7_ok = SENSOR_MQ7_PRESENTE || USAR_VALOR_FIJO_MQ7;
  mq8_ok = SENSOR_MQ8_PRESENTE || USAR_VALOR_FIJO_MQ8;

  delay(600);

  // ── WiFi ──
  tft.fillScreen(TFT_BLACK);
  showBootScreen("Conectando WiFi...", COL_YELLOW);
  tft.setTextFont(1);
  tft.drawString(WIFI_SSID, 10, 80);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    tft.drawString(".", 10 + tries * 6, 95);
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifi_ok = true;
    tft.setTextColor(COL_GREEN, TFT_BLACK);
    tft.drawString("WiFi OK! IP: " + WiFi.localIP().toString(), 10, 110);

    // NTP
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);

    // Servidor HTTP
    server.on("/",       handleRoot);
    server.on("/data",   handleData);
    server.on("/health", handleHealth);
    server.begin();
  } else {
    tft.setTextColor(COL_RED, TFT_BLACK);
    tft.drawString("WiFi FALLÓ — modo sin red", 10, 110);
  }

  delay(1000);

  // Primera lectura
  readSensors();
  renderPage();
}

// ─────────────────────────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  // Servidor HTTP
  if (wifi_ok) server.handleClient();

  // Botón derecho (GPIO35) — cambia de página
  bool btnPressed = (digitalRead(BTN_RIGHT) == LOW);
  if (btnPressed && !btnWasPressed) {
    currentPage = (currentPage + 1) % PAGE_COUNT;
    renderPage();
  }
  btnWasPressed = btnPressed;

  // Leer sensores cada 2 s
  if (millis() - lastSensor > 2000) {
    readSensors();
    lastSensor = millis();
  }

  // Redibujar cada 5 s (actualiza hora, etc.)
  if (millis() - lastDraw > 5000) {
    renderPage();
    lastDraw = millis();
  }

  delay(50);
}

/*
 * ══════════════════════════════════════════════════════════════════
 *  LIBRERÍAS REQUERIDAS (instalar por Arduino Library Manager)
 *  ─────────────────────────────────────────────────────────────
 *  • TFT_eSPI          — Bodmer
 *    → User_Setup: seleccionar LILYGO T-Display (o usar el preset)
 *  • Adafruit BME680   — Adafruit
 *  • Adafruit Unified Sensor — Adafruit
 *  • BH1750            — Christopher Laws
 *  • ArduinoJson       — Benoit Blanchon  (v6.x)
 *  ─────────────────────────────────────────────────────────────
 *  CONFIGURACIÓN TFT_eSPI (User_Setup.h)
 *  Descomentar:  #define LILYGO_T_DISPLAY
 *  O definir manualmente los pines del T-Display:
 *    #define TFT_MOSI 19
 *    #define TFT_SCLK 18
 *    #define TFT_CS    5
 *    #define TFT_DC   16
 *    #define TFT_RST  23
 *    #define TFT_BL    4
 *    #define ST7789_DRIVER
 *    #define TFT_WIDTH  135
 *    #define TFT_HEIGHT 240
 * ══════════════════════════════════════════════════════════════════
 */
