#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <SensirionCore.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

#include <tuple>

#define TEMP_OFFSET 4
#define SCL 5
#define SDA 6

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define FIRST_COL 0
#define SECOND_COL 36
#define THIRD_COL 84
#define FOURTH_COL 112
#define FIFTH_COL 100
#define FIRST_ROW 0
#define SECOND_ROW 16
#define THIRD_ROW 26
#define FOURTH_ROW 36
#define FIFTH_ROW 46

#define READ_FAIL -1

void setupSerial(TwoWire& i2c_sensor, SensirionI2CScd4x& scd4x,
                 Adafruit_SSD1306& display);

std::tuple<uint16_t, uint16_t, float, float> readSensors(SensirionI2CScd4x&,
                                                         Adafruit_SSD1306&);
