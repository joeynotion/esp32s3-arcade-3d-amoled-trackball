#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "TFT_eSPI.h"
#include "drivers/trackball.h"

#include "colors.h"
#include "config.h"
#include "physics.h"
#include "rendering.h"
#include "structs.h"
#include "track.h"
#include "utils.h"

// Board trackball pins
#define TRACKBALL_I2C_SDA 40
#define TRACKBALL_I2C_SCL 39

int timeOfDay = 0; // 0=day, 1=sunset, 2=night
long distSinceTimeChange = 0;

static Trackball trackball;
static float steerAccumulator = 0.0f;
static uint32_t lastFrameUs = 0; // High precision timer
static bool dayTransitionActive = false;
static int dayFrom = 0;
static int dayTo = 1;
static float dayTransitionT = 0.0f;
static bool gameStarted = false;
static bool gamePaused = false;

// Trackball mount compensation:
// rotate input vector 90 degrees to match physical orientation.
static constexpr bool TRACKBALL_ROTATE_90 = true;
static constexpr bool TRACKBALL_ROTATE_CW = true;
static constexpr int TRACKBALL_STEER_SIGN = -1;

static constexpr float DAY_TRANSITION_SECONDS = 4.0f;
static constexpr int TIME_CHANGE_DISTANCE = 180000;

void setup() {
  Serial.begin(115200);

  randomSeed((uint32_t)micros());
  Wire.begin(TRACKBALL_I2C_SDA, TRACKBALL_I2C_SCL);
  if (trackball.begin(Wire)) {
    trackball.setRGBW(0, 40, 120, 0);
  }

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  spr.setColorDepth(16);
  spr.setAttribute(PSRAM_ENABLE, true);
  if (spr.createSprite(SCR_W, SCR_H) == nullptr) {
    Serial.println("ERROR: sprite allocation failed");
  }

  initPhysics();

  Serial.print("Total PSRAM: ");
  Serial.println(ESP.getPsramSize());
  Serial.print("Free PSRAM:  ");
  Serial.println(ESP.getFreePsram());

  initColors(timeOfDay);
  initBackground();
  buildTrack();
  initTraffic(maxSpeed);
  Serial.println("=== TRAFFIC INIT ===");
  for (int i = 0; i < MAX_CARS; i++) {
    Serial.printf("Car %d: z=%.0f offset=%.2f speed=%.0f color=0x%04X\n", i,
                  trafficCars[i].z, trafficCars[i].offset, trafficCars[i].speed,
                  trafficCars[i].color);
  }

  while (true) {
    float animTime = millis() * 0.001f;
    drawStartScreen(animTime);
    if (trackball.update() && trackball.clicked()) {
      break;
    }
    delay(16);
  }

  lastFrameUs = micros();
  distSinceTimeChange = 0;
  dayTransitionActive = false;
  dayFrom = timeOfDay;
  dayTo = (timeOfDay + 1) % 3;
  dayTransitionT = 0.0f;
  gameStarted = true;
  gamePaused = false;

  // Flush any pending movement from startup interaction.
  trackball.update();
  steerAccumulator = 0.0f;
  manualSteerInput = 0.0f;
}

