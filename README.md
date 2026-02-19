# ESP32 Pseudo-3D Racing Game (AMOLED + Trackball Fork)

This repository is a hardware-adapted fork of
[`davidmonterocrespo24/esp32s3-arcade-3d`](https://github.com/davidmonterocrespo24/esp32s3-arcade-3d),
ported to a widescreen AMOLED setup with trackball input.

---

## Features

- Pseudo-3D road rendering with segment-based back-to-front drawing
- 3D player car with textured mesh rendering
- Traffic system with 6 AI vehicles
- Procedural track generation: curves, hills, tunnel, roadside scenery, buildings
- Day / sunset / night cycle with eased transitions
- Atmospheric distance fog and skyline parallax
- Automatic acceleration with speed-aware steering assist
- HUD with speedometer calibrated to 250 km/h top speed
- Trackball-driven steering, press-to-start splash, and press-to-pause

---

## Fork-Specific Changes

- Display target changed to 536x240 AMOLED (landscape)
- Input changed from button steering to Pimoroni trackball over I2C
- Trackball axis rotation and sign compensation added for physical mounting
- Rival vehicle rendering tuned for AMOLED composition
- Startup flow changed to wait on trackball click
- Pause/resume bound to trackball click while in-game
- Night stars masked behind skyline parallax layer

---

## Hardware

| Component | Details |
| --------- | ------- |
| MCU | ESP32-S3 (240 MHz dual-core) |
| Display | 1.91 inch AMOLED, 536x240, RGB565 |
| Input | Pimoroni Trackball Breakout (I2C) |
| I2C Pins | SDA = GPIO40, SCL = GPIO39 |
| PSRAM/Flash | 8 MB PSRAM, 16 MB Flash |

---

## Controls

| Action | Hardware |
| ------ | -------- |
| Steer | Roll trackball left/right |
| Start game | Click trackball on splash |
| Pause / Resume | Click trackball in-game |
| Throttle | Automatic |

---

## Build and Flash (PlatformIO)

### Prerequisites

- VS Code with PlatformIO extension, or PlatformIO Core CLI

### Default environment

- `waveshare_amoled`

### Commands

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

If upload fails on first flash, hold BOOT while connecting/resetting.

---

## Project Structure

```text
src/
|-- main.cpp                # Main loop, startup/pause flow, trackball mapping
|-- config.h                # Tunable gameplay and projection constants
|-- physics.cpp/.h          # Speed, steering assist, gravity, collisions
|-- track.cpp/.h            # Procedural track and traffic initialization
|-- rendering.cpp/.h        # Shared render state, parallax background
|-- render_road.cpp/.h      # Road bands, tunnel, scenery, traffic placement
|-- render_player.cpp/.h    # Player car mesh rendering + splash/crash overlays
|-- render_traffic.cpp/.h   # Rival car geometry rendering
|-- render_building.cpp/.h  # Roadside building rendering
|-- render_hud.cpp/.h       # Speedometer and lap HUD
|-- colors.cpp/.h           # Day/sunset/night palettes and blending
|-- utils.cpp/.h            # Math helpers
|-- drivers/
|   |-- qspi_display.cpp/.h # AMOLED display driver path used by TFT wrapper
|   `-- trackball.h         # Trackball I2C + click state
|-- TFT_eSPI.cpp/.h         # Local display wrapper used by this fork
|-- car2_mesh.h             # Car mesh asset
`-- car2_texture.h          # Car texture asset
```

---

## Key Constants (`src/config.h`)

| Constant | Value | Description |
| -------- | ----- | ----------- |
| `SCR_W` / `SCR_H` | `536 / 240` | AMOLED framebuffer size |
| `TOP_SPEED_KMH` | `250.0f` | HUD top speed calibration |
| `SPEED_MULTIPLIER` | `62.0f` | Physics speed cap scalar |
| `FOG_DENSITY` | `8` | Distance fog intensity |
| `DRAW_DIST` | `40` | Number of projected road segments |
| `ROAD_W` | `2000` | World road width scalar |
