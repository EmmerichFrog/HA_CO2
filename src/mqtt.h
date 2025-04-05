#pragma once
#include <ArduinoJson.h>
#include <FFat.h>
#include <FS.h>
#include <PubSubClient.h>
#include <WiFi.h>

#define DEFAULT_PW "sunisina"
#define SERVER_KEY "mqtt_server"
#define PORT_KEY "mqtt_server"

void setupWifi();
PubSubClient& setupMQTT();
void reconnect(PubSubClient&);
