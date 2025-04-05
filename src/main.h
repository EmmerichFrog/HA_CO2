#pragma once
#include <Arduino.h>

TaskHandle_t Task0;
TaskHandle_t Task1;
TaskHandle_t flash;
QueueHandle_t queueOut;
QueueHandle_t queueSts;
