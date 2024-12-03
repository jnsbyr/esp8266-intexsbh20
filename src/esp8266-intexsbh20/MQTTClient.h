/*
 * project:  generic
 *
 * file:     MQTTClient.h
 *
 * encoding: UTF-8
 * created:  13th March 2021
 *
 * Copyright (C) 2021 Jens B.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <functional>
#include <map>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>


/**
 * see https://github.com/knolleary/pubsubclient
 */
class MQTTClient
{
public:
  MQTTClient() : mqttClient(wifiClient) {}
  MQTTClient(WiFiClient *client) : mqttClient(*client) {
    wifiClient = *client;
  }


public:
  void addMetadata(const char* topic, const char* message);
  void addSubscriber(const char* topic, void (*setter)(bool value));
  void addSubscriber(const char* topic, void (*setter)(int value));

  void setup(const char* mqttServer, uint16 mqttPort, const char* mqttUsername, const char* mqttPassword,const char* clientId, const char* willTopic, const char* willMessage);
  bool check_connect(const char* mqttServer, uint16 mqttPort, const char* mqttUsername, const char* mqttPassword, const char* cid, const char* wt, const char* wm);

  void loop();

  bool isConnected();
  bool publish(const char* topic, const String& payload, bool retain=false, bool force=false);

private:
  static const unsigned int RECONNECT_DELAY = 3000; // [ms]

private:
  // PubSubClient callback
  void subscriptionUpdate(char* topic, byte* message, unsigned int length);

private:
  void reconnect();

private:
  PubSubClient mqttClient;
  WiFiClient wifiClient;
  const char* clientId = NULL;;
  const char* willTopic = NULL;
  const char* willMessage= NULL;
  const char* mqttuser = NULL;
  const char* mqttpw= NULL;

private:
  std::map<String, String> metadata;
  std::map<String, String> publications;

  std::map<String, std::function<void (bool)>> boolSubscriber;
  std::map<String, std::function<void (int)>> intSubscriber;

private:
  unsigned int now = 0;
  unsigned int lastConnectTime = 0;
};

#endif /* MQTT_CLIENT_H */
