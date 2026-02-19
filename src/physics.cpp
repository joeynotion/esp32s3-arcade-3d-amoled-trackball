#include "physics.h"

#include <Arduino.h>

#include "config.h"
#include "track.h"
#include "utils.h"

float cameraDepth;
float playerZdist;
float position = 0;
float playerX = 0;
float speed = 0;
float maxSpeed;
float centrifugal = CENTRIFUGAL;

bool crashed = false;
unsigned long crashTimer = 0;
unsigned long lastFrameMs;

float currentLapTime = 0;
float lastLapTime = 0;
float bestLapTime = 0;
float prevPosition = 0;
int currentLap = 1;
int totalLaps = 3;

float velocityX = 0;
float acceleration = 0;
float driftAngle = 0;
float manualSteerInput = 0;

static constexpr float LAUNCH_SPEED_PCT = 0.10f;
static constexpr float LAUNCH_ACCEL_PCT = 0.14f;
static constexpr float LOW_SPEED_GRAVITY_ATTEN = 0.18f;
static constexpr float AUTO_STEER_TARGET_SCALE = 0.12f; // Reference behavior.
static constexpr float AUTO_STEER_BLEND = 0.55f;        // Keep assist gentle.
static constexpr float AUTO_STEER_CURVE_BOOST = 0.55f;  // Extra assist on hard turns.

void initPhysics() {
  float fovRad = FOV_DEG * PI / 180.0f;
  cameraDepth = 1.0f / tanf(fovRad / 2.0f);
  playerZdist = CAM_HEIGHT * cameraDepth;
  maxSpeed = SEG_LEN * SPEED_MULTIPLIER;

  position = 0;
  playerX = 0;
  speed = 0;
  crashed = false;
  currentLapTime = 0;
  lastLapTime = 0;
  bestLapTime = 0;
  prevPosition = 0;

  velocityX = 0;
  acceleration = 0;
  driftAngle = 0;
  manualSteerInput = 0;
}

void handleInput(float dt) {
  float targetAccel = maxSpeed * ACCEL_TARGET;
  float accelSpeed = ACCEL_RAMP;

  if (speed < maxSpeed * ACCEL_NEAR_MAX) {
    acceleration += accelSpeed * dt;
    if (acceleration > targetAccel) {
      acceleration = targetAccel;
    }
  } else {
    acceleration *= ACCEL_DAMPING;
  }

  // Primary steering from trackball.
  playerX += manualSteerInput * 2.6f * dt;

  // Gentle auto-steer assist from the original implementation:
  // follow road curvature, but reduce assist when the player is actively steering.
  int pSeg = findSegIdx(position + playerZdist);
  float curveMag = fabsf(segments[pSeg].curve);
  float curveNorm = clampF((curveMag - 2.0f) / 5.5f, 0.0f, 1.0f);
  float autoTarget =
      -segments[pSeg].curve * AUTO_STEER_TARGET_SCALE * (1.0f + 0.20f * curveNorm);
  float speedAssist = clampF(speed / (maxSpeed * 0.45f), 0.0f, 1.0f);
  float manualAssist = 1.0f - clampF(fabsf(manualSteerInput) * 1.25f, 0.0f, 1.0f);
  float centerAssist = 1.0f - clampF(fabsf(playerX) / 1.35f, 0.0f, 1.0f);
  float assistGain = STEER_AUTO * AUTO_STEER_BLEND * (0.35f + centerAssist * 0.65f);
  assistGain *= 1.0f + AUTO_STEER_CURVE_BOOST * curveNorm;
  playerX += (autoTarget - playerX) * assistGain * speedAssist * manualAssist * dt;

  // Light recentering to avoid long-term offset drift.
  playerX += (-playerX) * 0.18f * dt;

  // Manual steering does NOT dampen here, as it's governed by the trackball
  // momentum in main.cpp

  playerX = clampF(playerX, -1.45f, 1.45f);
}

