#include "main.h"

#include "mqtt.h"
#include "read_sensors.h"

void reading(void* parameter) {
  std::tuple<uint16_t, uint16_t, float, float> data;
  std::tuple<SensirionI2CScd4x&, Adafruit_SSD1306&> param = setupSerial();
  SensirionI2CScd4x& sensor = std::get<0>(param);
  Adafruit_SSD1306& disp = std::get<1>(param);
  uint8_t commSts = 0;

  for (;;) {
    delay(100);
    data = readSensors(sensor, disp);
    // If no wifi connection is established do not populate queue
    if (commSts == 0) {
      xQueueReceive(queueSts, &commSts, portTICK_PERIOD_MS * 5);
    }
    // Also check for readings error from sensor task
    if (commSts == 1 && std::get<0>(data) == 0) {
      xQueueSend(queueOut, &data, portTICK_PERIOD_MS * 5000);
    }
    delay(2500);
  }
}

void sending(void* parameter) {
  uint8_t commSts = 0;
  std::tuple<uint16_t, uint16_t, float, float> data;
  uint16_t error;
  uint16_t co2 = 0u;
  float temperature = 0.0f;
  float humidity = 0.0f;
  long lastMsg = 0;
  uint16_t lastCo2 = 0.0f;
  xQueueSend(queueSts, &commSts, portMAX_DELAY);
  setupWifi();
  PubSubClient& client = setupMQTT();
  commSts = 1;
  // Send connection status to other task
  xQueueSend(queueSts, &commSts, portMAX_DELAY);
  for (;;) {
    if (xQueueReceive(queueOut, &data, portMAX_DELAY) == pdPASS) {
      if (!client.connected()) {
        commSts = 0;
        reconnect(client);
        commSts = 1;
        xQueueSend(queueSts, &commSts, portMAX_DELAY);
      }
      std::tie(error, co2, temperature, humidity) = data;
      client.loop();
      if (error == 0) {
        long now = millis();
        if (now - lastMsg > 5000 && co2 != lastCo2) {
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
  Serial.print("I2C using pin: SDA ");
  Serial.print(SDA);
  Serial.print(" SCL ");
  Serial.println(SCL);
  /* uncommentm for formatting ffat first time
  delay(10000);
  Serial.println("formatting");
  bool sts = FFat.format();
  Serial.print("Done");
  Serial.println(sts);
  FFat.begin(); */
  queueOut =
      xQueueCreate(5, sizeof(std::tuple<uint16_t, uint16_t, float, float>));
  queueSts = xQueueCreate(2, sizeof(uint8_t));
  setCpuFrequencyMhz(80);
  xTaskCreate(reading, "Reading", 10000, NULL, 0, &Task0);
  xTaskCreate(sending, "MQTT", 20000, NULL, 0, &Task1);
}

void loop() {}