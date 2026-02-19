#include "TFT_eSPI.h"

#include "drivers/qspi_display.h"

#include <esp_heap_caps.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// 5x7 bitmap font — ASCII 32..126, column-major (5 bytes per glyph)
static const uint8_t font5x7[][5] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32  (space)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33  !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34  "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35  #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36  $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37  %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38  &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39  '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40  (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41  )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // 42  *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43  +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44  ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45  -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46  .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47  /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48  0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49  1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50  2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51  3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52  4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53  5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54  6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55  7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56  8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57  9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58  :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59  ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // 60  <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61  =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // 62  >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63  ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64  @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65  A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66  B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67  C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68  D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69  E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // 70  F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // 71  G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72  H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73  I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74  J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75  K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76  L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // 77  M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78  N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79  O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80  P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81  Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82  R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83  S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84  T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85  U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86  V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 87  W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88  X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // 89  Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90  Z
    {0x00, 0x00, 0x7F, 0x41, 0x41}, // 91  [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92  (backslash)
    {0x41, 0x41, 0x7F, 0x00, 0x00}, // 93  ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94  ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95  _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96  `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97  a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98  b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99  c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // 103 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
    {0x00, 0x7F, 0x10, 0x28, 0x44}, // 107 k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}, // 126 ~
};

TFT_eSPI::TFT_eSPI(int w, int h) : _w(w), _h(h), _rotation(0) {}

void TFT_eSPI::begin() {
  static bool inited = false;
  if (!inited) {
    inited = lcd.begin();
    if (inited) {
      lcd.setBrightness(200);
    }
  }
}

void TFT_eSPI::setRotation(uint8_t r) { _rotation = r; }

void TFT_eSPI::fillScreen(uint16_t color) {
  lcd.setWindow(0, 0, _w, _h);
  lcd.pushColor(color, (uint32_t)_w * (uint32_t)_h);
}

uint16_t TFT_eSPI::color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

TFT_eSprite::TFT_eSprite(TFT_eSPI *tft)
    : _tft(tft), _buf(nullptr), _w(0), _h(0), _cursor_x(0), _cursor_y(0),
      _text_color(TFT_WHITE), _text_bgcolor(TFT_BLACK), _text_size(1),
      _color_depth(16) {}

TFT_eSprite::~TFT_eSprite() { deleteSprite(); }

void *TFT_eSprite::createSprite(int16_t w, int16_t h) {
  deleteSprite();
  if (w <= 0 || h <= 0) {
    return nullptr;
  }

  size_t bytes = (size_t)w * (size_t)h * sizeof(uint16_t);
  _buf = (uint16_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
  if (!_buf) {
    _buf = (uint16_t *)heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL);
  }
  if (!_buf) {
    return nullptr;
  }

  _w = w;
  _h = h;
  memset(_buf, 0, bytes);
  return _buf;
}

void TFT_eSprite::deleteSprite() {
  if (_buf) {
    heap_caps_free(_buf);
    _buf = nullptr;
  }
  _w = 0;
  _h = 0;
}

void TFT_eSprite::pushSprite(int32_t x, int32_t y) {
  if (!_buf || _w <= 0 || _h <= 0) {
    return;
  }

  int32_t src_x0 = 0;
  int32_t src_y0 = 0;
  int32_t draw_w = _w;
  int32_t draw_h = _h;

  if (x < 0) {
    src_x0 = -x;
    draw_w -= src_x0;
    x = 0;
  }
  if (y < 0) {
    src_y0 = -y;
    draw_h -= src_y0;
    y = 0;
  }

  int32_t max_w = LCD_WIDTH - x;
  int32_t max_h = LCD_HEIGHT - y;
  if (draw_w > max_w) {
    draw_w = max_w;
  }
  if (draw_h > max_h) {
    draw_h = max_h;
  }

  if (draw_w <= 0 || draw_h <= 0) {
    return;
  }

  if (src_x0 == 0 && draw_w == _w) {
    lcd.setWindow((uint16_t)x, (uint16_t)y, (uint16_t)draw_w, (uint16_t)draw_h);
    lcd.pushPixels(_buf + src_y0 * _w, (uint32_t)draw_w * (uint32_t)draw_h);
    return;
  }

  uint16_t *row = (uint16_t *)heap_caps_malloc(
      (size_t)draw_w * sizeof(uint16_t), MALLOC_CAP_INTERNAL);
  if (!row) {
    return;
  }

  for (int32_t r = 0; r < draw_h; r++) {
    const uint16_t *src = _buf + (src_y0 + r) * _w + src_x0;
    memcpy(row, src, (size_t)draw_w * sizeof(uint16_t));
    lcd.setWindow((uint16_t)x, (uint16_t)(y + r), (uint16_t)draw_w, 1);
    lcd.pushPixels(row, (uint32_t)draw_w);
  }

  heap_caps_free(row);
}

