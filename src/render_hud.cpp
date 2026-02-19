#include "render_hud.h"

#include "colors.h"
#include "config.h"
#include "physics.h"
#include "rendering.h"

void drawSpeedometer(float speed, float maxSpeed) {
  int centerX = SCR_W - 68;
  int centerY = SCR_H - 62;
  int radius = 48;

  spr.fillCircle(centerX, centerY, radius + 3, rgb(20, 20, 20));
  spr.drawCircle(centerX, centerY, radius + 3, TFT_DARKGREY);
  spr.drawCircle(centerX, centerY, radius + 2, rgb(60, 60, 60));

  int kmh = (int)(speed * TOP_SPEED_KMH / maxSpeed);

  for (int i = 0; i <= 5; i++) {
    float angle = (-225 + i * (270.0f / 5.0f)) * PI / 180.0f;
    int x1 = centerX + cos(angle) * (radius - 8);
    int y1 = centerY + sin(angle) * (radius - 8);
    int x2 = centerX + cos(angle) * (radius - 2);
    int y2 = centerY + sin(angle) * (radius - 2);

    uint16_t markColor = (i >= 4) ? TFT_RED : TFT_ORANGE;
    spr.drawLine(x1, y1, x2, y2, markColor);

    int num = i * 50;
    int tx = centerX + cos(angle) * (radius - 20);
    int ty = centerY + sin(angle) * (radius - 20);
    spr.setTextSize(1);
    spr.setTextColor(TFT_WHITE, rgb(20, 20, 20));
    spr.setCursor(tx - 6, ty - 4);
    spr.print(num);
  }

  float needleAngle = (-225 + (kmh / TOP_SPEED_KMH) * 270.0f) * PI / 180.0f;
  int needleX = centerX + cos(needleAngle) * (radius - 10);
  int needleY = centerY + sin(needleAngle) * (radius - 10);

  spr.drawLine(centerX + 1, centerY + 1, needleX + 1, needleY + 1,
               rgb(10, 10, 10));

  uint16_t needleColor =
      (kmh > (int)(TOP_SPEED_KMH - 20.0f))
          ? TFT_RED
          : (kmh > (int)(TOP_SPEED_KMH * 0.72f)) ? TFT_YELLOW : TFT_WHITE;
  spr.drawLine(centerX, centerY, needleX, needleY, needleColor);
  spr.drawLine(centerX - 1, centerY, needleX - 1, needleY, needleColor);
  spr.drawLine(centerX, centerY - 1, needleX, needleY - 1, needleColor);

  spr.fillCircle(centerX, centerY, 4, needleColor);
  spr.drawCircle(centerX, centerY, 5, TFT_DARKGREY);

  spr.setTextSize(2);
  spr.setTextColor(needleColor, rgb(20, 20, 20));
  spr.setCursor(centerX - 18, centerY + 14);
  if (kmh < 100) {
    spr.print(" ");
  }
  if (kmh < 10) {
    spr.print(" ");
  }
  spr.print(kmh);
}

void drawHUD(float speed, float maxSpeed, float currentLapTime, float bestLapTime) {
  int panelX = UI_SAFE_MARGIN;
  int panelY = UI_SAFE_MARGIN;
  int panelW = 128;
  int panelH = 42;

  spr.fillRect(panelX, panelY, panelW, panelH, TFT_BLACK);
  spr.drawRect(panelX, panelY, panelW, panelH, TFT_RED);
  spr.drawRect(panelX + 1, panelY + 1, panelW - 2, panelH - 2, TFT_DARKGREY);

  spr.setTextSize(2);
  spr.setTextColor(TFT_RED, TFT_BLACK);
  spr.setCursor(panelX + 5, panelY + 4);
  spr.print("LAP ");
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.print(currentLap);
  spr.print("/");
  spr.print(totalLaps);

  spr.setTextSize(1);
  spr.setTextColor(TFT_YELLOW, TFT_BLACK);
  spr.setCursor(panelX + 5, panelY + 24);
  spr.print("TIME ");
  spr.setTextColor(TFT_WHITE, TFT_BLACK);

  int mins = (int)currentLapTime / 60;
  int secs = (int)currentLapTime % 60;
  int decimals = (int)((currentLapTime - (int)currentLapTime) * 100);
  if (mins > 0) {
    spr.print(mins);
    spr.print(":");
    if (secs < 10) {
      spr.print("0");
    }
  }
  spr.print(secs);
  spr.print(".");
  if (decimals < 10) {
    spr.print("0");
  }
  spr.print(decimals);

  if (bestLapTime > 0 && bestLapTime < 999) {
    spr.setCursor(SCR_W - 138, UI_SAFE_MARGIN + 2);
    spr.setTextColor(TFT_GREEN, TFT_BLACK);
    spr.print("BEST ");
    spr.print((int)bestLapTime);
    spr.print(".");
    spr.print((int)((bestLapTime - (int)bestLapTime) * 10));
    spr.print(" ");
  }

  drawSpeedometer(speed, maxSpeed);
}
