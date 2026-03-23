#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include <DHT.h>
#include <XPT2046_Touchscreen.h>

#define W        320
#define H        240
#define SSID     "F3"
#define PASS     "uwu"
#define NTP      "pool.ntp.org"
#define TZ       "PST8PDT,M3.2.0,M11.1.0"
#define DHT_PIN  22
#define DHT_TYPE DHT11

#define T_IRQ    36
#define T_MOSI   32
#define T_MISO   39
#define T_CLK    25
#define T_CS     33


TFT_eSPI tft = TFT_eSPI();
DHT dht(DHT_PIN, DHT_TYPE);

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen ts(T_CS, T_IRQ);

enum Screen { HOME, POMO_SET, POMO_RUN, POMO_DONE };
Screen currentScreen = HOME;

const int POMO_TIMES[] = { 1, 5, 10, 15, 20, 25, 30, 60 };
const int POMO_COUNT = 8;

int pomoIdx = 1;
unsigned long pomoStartMs = 0;
unsigned long pomoDurMs = 0;
unsigned long pomoDoneMs = 0;
unsigned long pomoFlashMs = 0;
bool pomoFlashOn = false;
int prevRemaining = -1;

char  prevTime[12] = "";
char  prevAmpm[3] = "";
float prevTemp    = NAN;
float prevHumi    = NAN;

unsigned long prevClockMs  = 0;
unsigned long prevSensorMs = 0;

struct Pt { int x, y; };
Pt getTouch() {
  TS_Point p = ts.getPoint();
  return {
    constrain((int)map(p.x, 200, 3800, 0, W), 0, W - 1),
    constrain((int)map(p.y, 200, 3800, 0, H), 0, H - 1)
  };
}

bool isTouched() { return ts.tirqTouched() && ts.touched(); }
bool hit(Pt p, int x, int y, int w, int h) { return p.x >= x && p.x <= x+w && p.y >= y && p.y <= y+h; }
bool waitRelease() { while (isTouched()) delay(10); }

void drawBorder(uint16_t color) {
  for (int i = 0; i < 3; i++)
    tft.drawRoundRect(4+i, 4+i, W-8-i*2, H-8-i*2, 8, color);
}

void drawHomeBase() { // im stupid this is the best name i could think of so shut up
  tft.fillScreen(TFT_BLACK);
  drawBorder(0x07FF);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(0x8410, TFT_BLACK);
  tft.drawString("TEMPERATURE:", 80, 165, 2);
  tft.drawString("HUMIDITY:", 240, 165, 2);
  tft.drawFastHLine(20, 150, W - 40, 0x4208);
  prevTime[0] = '\0';
  prevAmpm[0] = '\0';
  prevTemp = NAN;
  prevHumi = NAN;
}

void updateClock() {
  struct tm t;
  if (!getLocalTime(&t, 200)) return;

  int hour12 = t.tm_hour % 12;
  if (hour12 == 0) hour12 = 12;
  const char *ampm = (t.tm_hour < 12) ? "AM" : "PM";

  char buf[12];
  snprintf(buf, sizeof(buf), "%2d:%02d:%02d", hour12, t.tm_min, t.tm_sec);

  if (strcmp(buf, prevTime) != 0) {
    strcpy(prevTime, buf);
    tft.setTextSize(2.5);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(0x07FF, TFT_BLACK);
    tft.drawString(buf, W / 2, 82, 4);
    tft.setTextSize(1);
  }

  if (strcmp(ampm, prevAmpm) != 0) {
    strcpy(prevAmpm, ampm);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(ampm, W / 2, 125, 4);
  }
}

void updateSensors() {
  float temp = dht.readTemperature();
  float humi = dht.readHumidity();

  if (!isnan(temp) && temp != prevTemp) {
    prevTemp = temp;
    char buf[10];
    snprintf(buf, sizeof(buf), "%.1fF  ", temp * 9.0f / 5.0f + 32.0f);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(buf, 80, 196, 4);
  }

  if (!isnan(humi) && humi != prevHumi) {
    prevHumi = humi;
    char buf[8];
    snprintf(buf, sizeof(buf), "%.0f%%  ", humi);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(buf, 240, 196, 4);
  }
}

void drawPomoSetTime() {
  tft.fillRect(110, 88, 100, 64, TFT_BLACK);
  char buf[6];
  snprintf(buf, sizeof(buf), "%dm", POMO_TIMES[pomoIdx]);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(buf, W / 2, 120, 4);
  tft.setTextSize(1);
}

