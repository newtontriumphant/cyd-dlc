#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include <DHT.h>

#define W        320
#define H        240
#define SSID     "F3"
#define PASS     "weatherboy"
#define NTP      "pool.ntp.org"
#define TZ       "PST8PDT,M3.2.0,M11.1.0"
#define DHT_PIN  22
#define DHT_TYPE DHT11

TFT_eSPI tft = TFT_eSPI();
DHT dht(DHT_PIN, DHT_TYPE);

char  prevTime[12] = "";
char  prevAmpm[3] = "";
float prevTemp    = NAN;
float prevHumi    = NAN;

unsigned long prevClockMs  = 0;
unsigned long prevSensorMs = 0;

void drawBorder() {
  for (int i = 0; i < 3; i++)
    tft.drawRoundRect(4 + i, 4 + i, W - 8 - i*2, H - 8 - i*2, 8, 0x07FF);
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

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  dht.begin();
  drawBorder();

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("connecting to wifi :3", W / 2, H / 2, 4);

  WiFi.begin(SSID, PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    tft.fillScreen(TFT_BLACK);
    drawBorder();
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

  tft.fillScreen(TFT_BLACK);
  drawBorder();

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(0x8410, TFT_BLACK);
  tft.drawString("TEMPERATURE:", 80, 165, 2);
  tft.drawString("HUMIDITY:", 240, 165, 2);
  tft.drawFastHLine(20, 150, W - 40, 0x4208);
}

void loop() {
  unsigned long now = millis();

  if (now - prevClockMs >= 1000) {
    prevClockMs = now;
    updateClock();
  }

  if (now - prevSensorMs >= 3000) {
    prevSensorMs = now;
    updateSensors();
  }
}
