#include "main.h"

#include "mqtt.h"
#include "serial.h"

void setup() {
  Serial.begin(115200);
  TaskHandle_t SensorTask;
  TaskHandle_t CommTask;
  QueueHandle_t queue;

  LittleFS.begin(true);
  queue = xQueueCreate(2, sizeof(std::tuple<uint16_t, uint16_t, float, float>));
  setCpuFrequencyMhz(80);
  xTaskCreate(reading, "Reading", 10000, (void*)queue, 0, &SensorTask);
  xTaskCreate(sending, "MQTT", 20000, (void*)queue, 0, &CommTask);
}

void loop() {}