void TFT_eSprite::pushToSprite(TFT_eSprite *dspr, int32_t x, int32_t y) {
  if (!_buf || !dspr || !dspr->_buf) {
    return;
  }

  int32_t src_x0 = 0;
  int32_t src_y0 = 0;
  int32_t dst_x0 = x;
  int32_t dst_y0 = y;
  int32_t copy_w = _w;
  int32_t copy_h = _h;

  if (dst_x0 < 0) {
    src_x0 = -dst_x0;
    copy_w -= src_x0;
    dst_x0 = 0;
  }
  if (dst_y0 < 0) {
    src_y0 = -dst_y0;
    copy_h -= src_y0;
    dst_y0 = 0;
  }

  if (dst_x0 + copy_w > dspr->_w) {
    copy_w = dspr->_w - dst_x0;
  }
  if (dst_y0 + copy_h > dspr->_h) {
    copy_h = dspr->_h - dst_y0;
  }

  if (copy_w <= 0 || copy_h <= 0) {
    return;
  }

  for (int32_t row = 0; row < copy_h; row++) {
    const uint16_t *src = _buf + (src_y0 + row) * _w + src_x0;
    uint16_t *dst = dspr->_buf + (dst_y0 + row) * dspr->_w + dst_x0;
    memcpy(dst, src, (size_t)copy_w * sizeof(uint16_t));
  }
}

void TFT_eSprite::fillSprite(uint16_t color) {
  if (!_buf) {
    return;
  }
  size_t bytes = (size_t)_w * (size_t)_h * sizeof(uint16_t);
  if (color == 0) {
    memset(_buf, 0, bytes);
  } else {
    size_t count = (size_t)_w * (size_t)_h;
    for (size_t i = 0; i < count; i++) {
      _buf[i] = color;
    }
  }
}

void TFT_eSprite::fillRect(int32_t x, int32_t y, int32_t w, int32_t h,
                           uint16_t color) {
  if (!_buf || w <= 0 || h <= 0) {
    return;
  }

  int32_t x0 = x;
  int32_t y0 = y;
  int32_t x1 = x + w;
  int32_t y1 = y + h;

  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 > _w)
    x1 = _w;
  if (y1 > _h)
    y1 = _h;

  if (x0 >= x1 || y0 >= y1) {
    return;
  }

  for (int32_t yy = y0; yy < y1; yy++) {
    uint16_t *row = _buf + yy * _w + x0;
    for (int32_t xx = x0; xx < x1; xx++) {
      *row++ = color;
    }
  }
}

void TFT_eSprite::drawFastVLine(int32_t x, int32_t y, int32_t h,
                                uint16_t color) {
  if (!_buf || h <= 0 || x < 0 || x >= _w) {
    return;
  }
  int32_t y0 = y;
  int32_t y1 = y + h;
  if (y0 < 0)
    y0 = 0;
  if (y1 > _h)
    y1 = _h;
  if (y0 >= y1)
    return;

  for (int32_t yy = y0; yy < y1; yy++) {
    _buf[yy * _w + x] = color;
  }
}

