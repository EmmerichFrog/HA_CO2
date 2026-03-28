#include "serial.h"

#include "mqtt.h"

extern uint32_t lastMsg;
extern uint8_t commSts;
extern const uint32_t MIN_PUB_RATE;

void printSerial(uint64_t& value) {
  Serial.print("0x");
  Serial.print((uint32_t)(value >> 32), HEX);
  Serial.print((uint32_t)(value & 0xFFFFFFFF), HEX);
}

void setupSerial(TwoWire& i2c_sensor, SensirionI2cScd4x& scd4x,
                 Adafruit_SSD1306& display) {
  char errorMessage[256];
  Serial.print("I2C using pin: SDA ");
  Serial.print(SDA);
  Serial.print(" SCL ");
  Serial.println(SCL);
  i2c_sensor.begin(SDA, SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("CO2 Sensor");
  display.display();
  uint16_t error;
  scd4x.begin(i2c_sensor, SCD40_I2C_ADDR_62);
  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  scd4x.setTemperatureOffset(6.f);
  scd4x.setSensorAltitude(566.f);
  uint64_t serial;
  error = scd4x.getSerialNumber(serial);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    printSerial(serial);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  Serial.println("Waiting for first measurement... (5 sec)");
  display.clearDisplay();
  display.setCursor(FIRST_COL, SECOND_ROW);
  display.print("Waiting 5 seconds for sensor...");
}

std::tuple<uint16_t, uint16_t, float, float> readSensors(
    SensirionI2cScd4x& scd4x, Adafruit_SSD1306& display) {
  char errorMessage[256];
  String printStr;
  uint16_t co2;
  float temperature;
  float humidity;
  uint16_t error;

  printStr = "";
  bool isDataReady = false;
  error = scd4x.getDataReadyStatus(isDataReady);
  if (error) {
    Serial.print("error trying to execute getDataReadyStatus(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    return std::make_tuple(error, 0, 0.0f, 0.0f);
  }
  if (!isDataReady) {
    return std::make_tuple(READ_FAIL, 0, 0.0f, 0.0f);
  }
  error = scd4x.readMeasurement(co2, temperature, humidity);
  if (error) {
    Serial.print("error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else if (co2 == 0) {
    Serial.println("Invalid sample detected, skipping.");
  }

  printStr.concat("eco2: ");
  printStr.concat(co2);
  printStr.concat("ppm");
  printStr.concat("\n");
  printStr.concat("T: ");
  printStr.concat(temperature);
  printStr.concat("°C");
  printStr.concat("\n");
  printStr.concat("Hum: ");
  printStr.concat(humidity);
  printStr.concat("%");
  printStr.concat("\n");
  Serial.print(printStr);
  display.clearDisplay();
  display.setCursor(FIRST_COL, FIRST_ROW);
  display.print("Comm. Sts:");
  display.setCursor(THIRD_COL, FIRST_ROW);
  display.print((commSts = COMM_OK) ? "OK" : "KO");
  display.setCursor(FIRST_COL, SECOND_ROW);
  display.print("co2");
  display.setCursor(SECOND_COL, SECOND_ROW);
  display.print("Temp.");
  display.setCursor(THIRD_COL, SECOND_ROW);
  display.print("Hum.");
  display.setCursor(FIRST_COL, THIRD_ROW);
  display.print(co2);
  display.setCursor(SECOND_COL, THIRD_ROW);
  display.print(temperature);
  display.setCursor(THIRD_COL, THIRD_ROW);
  display.print(humidity);
  display.setCursor(FIRST_COL, FIFTH_ROW);
  display.print("Last pub [s]:");
  display.setCursor(THIRD_COL, FIFTH_ROW);
  display.print((millis() - lastMsg) / 1000);
  display.display();

  if (millis() - lastMsg > MIN_PUB_RATE * 1.05) {
    esp_restart();
  }

  return std::make_tuple(error, co2, temperature, humidity);
}

void reading(void* parameter) {
  QueueHandle_t queue = (QueueHandle_t)parameter;
  std::tuple<uint16_t, uint16_t, float, float> data;
  TwoWire i2c_sensor(0);
  SensirionI2cScd4x scd4x;
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &i2c_sensor, -1);
  setupSerial(i2c_sensor, scd4x, display);
  bool run = true;

  while (run) {
    data = readSensors(scd4x, display);
    xQueueSend(queue, &data, portTICK_PERIOD_MS * 5000);
    delay(2500);
  }
}