void updatePhysics(float dt) {
  // Update traffic regardless of player crash state
  for (int i = 0; i < MAX_CARS; i++) {
    trafficCars[i].z =
        loopIncrease(trafficCars[i].z, dt * trafficCars[i].speed, trackLength);
    if (random(0, 200) < 2) {
      trafficCars[i].offset += (random(-1, 2)) * 0.1f;
      trafficCars[i].offset = clampF(trafficCars[i].offset, -0.8f, 0.8f);
    }
  }

  if (crashed)
    return;

  int pSeg = findSegIdx(position + playerZdist);
  int prevSegIdx = (pSeg - 1 + TOTAL_SEGS) % TOTAL_SEGS;

  float spPct = speed / maxSpeed;

  float currentY = segments[pSeg].y;
  float prevY = segments[prevSegIdx].y;
  float slope = (currentY - prevY) / SEG_LEN;

  float gravityEffect = -slope * GRAVITY_FACTOR;
  if (speed < maxSpeed * LAUNCH_SPEED_PCT && gravityEffect < 0.0f) {
    gravityEffect *= LOW_SPEED_GRAVITY_ATTEN;
  }
  acceleration += gravityEffect * dt;

  float minLaunchAccel = maxSpeed * LAUNCH_ACCEL_PCT;
  if (speed < maxSpeed * LAUNCH_SPEED_PCT && acceleration < minLaunchAccel) {
    acceleration = minLaunchAccel;
  }

  speed *= FRICTION;
  speed += acceleration * dt;

  if (speed > maxSpeed) {
    speed = maxSpeed;
  }
  if (speed < 0) {
    speed = 0;
  }

  float curveForce = segments[pSeg].curve * centrifugal * spPct;
  velocityX += curveForce * dt * CURVE_FORCE;
  velocityX *= LATERAL_FRICTION;
  playerX -= velocityX * dt;

  float steerDx = dt * CENTRIFUGAL_DX * spPct;
  playerX -= steerDx * spPct * segments[pSeg].curve * centrifugal;
  playerX = clampF(playerX, -1.6f, 1.6f);

  if (speed > 0.1f) {
    driftAngle = atan2f(velocityX * 10.0f, speed / maxSpeed) * 0.5f;
    driftAngle = clampF(driftAngle, -0.5f, 0.5f);
  } else {
    driftAngle *= 0.9f;
  }

  prevPosition = position;
  position = loopIncrease(position, dt * speed, trackLength);

  if (position < prevPosition && prevPosition > trackLength * 0.9f) {
    if (currentLapTime > 5.0f) {
      lastLapTime = currentLapTime;
      if (bestLapTime <= 0 || currentLapTime < bestLapTime) {
        bestLapTime = currentLapTime;
      }

      if (currentLap < totalLaps) {
        currentLap++;
      } else {
        currentLap = 1;
      }
    }
    currentLapTime = 0;
  }
  currentLapTime += dt;
}

void checkCollisions() {
  const float playerW = 0.15f;
  int pSeg = findSegIdx(position + playerZdist);
  Segment &s = segments[pSeg];

  for (int i = 0; i < MAX_CARS; i++) {
    int cs = findSegIdx(trafficCars[i].z);
    int d = abs(cs - pSeg);
    if (d > 3 && d < TOTAL_SEGS - 3) {
      continue;
    }
    if (speed > trafficCars[i].speed &&
        overlapChk(playerX, playerW, trafficCars[i].offset, 0.15f)) {
      speed = trafficCars[i].speed * 0.7f;
      position = loopIncrease(position, -(speed * 0.05f), trackLength);
      if (speed > maxSpeed * 0.5f) {
        crashed = true;
        crashTimer = millis();
      }
    }
  }

  if (s.tunnel) {
    const float wallX = 0.95f;
    if (playerX < -wallX || playerX > wallX) {
      playerX = clampF(playerX, -wallX, wallX);
      velocityX = 0.0f;
      speed *= 0.3f;
      if (speed > maxSpeed * 0.35f) {
        crashed = true;
        crashTimer = millis();
      }
    }
  }

  if (playerX < -1.0f || playerX > 1.0f) {
    if (s.spriteType >= 0 &&
        overlapChk(playerX, playerW, s.spriteOffset, 0.4f)) {
      speed *= 0.2f;
      if (speed > maxSpeed * 0.25f) {
        crashed = true;
        crashTimer = millis();
      }
    }
  }

  if (playerX <= -1.55f || playerX >= 1.55f) {
    playerX = clampF(playerX, -1.45f, 1.45f);
    velocityX = 0.0f;
    if (speed > maxSpeed * 0.18f) {
      speed *= 0.93f;
    }
    if (speed > maxSpeed * 0.55f) {
      crashed = true;
      crashTimer = millis();
    }
  }
}
