#include "mqtt.h"

#include <WiFiManager.h>

uint32_t lastMsg = 0;
uint8_t commSts = COMM_NA;

// Default settings for mqtt
char mqtt_server[64] = "192.168.2.205";
char mqtt_port[6] = "1883";

WiFiClient espClient;
PubSubClient client(espClient);
bool needSave = false;
// callback notifying us of the need to save config
void saveConfigCallback() { needSave = true; }

void setupWifi() {
  Serial.println("mounted file system");
  if (FFat.exists("/config.json")) {
    // file exists, reading and loading
    Serial.println("reading config file");
    File configFile = FFat.open("/config.json", FILE_READ);
    if (configFile) {
      Serial.println("opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);

      JsonDocument json;
      auto deserializeError = deserializeJson(json, buf.get());
      serializeJson(json, Serial);
      if (!deserializeError) {
        Serial.println("\nparsed json");
        strcpy(mqtt_server, json[SERVER_KEY]);
        strcpy(mqtt_port, json[PORT_KEY]);
      } else {
        Serial.println("failed to load json config");
      }
      configFile.close();
    }
  }
  // end read

  // value id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server,
                                          64);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);

  WiFiManager wm;
  // wm.resetSettings();
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  bool res = wm.autoConnect("AutoConnectAP_CO2", DEFAULT_PW);

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  Serial.println("Loaded from file: ");
  Serial.println("\tmqtt_server : " + String(mqtt_server));
  Serial.println("\tmqtt_port : " + String(mqtt_port));

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  if (needSave) {
    Serial.println("saving config");
    JsonDocument json;

    json[SERVER_KEY] = mqtt_server;
    json[PORT_KEY] = mqtt_port;

    File configFile = FFat.open("/config.json", FILE_WRITE, true);
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    serializeJson(json, Serial);
    Serial.println();
    serializeJson(json, configFile);

    configFile.close();
  }
}

PubSubClient& setupMQTT() {
  client.setServer(mqtt_server, (uint16_t)atoi(mqtt_port));
  return client;
}

void reconnect(PubSubClient& client) {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("esp32/output");
      commSts = COMM_OK;
    } else {
      commSts = COMM_KO;
      Serial.print("failed, rc=");
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
  bool run = true;

  while (run) {
    if (xQueueReceive(queue, &data, portMAX_DELAY) == pdPASS) {
      if (!client.connected()) {
        reconnect(client);
      }
      std::tie(error, co2, temperature, humidity) = data;
      client.loop();
      if (error == 0) {
        uint32_t now = millis();
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