void loop() {
  uint32_t nowUs = micros();
  float dt = (float)(nowUs - lastFrameUs) / 1000000.0f;
  lastFrameUs = nowUs;

  // Stability clamp
  if (dt <= 0.0f)
    dt = 0.001f;
  if (dt > 0.1f)
    dt = 0.1f;

  bool trackballOk = trackball.update();
  bool buttonClicked = trackballOk && trackball.clicked();
  int16_t raw_dx = 0;
  if (trackballOk) {
    int16_t axisX = trackball.right() - trackball.left();
    int16_t axisY = trackball.down() - trackball.up();

    if (TRACKBALL_ROTATE_90) {
      axisX = TRACKBALL_ROTATE_CW ? axisY : -axisY;
    }
    raw_dx = axisX * TRACKBALL_STEER_SIGN;
  }

  if (!gameStarted) {
    float animTime = millis() * 0.001f;
    drawStartScreen(animTime);
    if ((millis() / 400) % 2 == 0) {
      spr.setTextSize(1);
      spr.setTextColor(TFT_CYAN);
      spr.setCursor(SCR_CX - 73, 206);
      spr.print("PRESS TRACKBALL TO START");
      spr.pushSprite(0, 0);
    }
    if (buttonClicked) {
      gameStarted = true;
      gamePaused = false;
      steerAccumulator = 0.0f;
      manualSteerInput = 0.0f;
      lastFrameUs = micros();
    }
    delay(16);
    return;
  }

  if (buttonClicked) {
    gamePaused = !gamePaused;
    steerAccumulator = 0.0f;
    manualSteerInput = 0.0f;
    lastFrameUs = nowUs;
  }

  if (gamePaused) {
    drawSky(position, playerZdist, timeOfDay, skyOffset);
    drawRoad(position, playerX, playerZdist, cameraDepth, timeOfDay);
    drawPlayerCar();
    drawHUD(speed, maxSpeed, currentLapTime, bestLapTime);
    spr.fillRect(SCR_CX - 90, SCR_CY - 24, 180, 48, TFT_BLACK);
    spr.drawRect(SCR_CX - 91, SCR_CY - 25, 182, 50, TFT_YELLOW);
    spr.setTextSize(2);
    spr.setTextColor(TFT_YELLOW, TFT_BLACK);
    spr.setCursor(SCR_CX - 36, SCR_CY - 16);
    spr.print("PAUSED");
    spr.setTextSize(1);
    spr.setCursor(SCR_CX - 68, SCR_CY + 7);
    spr.print("Click trackball to resume");
    spr.pushSprite(0, 0);
    delay(16);
    return;
  }

  if (raw_dx >= -1 && raw_dx <= 1) {
    raw_dx = 0;
  }

  // Trackball sensitivity/response tuned for faster recentering.
  float targetSteer = clampF(raw_dx * 0.42f, -1.0f, 1.0f);
  steerAccumulator = steerAccumulator * 0.30f + targetSteer * 0.70f;
  if (!trackballOk) {
    steerAccumulator *= 0.85f;
  }

  // Click recenters steering quickly.
  if (trackball.clicked() || trackball.isPressed()) {
    steerAccumulator *= 0.45f;
  }
  if (steerAccumulator > -0.02f && steerAccumulator < 0.02f) {
    steerAccumulator = 0.0f;
  }
  manualSteerInput = steerAccumulator;

  if (!crashed) {
    handleInput(dt);
  }

  // updatePhysics now handles its own internal "crashed" early return for
  // player logic, while still updating traffic regardless.
  updatePhysics(dt);

  if (!crashed) {
    checkCollisions();

    int pSeg = findSegIdx(position + playerZdist);
    float curveForce = segments[pSeg].curve;
    skyOffset += curveForce * (speed / maxSpeed) * 150.0f * dt;
  }

  drawSky(position, playerZdist, timeOfDay, skyOffset);
  drawRoad(position, playerX, playerZdist, cameraDepth, timeOfDay);
  drawPlayerCar();
  drawHUD(speed, maxSpeed, currentLapTime, bestLapTime);

  if (crashed) {
    drawCrashMessage();
    if (millis() - crashTimer > 2000) {
      crashed = false;
      speed = 0;
      playerX = 0;
    }
  }

  spr.pushSprite(0, 0);
  delay(2); // Cap frame rate to ~500fps for physics stability

  distSinceTimeChange += (int)(speed * dt);
  if (!dayTransitionActive && distSinceTimeChange > TIME_CHANGE_DISTANCE) {
    distSinceTimeChange = 0;
    dayTransitionActive = true;
    dayFrom = timeOfDay;
    dayTo = (timeOfDay + 1) % 3;
    dayTransitionT = 0.0f;
  }

  if (dayTransitionActive) {
    dayTransitionT += dt / DAY_TRANSITION_SECONDS;
    float t = dayTransitionT;
    if (t > 1.0f) {
      t = 1.0f;
    }
    // Smooth cosine easing avoids visible snapping.
    float eased = -cosf(t * PI) * 0.5f + 0.5f;
    blendColors(dayFrom, dayTo, eased);
    if (dayTransitionT >= 1.0f) {
      dayTransitionActive = false;
      timeOfDay = dayTo;
      initColors(timeOfDay);
    }
  }
}
