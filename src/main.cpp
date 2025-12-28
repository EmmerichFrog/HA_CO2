#include "main.h"

#include "mqtt.h"
#include "serial.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Booting up...");
  TaskHandle_t SensorTask;
  TaskHandle_t CommTask;
  QueueHandle_t queue;
  delay(3000);
  LittleFS.begin(true);
  // tuple of error, co2, temperature, humidity coming from sensor
  queue = xQueueCreate(2, sizeof(std::tuple<uint16_t, uint16_t, float, float>));
  xTaskCreate(reading, "Reading", 10000, (void*)queue, 0, &SensorTask);
  xTaskCreate(sending, "MQTT", 24000, (void*)queue, 0, &CommTask);
}

void loop() {}