void TFT_eSprite::drawRect(int32_t x, int32_t y, int32_t w, int32_t h,
                           uint16_t color) {
  if (w <= 0 || h <= 0) {
    return;
  }
  drawFastHLine(x, y, w, color);
  drawFastHLine(x, y + h - 1, w, color);
  drawFastVLine(x, y, h, color);
  drawFastVLine(x + w - 1, y, h, color);
}

void TFT_eSprite::drawFastHLine(int32_t x, int32_t y, int32_t w,
                                uint16_t color) {
  if (!_buf || w <= 0 || y < 0 || y >= _h) {
    return;
  }
  int32_t x0 = x;
  int32_t x1 = x + w;
  if (x0 < 0)
    x0 = 0;
  if (x1 > _w)
    x1 = _w;
  if (x0 >= x1) {
    return;
  }

  uint16_t *row = _buf + y * _w + x0;
  for (int32_t i = x0; i < x1; i++) {
    *row++ = color;
  }
}

void TFT_eSprite::drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                           uint16_t color) {
  if (!_buf) {
    return;
  }

  int32_t dx = abs(x1 - x0);
  int32_t sx = x0 < x1 ? 1 : -1;
  int32_t dy = -abs(y1 - y0);
  int32_t sy = y0 < y1 ? 1 : -1;
  int32_t err = dx + dy;

  while (true) {
    if (inBounds(x0, y0)) {
      putPixelUnsafe(x0, y0, color);
    }
    if (x0 == x1 && y0 == y1) {
      break;
    }
    int32_t e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void TFT_eSprite::drawPixel(int32_t x, int32_t y, uint16_t color) {
  if (!_buf || !inBounds(x, y)) {
    return;
  }
  putPixelUnsafe(x, y, color);
}

void TFT_eSprite::fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                               int32_t x2, int32_t y2, uint16_t color) {
  if (!_buf) {
    return;
  }

  // Sort vertices by Y: (x0,y0) top, (x1,y1) mid, (x2,y2) bottom
  if (y0 > y1) {
    int32_t t;
    t = x0;
    x0 = x1;
    x1 = t;
    t = y0;
    y0 = y1;
    y1 = t;
  }
  if (y0 > y2) {
    int32_t t;
    t = x0;
    x0 = x2;
    x2 = t;
    t = y0;
    y0 = y2;
    y2 = t;
  }
  if (y1 > y2) {
    int32_t t;
    t = x1;
    x1 = x2;
    x2 = t;
    t = y1;
    y1 = y2;
    y2 = t;
  }

  int32_t total_h = y2 - y0;
  if (total_h == 0) {
    // Degenerate: horizontal line
    int32_t lx = min(x0, min(x1, x2));
    int32_t rx = max(x0, max(x1, x2));
    drawFastHLine(lx, y0, rx - lx + 1, color);
    return;
  }

  for (int32_t y = y0; y <= y2; y++) {
    if (y < 0)
      continue;
    if (y >= _h)
      break;

    // Long edge: always (x0,y0)→(x2,y2)
    int32_t xa = x0 + (int32_t)((int64_t)(x2 - x0) * (y - y0) / total_h);

    // Short edge: upper half or lower half
    int32_t xb;
    if (y < y1) {
      int32_t seg_h = y1 - y0;
      xb = (seg_h == 0) ? x0
                        : x0 + (int32_t)((int64_t)(x1 - x0) * (y - y0) / seg_h);
    } else {
      int32_t seg_h = y2 - y1;
      xb = (seg_h == 0) ? x1
                        : x1 + (int32_t)((int64_t)(x2 - x1) * (y - y1) / seg_h);
    }

    if (xa > xb) {
      int32_t t = xa;
      xa = xb;
      xb = t;
    }
    drawFastHLine(xa, y, xb - xa + 1, color);
  }
}

void TFT_eSprite::fillCircle(int32_t x, int32_t y, int32_t r, uint16_t color) {
  if (!_buf || r <= 0) {
    return;
  }

  for (int32_t yy = -r; yy <= r; yy++) {
    int32_t xx = (int32_t)sqrtf((float)(r * r - yy * yy));
    drawFastHLine(x - xx, y + yy, 2 * xx + 1, color);
  }
}

void TFT_eSprite::drawCircle(int32_t x0, int32_t y0, int32_t r,
                             uint16_t color) {
  if (!_buf || r <= 0) {
    return;
  }

  int32_t x = r;
  int32_t y = 0;
  int32_t err = 0;

  while (x >= y) {
    drawPixel(x0 + x, y0 + y, color);
    drawPixel(x0 + y, y0 + x, color);
    drawPixel(x0 - y, y0 + x, color);
    drawPixel(x0 - x, y0 + y, color);
    drawPixel(x0 - x, y0 - y, color);
    drawPixel(x0 - y, y0 - x, color);
    drawPixel(x0 + y, y0 - x, color);
    drawPixel(x0 + x, y0 - y, color);

    y++;
    if (err <= 0) {
      err += 2 * y + 1;
    } else {
      x--;
      err += 2 * (y - x) + 1;
    }
  }
}

void TFT_eSprite::fillEllipse(int32_t x, int32_t y, int32_t rx, int32_t ry,
                              uint16_t color) {
  if (!_buf || rx <= 0 || ry <= 0) {
    return;
  }

  for (int32_t yy = -ry; yy <= ry; yy++) {
    float t = 1.0f - ((float)(yy * yy) / (float)(ry * ry));
    if (t < 0.0f) {
      continue;
    }
    int32_t xx = (int32_t)(rx * sqrtf(t));
    drawFastHLine(x - xx, y + yy, 2 * xx + 1, color);
  }
}

void TFT_eSprite::setTextColor(uint16_t color) { _text_color = color; }

void TFT_eSprite::setTextColor(uint16_t fg, uint16_t bg) {
  _text_color = fg;
  _text_bgcolor = bg;
}

void TFT_eSprite::setTextSize(uint8_t size) {
  _text_size = size > 0 ? size : 1;
}

void TFT_eSprite::setCursor(int16_t x, int16_t y) {
  _cursor_x = x;
  _cursor_y = y;
}

void TFT_eSprite::drawGlyphCell(char c) {
  int cw = 6 * _text_size;
  int ch = 8 * _text_size;

  // Clear cell background
  fillRect(_cursor_x, _cursor_y, cw, ch, _text_bgcolor);

  // Render glyph if in printable range
  if (c >= 32 && c <= 126) {
    int idx = c - 32;
    for (int col = 0; col < 5; col++) {
      uint8_t bits = font5x7[idx][col];
      for (int row = 0; row < 7; row++) {
        if (bits & (1 << row)) {
          if (_text_size == 1) {
            drawPixel(_cursor_x + col, _cursor_y + row, _text_color);
          } else {
            fillRect(_cursor_x + col * _text_size, _cursor_y + row * _text_size,
                     _text_size, _text_size, _text_color);
          }
        }
      }
    }
  }

  _cursor_x += cw;
}

void TFT_eSprite::print(const char *str) {
  if (!str) {
    return;
  }
  while (*str) {
    if (*str == '\n') {
      _cursor_x = 0;
      _cursor_y += 8 * _text_size;
    } else {
      drawGlyphCell(*str);
    }
    str++;
  }
}

void TFT_eSprite::print(String str) { print(str.c_str()); }

void TFT_eSprite::print(int n) {
  char b[24];
  snprintf(b, sizeof(b), "%d", n);
  print(b);
}

void TFT_eSprite::print(float n) {
  char b[32];
  dtostrf(n, 0, 2, b);
  print(b);
}

void TFT_eSprite::setColorDepth(int8_t b) { _color_depth = b; }

void TFT_eSprite::setAttribute(uint8_t id, uint8_t a) {
  (void)id;
  (void)a;
}
