#ifndef TFT_ESPI_H
#define TFT_ESPI_H

#include <Arduino.h>

// RGB565 color constants
#define TFT_BLACK 0x0000
#define TFT_NAVY 0x000F
#define TFT_DARKGREEN 0x03E0
#define TFT_DARKCYAN 0x03EF
#define TFT_MAROON 0x7800
#define TFT_PURPLE 0x780F
#define TFT_OLIVE 0x7BE0
#define TFT_LIGHTGREY 0xC618
#define TFT_DARKGREY 0x7BEF
#define TFT_BLUE 0x001F
#define TFT_GREEN 0x07E0
#define TFT_CYAN 0x07FF
#define TFT_RED 0xF800
#define TFT_MAGENTA 0xF81F
#define TFT_YELLOW 0xFFE0
#define TFT_WHITE 0xFFFF
#define TFT_ORANGE 0xFD20
#define TFT_GREENYELLOW 0xAFE5
#define TFT_PINK 0xF81F

#define PSRAM_ENABLE 1

class TFT_eSPI {
public:
  TFT_eSPI(int w = 320, int h = 240);
  void begin();
  void setRotation(uint8_t r);
  void fillScreen(uint16_t color);
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

  int width() const { return _w; }
  int height() const { return _h; }

private:
  int _w;
  int _h;
  uint8_t _rotation;
};

class TFT_eSprite {
public:
  explicit TFT_eSprite(TFT_eSPI *tft);
  ~TFT_eSprite();

  void *createSprite(int16_t w, int16_t h);
  void deleteSprite();

  void pushSprite(int32_t x, int32_t y);
  void pushToSprite(TFT_eSprite *dspr, int32_t x, int32_t y);

  void fillSprite(uint16_t color);
  void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color);
  void drawFastHLine(int32_t x, int32_t y, int32_t w, uint16_t color);
  void drawFastVLine(int32_t x, int32_t y, int32_t h, uint16_t color);
  void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color);
  void drawPixel(int32_t x, int32_t y, uint16_t color);

  void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2,
                    int32_t y2, uint16_t color);
  void fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color);
  void drawCircle(int32_t x, int32_t y, int32_t r, uint16_t color);
  void fillEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry,
                   uint16_t color);

  void setTextColor(uint16_t color);
  void setTextColor(uint16_t fg, uint16_t bg);
  void setTextSize(uint8_t size);
  void setCursor(int16_t x, int16_t y);

  void print(const char *str);
  void print(String str);
  void print(int n);
  void print(float n);

  void setColorDepth(int8_t b);
  void setAttribute(uint8_t id, uint8_t a);

private:
  TFT_eSPI *_tft;
  uint16_t *_buf;
  int16_t _w;
  int16_t _h;
  int16_t _cursor_x;
  int16_t _cursor_y;
  uint16_t _text_color;
  uint16_t _text_bgcolor;
  uint8_t _text_size;
  int8_t _color_depth;

  inline bool inBounds(int32_t x, int32_t y) const {
    return (x >= 0 && y >= 0 && x < _w && y < _h);
  }
  inline void putPixelUnsafe(int32_t x, int32_t y, uint16_t c) {
    _buf[y * _w + x] = c;
  }
  void drawGlyphCell(char c);
};

#endif
