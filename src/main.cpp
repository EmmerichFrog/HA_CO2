#include "main.h"

#include "mqtt.h"
#include "read_sensors.h"

TaskHandle_t SensorTask;
TaskHandle_t CommTask;
QueueHandle_t queue;

void reading(void* parameter) {
  std::tuple<uint16_t, uint16_t, float, float> data;
  TwoWire i2c_sensor(0);
  SensirionI2CScd4x scd4x;
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &i2c_sensor, -1);
  setupSerial(i2c_sensor, scd4x, display);
  bool run = true;

  while (run) {
    data = readSensors(scd4x, display);
    xQueueSend(queue, &data, portTICK_PERIOD_MS * 5000);
    delay(2500);
  }
}

void sending(void* parameter) {
  std::tuple<uint16_t, uint16_t, float, float> data;
  uint16_t error;
  uint16_t co2 = 0u;
  float temperature = 0.0f;
  float humidity = 0.0f;
  long lastMsg = 0;
  uint16_t lastCo2 = 0.0f;
  setupWifi();
  PubSubClient& client = setupMQTT();
  bool run = true;

  while (run) {
    if (xQueueReceive(queue, &data, portMAX_DELAY) == pdPASS) {
      if (!client.connected()) {
        reconnect(client);
      }
      std::tie(error, co2, temperature, humidity) = data;
      client.loop();
      if (error == 0) {
        long now = millis();
        if (now - lastMsg > 5000 && abs(co2 - lastCo2) > 50) {
          lastMsg = now;
          // Convert the value to a char array
          char co2Str[8];
          itoa(co2, co2Str, 10);
          Serial.print("Co2 to Str: ");
          Serial.println(co2Str);
          client.publish("esp32/co2", co2Str);
          lastCo2 = co2;
        }
      }
    } else {
      client.loop();
      delay(2500);
    }
  }
}

void setup() {
  Serial.begin(115200);

  /* uncommentm for formatting ffat first time
  delay(10000);
  Serial.println("formatting");
  bool sts = FFat.format();
  Serial.print("Done");
  Serial.println(sts);
  FFat.begin(); */
  queue = xQueueCreate(1, sizeof(std::tuple<uint16_t, uint16_t, float, float>));
  setCpuFrequencyMhz(80);
  xTaskCreate(reading, "Reading", 10000, NULL, 0, &SensorTask);
  xTaskCreate(sending, "MQTT", 20000, NULL, 0, &CommTask);
}

void loop() {}