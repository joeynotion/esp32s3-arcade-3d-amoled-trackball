/*
  TRAFFIC CAR RENDERING IMPLEMENTATION
*/

#include "render_traffic.h"
#include "colors.h"
#include "config.h"
#include "rendering.h"


void drawTrafficCar(int cx, int cy, float scale, uint16_t col, int16_t clipY) {
  // Minimum scale check
  if (scale < 0.00018f)
    return;
  if (cy >= SCR_H || cy < 0)
    return;
  if (clipY <= 0)
    return;

  // CAR VERTICES (simplified version of player, always facing forward)
  // Only X and Y are needed — traffic cars never rotate on the Y axis.
  static constexpr float verts[16][2] = {// {X, Y}
                                         // --- LOWER CHASSIS (0-7) ---
                                         {-18, 3},
                                         {18, 3},
                                         {18, 0},
                                         {-18, 0},
                                         {-18, 11},
                                         {18, 11},
                                         {20, 10},
                                         {-20, 10},

                                         // --- CABIN AND WINDOWS (8-15) ---
                                         {-15, 11},
                                         {15, 11},
                                         {17, 11},
                                         {-17, 11},
                                         {-12, 21},
                                         {12, 21},
                                         {12, 20},
                                         {-12, 20}};

  float sx[16], sy[16];

  float projScale = scale * 17.6f; // +10% rival vehicle size

  for (int i = 0; i < 16; i++) {
    sx[i] = cx + (verts[i][0] * projScale * PROJ_X);
    sy[i] = cy - (verts[i][1] * projScale * SCR_CY);
  }

  // Backface Culling
  auto drawFace = [&](int v0, int v1, int v2, int v3, uint16_t faceCol) {
    float cross = (sx[v1] - sx[v0]) * (sy[v2] - sy[v0]) -
                  (sy[v1] - sy[v0]) * (sx[v2] - sx[v0]);
    if (cross > 0) {
      // Avoid drawing faces completely off screen
      float maxY = max(max(sy[v0], sy[v1]), max(sy[v2], sy[v3]));
      float minY = min(min(sy[v0], sy[v1]), min(sy[v2], sy[v3]));
      if (maxY < 0 || minY > SCR_H)
        return;
      drawQuad(sx[v0], sy[v0], sx[v1], sy[v1], sx[v2], sy[v2], sx[v3], sy[v3],
               faceCol);
    }
  };

  // Colors based on car color
  uint16_t hoodCol = col;
  uint16_t bodyCol = darkenCol(col, 0.85);
  uint16_t darkCol = darkenCol(col, 0.65);
  uint16_t glassCol = rgb(80, 180, 255);
  uint16_t grillCol = rgb(30, 30, 30);

  // --- DRAW ORDER (Back to front for this angle) ---

  // Rear first (farthest from camera)
  drawFace(6, 7, 3, 2, darkCol);      // Rear Bumper
  drawFace(14, 15, 11, 10, grillCol); // Rear Window

  // Sides and roof
  drawFace(7, 6, 5, 4, hoodCol); // Top Cover
  drawFace(7, 4, 0, 3, bodyCol); // Left Side
  drawFace(5, 6, 2, 1, bodyCol); // Right Side

  // Cabin
  drawFace(15, 14, 13, 12, hoodCol); // Roof
  drawFace(13, 14, 10, 9, bodyCol);  // Right Door
  drawFace(15, 12, 8, 11, bodyCol);  // Left Door
  drawFace(12, 13, 9, 8, glassCol);  // Windshield

  // Front (closest to player camera)
  drawFace(0, 1, 2, 3, darkCol);  // Chassis Base
  drawFace(4, 5, 1, 0, grillCol); // Front Grille
}
