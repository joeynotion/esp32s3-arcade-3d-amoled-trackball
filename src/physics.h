#ifndef PHYSICS_H
#define PHYSICS_H

extern float cameraDepth;
extern float playerZdist;
extern float position;
extern float playerX;
extern float speed;
extern float maxSpeed;
extern float centrifugal;
extern bool crashed;
extern unsigned long crashTimer;

extern float currentLapTime;
extern float lastLapTime;
extern float bestLapTime;
extern float prevPosition;
extern int currentLap;
extern int totalLaps;

extern float velocityX;
extern float acceleration;
extern float driftAngle;
extern float manualSteerInput;

void initPhysics();
void handleInput(float dt);
void updatePhysics(float dt);
void checkCollisions();

#endif
