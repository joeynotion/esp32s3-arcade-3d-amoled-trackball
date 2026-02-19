#pragma once

#include <Arduino.h>
#include <Wire.h>

#define TRACKBALL_I2C_ADDR 0x0A

class Trackball {
public:
  Trackball()
      : _wire(nullptr), _left(0), _right(0), _up(0), _down(0), _sw(0),
        _sw_pressed(false), _sw_released(false) {}

  bool begin(TwoWire &wire = Wire, uint8_t addr = TRACKBALL_I2C_ADDR) {
    _wire = &wire;
    _addr = addr;

    // Check if device is present
    _wire->beginTransmission(_addr);
    uint8_t error = _wire->endTransmission();
    if (error == 0) {
      Serial.println("Trackball found at 0x0A");
      return true;
    } else {
      Serial.printf("Trackball init failed: error %d\n", error);
      return false;
    }
  }

  void setRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    if (!_wire)
      return;
    _wire->beginTransmission(_addr);
    _wire->write(0x00); // LED register
    _wire->write(r);
    _wire->write(g);
    _wire->write(b);
    _wire->write(w);
    _wire->endTransmission();
  }

  bool update() {
    if (!_wire)
      return false;

    _wire->beginTransmission(_addr);
    _wire->write(0x04); // Data register
    _wire->endTransmission();

    if (_wire->requestFrom(_addr, (uint8_t)5) == 5) {
      _left = _wire->read();
      _right = _wire->read();
      _up = _wire->read();
      _down = _wire->read();
      uint8_t sw_state = _wire->read();

      // Detect press/release edges
      _sw_pressed = (sw_state & 0x80) && !(_sw & 0x80);
      _sw_released = !((sw_state & 0x80)) && (_sw & 0x80);
      _sw = sw_state;

      return true;
    } else {
      Serial.println("Trackball read failed");
    }
    return false;
  }

  int8_t left() const { return _left; }
  int8_t right() const { return _right; }
  int8_t up() const { return _up; }
  int8_t down() const { return _down; }
  bool clicked() const { return _sw_pressed; }
  bool released() const { return _sw_released; }
  bool isPressed() const { return (_sw & 0x80) != 0; }

private:
  TwoWire *_wire;
  uint8_t _addr;
  int8_t _left, _right, _up, _down;
  uint8_t _sw;
  bool _sw_pressed, _sw_released;
};
