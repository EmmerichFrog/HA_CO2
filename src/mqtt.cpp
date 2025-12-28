#include "mqtt.h"

#include <WiFiManager.h>

uint32_t lastMsg = 0;
uint8_t commSts = COMM_NA;

// Default settings for mqtt
char mqtt_server[64] = "192.168.2.99";
char mqtt_port[64] = "1883";

WiFiClient espClient;
PubSubClient client(espClient);

void saveConfigCallback() {
  Serial.println("saving config");
  JsonDocument json;

  json[SERVER_KEY] = mqtt_server;
  json[PORT_KEY] = mqtt_port;

  File configFile = LittleFS.open("/config.json", FILE_WRITE, true);
  if (!configFile) {
    while (true) {
      Serial.println("failed to open config file for writing");
    }
  }

  serializeJson(json, Serial);
  Serial.println();
  serializeJson(json, configFile);

  configFile.close();
}

void loadConfig() {
  if (LittleFS.exists("/config.json")) {
    Serial.println("reading config file");
    File configFile = LittleFS.open("/config.json", FILE_READ);
    if (configFile) {
      Serial.println("opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);

      JsonDocument json;
      auto deserializeError = deserializeJson(json, buf.get());
      // serializeJson(json, Serial);
      if (!deserializeError) {
        Serial.println("parsed json");
        const char* ip = json[SERVER_KEY];
        strcpy(mqtt_server, ip);
        const char* port = json[PORT_KEY];
        strcpy(mqtt_port, port);
      } else {
        Serial.println("failed to load json config");
      }
      configFile.close();
    }
  } else {
    Serial.println("config file not present!");
  }
}

void setupWifi() {
  loadConfig();

  Serial.println("Loaded from file: ");
  Serial.println("\tmqtt_server : " + String(mqtt_server));
  Serial.println("\tmqtt_port : " + String(mqtt_port) + "\n");
  // value id/name placeholder/prompt/default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server,
                                          sizeof(mqtt_server));
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port,
                                        sizeof(mqtt_port));
  WiFiManager wm;
  // wm.resetSettings();
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  bool res = wm.autoConnect("AutoConnectAP_CO2", DEFAULT_PW);

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
}

PubSubClient& setupMQTT() {
  client.setServer(mqtt_server, (uint16_t)atoi(mqtt_port));
  return client;
}

void reconnect(PubSubClient& client) {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...\n");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("esp32/output");
      commSts = COMM_OK;
    } else {
      commSts = COMM_KO;
      Serial.println("MQTT connection failed, rc=");
      Serial.print(client.state());
      Serial.print(" - ip: " + String(mqtt_server) + ":" + String(mqtt_port));
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void sending(void* parameter) {
  QueueHandle_t queue = (QueueHandle_t)parameter;
  std::tuple<uint16_t, uint16_t, float, float> data;
  uint16_t error;
  uint16_t co2 = 0u;
  float temperature = 0.0f;
  float humidity = 0.0f;
  uint16_t lastCo2 = 0.0f;
  setupWifi();
  PubSubClient& client = setupMQTT();
  const uint32_t MIN_PUB_RATE = 5 * 60 * 1000;
  const uint16_t MIN_PUB_DELTA = 50;

  bool run = true;
  // Cpu freq. will be limited to 80 once all setup is finished and changed
  // every time we need to publish
  setCpuFrequencyMhz(80);
  while (run) {
    if (xQueueReceive(queue, &data, portMAX_DELAY) == pdPASS) {
      setCpuFrequencyMhz(240);
      if (!client.connected()) {
        reconnect(client);
      }
      std::tie(error, co2, temperature, humidity) = data;
      client.loop();
      if (error == 0) {
        uint32_t now = millis();
        if (now - lastMsg > MIN_PUB_RATE ||
            abs(co2 - lastCo2) > MIN_PUB_DELTA) {
          lastMsg = now;
          char co2Str[8];
          itoa(co2, co2Str, 10);
          Serial.print("Co2 to Str: ");
          Serial.println(co2Str);
          client.publish("esp32/co2", co2Str);
          lastCo2 = co2;
        }
      }
      setCpuFrequencyMhz(80);
    }
  }
}
