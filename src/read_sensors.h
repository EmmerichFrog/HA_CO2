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

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

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

uint16_t serial0;
uint16_t serial1;
uint16_t serial2;
uint16_t co2R;
String printStr;

float temperature;
float humidity;
uint16_t error;
char errorMessage[256];

uint32_t clockSpeed;

std::tuple<SensirionI2CScd4x&, Adafruit_SSD1306&> setupSerial();

std::tuple<uint16_t, uint16_t, float, float> readSensors(SensirionI2CScd4x&,
                                                         Adafruit_SSD1306&);