void drawPomoSet() {
  tft.fillScreen(TFT_BLACK);
  drawBorder(TFT_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("POMODORO!!!", W / 2, 28, 4);
  tft.fillRoundRect(25,  88, 70, 64, 8, 0x2945);
  tft.drawRoundRect(25,  88, 70, 64, 8, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, 0x2945);
  tft.drawString("-", 60, 120, 4);
  tft.fillRoundRect(225, 88, 70, 64, 8, 0x2945);
  tft.drawRoundRect(225, 88, 70, 64, 8, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, 0x2945);
  tft.drawString("+", 260, 120, 4);
  drawPomoSetTime();
  tft.fillRoundRect(95, 178, 130, 44, 8, TFT_RED);
  tft.drawRoundRect(95, 178, 130, 44, 8, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawString("START!!!", W / 2, 200, 4);
}

void updatePomoRun() {
  unsigned long elapsed = millis() - pomoStartMs;
  if (elapsed >= pomoDurMs) {
    currentScreen = POMO_DONE;
    pomoDoneMs = millis();
    pomoFlashOn = true;
    pomoFlashMs = millis();
    tft.fillScreen(TFT_WHITE);
    return;
  }
  int remaining = (int)((pomoDurMs - elapsed) / 1000);
  if (remaining == prevRemaining) return;
  prevRemaining = remaining;
  // ethical disclaimer: AI was used for part of the code below (approx 5 lines).
  int cx = W / 2, cy = 108, r = 80, ir = 62;
  int endDeg = (int)(360.0f * (1.0f - (float)elapsed / (float)pomoDurMs));
  tft.drawArc(cx, cy, r, ir, 0,      endDeg, TFT_BLACK, TFT_BLACK);
  tft.drawArc(cx, cy, r, ir, endDeg, 360,    TFT_RED,   TFT_BLACK);
  int mins = remaining / 60, secs = remaining % 60;
  char buf[6];
  snprintf(buf, sizeof(buf), "%d:%02d", mins, secs);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(buf, W / 2, 108, 4);
  tft.setTextSize(1);
  tft.setTextColor(0x4208, TFT_BLACK);
  tft.drawString("tap to cancel", W / 2, 215, 2);
}

void drawPomoRun() {
  tft.fillScreen(TFT_BLACK);
  drawBorder(TFT_RED);
  prevRemaining = -1;
  updatePomoRun();
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  dht.begin();
  drawBorder(0x07FF);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("connecting to wifi :3", W / 2, H / 2, 4);

  WiFi.begin(SSID, PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20)
 {  delay(500); tries++; } 
  if (WiFi.status() != WL_CONNECTED) {
    tft.fillScreen(TFT_BLACK);
    drawBorder(0x07FF);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("wifi failed :c", W / 2, H / 2 - 16, 4);
    tft.setTextColor(0x8410, TFT_BLACK);
    tft.drawString("check ssid & pass!", W / 2, H / 2 + 16, 2);
    while (true) delay(1000);
  }

  configTzTime(TZ, NTP);
  struct tm t;
  while (!getLocalTime(&t, 200)) delay(500);

  touchSPI.begin(T_CLK, T_MISO, T_MOSI, T_CS);
  ts.begin(touchSPI);
  ts.setRotation(1);
  drawHomeBase();
  currentScreen = HOME;
}

void loop() {
  unsigned long now = millis();

  if (currentScreen == HOME) {
    if (now - prevClockMs >= 1000)  { prevClockMs = now;  updateClock();   }
    if (now - prevSensorMs >= 3000) { prevSensorMs = now; updateSensors(); }
  }
  if (currentScreen == POMO_RUN) updatePomoRun();
  if (currentScreen == POMO_DONE) {
    if (now - pomoFlashMs >= 500) {
      pomoFlashMs = now;
      pomoFlashOn = !pomoFlashOn;
      tft.fillScreen(pomoFlashOn ? TFT_WHITE : TFT_RED);
    }
    if (now - pomoDoneMs >= 60000) { drawHomeBase(); currentScreen = HOME; }
  }

  if (!isTouched()) return;
  Pt p = getTouch();
  delay(30); // debounce

  switch (currentScreen) {
    case HOME: //c(l)ock screen :\3
      currentScreen = POMO_SET;
      drawPomoSet();
      break;
    case POMO_SET:
      if (hit(p, 25, 88, 70, 64))        { pomoIdx = (pomoIdx - 1 + POMO_COUNT) % POMO_COUNT; drawPomoSetTime(); }
      else if (hit(p, 225, 88, 70, 64))  { pomoIdx = (pomoIdx + 1) % POMO_COUNT; drawPomoSetTime(); }
      else if (hit(p, 95, 178, 130, 44)) { pomoDurMs = (unsigned long)POMO_TIMES[pomoIdx] * 60UL * 1000UL; pomoStartMs = millis(); currentScreen = POMO_RUN; drawPomoRun(); }
      break;
    case POMO_RUN:
      drawPomoSet();
      break;
    case POMO_DONE:
      drawHomeBase();
      currentScreen = HOME;
      break;
  }
  waitRelease();
}
