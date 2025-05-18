#pragma once
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "main.h"

#define DEFAULT_PW "sunisina"
#define SERVER_KEY "mqtt_server"
#define PORT_KEY "mqtt_server"

enum CommSts {
  COMM_NA = 0,
  COMM_OK = 1,
  COMM_KO = 2,
};
void sending(void* parameter);
