/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     MQTTPublisher.cpp
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

#include "MQTTPublisher.h"

#include "MQTTClient.h"
#include "SBH20IO.h"
#include "NTCThermometer.h"
#include "common.h"


MQTTPublisher::MQTTPublisher(MQTTClient& mqttClient, SBH20IO& poolIO, NTCThermometer& thermometer) :
  mqttClient(mqttClient),
  poolIO(poolIO),
  thermometer(thermometer)
{
}

void MQTTPublisher::publishIfDefined(const char* topic, uint8 b, uint8 undef)
{
  if (b != undef)
  {
    mqttClient.publish(topic, b? "on" : "off");
  }
}

void MQTTPublisher::publishIfDefined(const char* topic, int i, int undef)
{
  if (i != undef)
  {
     publish(topic, i);
  }
}

void MQTTPublisher::publishIfDefined(const char* topic, uint16 u, uint16 undef)
{
  if (u != undef)
  {
     publish(topic, u);
  }
}

void MQTTPublisher::publish(const char* topic, int i)
{
  snprintf(buf, BUFFER_SIZE, "%d", i);
  mqttClient.publish(topic, buf);
}

void MQTTPublisher::publish(const char* topic, unsigned int u)
{
  snprintf(buf, BUFFER_SIZE, "%u", u);
  mqttClient.publish(topic, buf);
}

void MQTTPublisher::publishTemp(const char* topic, float t)
{
  snprintf(buf, BUFFER_SIZE, "%.0f", t);
  if (t >= -60 && t <= 145)
  {
    DEBUG_MSG("controller temperature: %s °C\n", buf);
    mqttClient.publish(topic, buf);
  }
  else
  {
    DEBUG_MSG("controller temperature: error (%s °C)\n", buf);
    mqttClient.publish(topic, "error");
  }
}

/**
 * publish changed topics with rate limit
 * except topic 'wifi/state' that is force published ever 10 seconds
 */
void MQTTPublisher::loop()
{
  unsigned long now = millis();
  if (timeDiff(now, poolUpdateTime) >= CONFIG::POOL_UPDATE_PERIOD)
  {
    poolUpdateTime = now;

    bool forcedStateUpdate = false;
    if (timeDiff(now, poolStateUpdateTime) >= CONFIG::FORCED_STATE_UPDATE_PERIOD)
    {
      poolStateUpdateTime = now;
      forcedStateUpdate = true;
    }

    if (poolIO.isOnline())
    {
      publishIfDefined(MQTT_TOPIC::BUBBLE, poolIO.isBubbleOn(), SBH20IO::UNDEF::BOOL);
      publishIfDefined(MQTT_TOPIC::FILTER, poolIO.isFilterOn(), SBH20IO::UNDEF::BOOL);
      publishIfDefined(MQTT_TOPIC::POWER,  poolIO.isPowerOn(),  SBH20IO::UNDEF::BOOL);

      bool b = poolIO.isHeaterOn();
      if (b != SBH20IO::UNDEF::BOOL)
      {
        mqttClient.publish(MQTT_TOPIC::HEATER, b? (poolIO.isHeaterStandby()? "standby" : "on") : "off");
      }

      publishIfDefined(MQTT_TOPIC::WATER_ACT, poolIO.getActWaterTempCelsius(),     (int)SBH20IO::UNDEF::USHORT);
      publishIfDefined(MQTT_TOPIC::WATER_SET, poolIO.getDesiredWaterTempCelsius(), (int)SBH20IO::UNDEF::USHORT);

#ifdef SERIAL_DEBUG
      publishIfDefined("pool/telegram/led", poolIO.getRawLedValue(), SBH20IO::UNDEF::USHORT);
#endif

      unsigned int errorVal = poolIO.getErrorValue();
      if (errorVal == 0)
      {
        mqttClient.publish(MQTT_TOPIC::STATE, "online", forcedStateUpdate);
      }
      else
      {
        mqttClient.publish(MQTT_TOPIC::STATE, "error", forcedStateUpdate);
      }
      mqttClient.publish(MQTT_TOPIC::ERROR, poolIO.getErrorMessage(errorVal).c_str());
    }
    else
    {
      mqttClient.publish(MQTT_TOPIC::STATE, "offline", forcedStateUpdate);
    }

    // update WiFi controller temperature and RSSI
    if (timeDiff(now, wifiStateUpdateTime) >= CONFIG::WIFI_UPDATE_PERIOD)
    {
      wifiStateUpdateTime = now;

      // get temperature of WiFi controller
      publishTemp(MQTT_TOPIC::WIFI_TEMP, thermometer.getTemperature());

      // get WiFi RSSI
      publish(MQTT_TOPIC::RSSI, WiFi.RSSI());

#ifdef SERIAL_DEBUG
      publish("wifi/heap", ESP.getFreeHeap());
#endif
    }
  }
}
