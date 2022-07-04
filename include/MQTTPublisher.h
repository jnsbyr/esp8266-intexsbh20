/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     MQTTPublisher.h
 *
 * encoding: UTF-8
 * created:  15th March 2021
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

#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <stdint.h>

class MQTTClient;
class SBH20IO;
class NTCThermometer;

class MQTTPublisher
{
public:
  MQTTPublisher(MQTTClient &mqttClient, SBH20IO &poolIO, NTCThermometer &thermometer);

public:
  void setRetainAll(bool retain);
  bool isRetainAll() const;

public:
  void loop();

private:
  static const unsigned int BUFFER_SIZE = 16;

private:
  void publish(const char *topic, int i);
  void publish(const char *topic, unsigned int u);

  void publishIfDefined(const char *topic, uint8_t b, uint8_t undef);
  void publishIfDefined(const char *topic, uint16_t i, uint16_t undef);
  void publishIfDefined(const char *topic, int i, int undef);

  void publishTemp(const char *topic, float t);

private:
  MQTTClient &mqttClient;
  SBH20IO &poolIO;
  NTCThermometer &thermometer;
  bool retainAll;

private:
  unsigned long poolUpdateTime = 0;
  unsigned long poolStateUpdateTime = 0;
  unsigned long wifiStateUpdateTime = 0;
  char buf[BUFFER_SIZE];
};

#endif /* MQTT_PUBLISHER_H */
