#ifndef CONFIG_H
#define CONFIG_H

#define SCR_W 536
#define SCR_H 240
#define SCR_CX (SCR_W / 2)
#define SCR_CY (SCR_H / 2)

// Horizontal projection scale for widescreen tuning.
#define PROJ_X_SCALE 0.68f
#define PROJ_X ((float)SCR_CX * PROJ_X_SCALE)
#define UI_SAFE_MARGIN 10

#define SEG_LEN 200
#define RUMBLE_LEN 3
#define DRAW_DIST 40
#define TOTAL_SEGS 200
#define ROAD_W 2000
#define LANES 3
#define FOV_DEG 92
#define CAM_HEIGHT 1000
#define FOG_DENSITY 8

#define RANDOM_TRACK 1

#define TOP_SPEED_KMH 250.0f

#define SPEED_MULTIPLIER 62.0f
#define ACCEL_TARGET 0.72f
#define ACCEL_RAMP 110.0f
#define ACCEL_NEAR_MAX 0.90f
#define ACCEL_DAMPING 0.97f
#define FRICTION 0.996f
#define GRAVITY_FACTOR 1600.0f
#define CENTRIFUGAL 0.18f
#define CURVE_FORCE 3.0f
#define LATERAL_FRICTION 0.90f
#define STEER_AUTO 1.8f
#define CENTRIFUGAL_DX 1.5f

#define BUILDING_H_MIN 120000
#define BUILDING_H_MAX 350000
#define BUILDING_W 400000
#define BUILDING_OFFSET 1.5f
#define BUILDING_SEG_MIN 6
#define BUILDING_SEG_MAX 16
#define BUILDING_GAP_MIN 10
#define BUILDING_GAP_MAX 20

#define MAX_CARS 6

#endif
