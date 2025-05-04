#include "read_sensors.h"

uint32_t clockSpeed;

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void setupSerial(TwoWire& i2c_sensor, SensirionI2CScd4x& scd4x,
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
  clockSpeed = getCpuFrequencyMhz();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("CO2 Sensor");
  display.display();
  uint16_t error;
  scd4x.begin(i2c_sensor);
  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    printSerialNumber(serial0, serial1, serial2);
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
    SensirionI2CScd4x& scd4x, Adafruit_SSD1306& display) {
  char errorMessage[256];
  String printStr;
  uint16_t co2R;
  float temperature;
  float humidity;
  uint16_t error;

  printStr = "";
  bool isDataReady = false;
  error = scd4x.getDataReadyFlag(isDataReady);
  if (error) {
    Serial.print("error trying to execute getDataReadyFlag(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    return std::make_tuple(error, 0, 0.0f, 0.0f);
  }
  if (!isDataReady) {
    return std::make_tuple(READ_FAIL, 0, 0.0f, 0.0f);
  }
  error = scd4x.readMeasurement(co2R, temperature, humidity);
  if (error) {
    Serial.print("error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else if (co2R == 0) {
    Serial.println("Invalid sample detected, skipping.");
  }

  printStr.concat("eco2R: ");
  printStr.concat(co2R);
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
  display.print("co2 Sensor");
  display.setCursor(FIRST_COL, SECOND_ROW);
  display.print("co2");
  display.setCursor(SECOND_COL, SECOND_ROW);
  display.print("Temp.");
  display.setCursor(THIRD_COL, SECOND_ROW);
  display.print("Hum.");
  display.setCursor(FIRST_COL, THIRD_ROW);
  display.print(co2R);
  display.setCursor(SECOND_COL, THIRD_ROW);
  display.print(temperature);
  display.setCursor(THIRD_COL, THIRD_ROW);
  display.print(humidity);
  display.setCursor(FIRST_COL, FIFTH_ROW);
  display.print("CPU MHz:");
  display.setCursor(THIRD_COL, FIFTH_ROW);
  display.print(clockSpeed);
  display.display();

  return std::make_tuple(error, co2R, temperature, humidity);
}
