/*
 * project:  generic
 *
 * file:     MQTTClient.cpp
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

#include "MQTTClient.h"

#include "common.h"


/**
 * MQTT subscription received callback
 */
void MQTTClient::subscriptionUpdate(char* topic, byte* message, unsigned int length)
{
  message[length] = '\0';

  // find subscription for topic
  auto i = boolSubscriber.find(topic);
  if (i != boolSubscriber.end())
  {
    // decode payload as bool (on/off) and pass to subscriber
    bool on = strcmp("on", (char*)message) == 0;
    DEBUG_MSG("set %s: %d\n", topic, on);
    i->second(on);
  }
  else
  {
    auto i = intSubscriber.find(topic);
    if (i != intSubscriber.end())
    {
      // decode payload as int and pass to subscriber
      int value = atoi((char*)message);
      DEBUG_MSG("set %s: %d\n", topic, value);
      i->second(value);
    }
  }

  // remove last transmitted state topic and the will topic to force retransmission
  String t = topic;
  String c = "command/";
  int p = t.indexOf(c);
  if (p >= 0)
  {
    t.remove(p, c.length());
    publications.erase(t);
  }
  publications.erase(willTopic);
}

void MQTTClient::setup(const char* mqttServer, uint16 mqttPort, const char* mqttUsername, const char* mqttPassword, const char* cid, const char* wt, const char* wm)
{
  mqttuser = mqttUsername;
  mqttpw = mqttPassword;
  clientId = cid;
  willTopic = wt;
  willMessage = wm;

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(std::bind(&MQTTClient::subscriptionUpdate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void MQTTClient::reconnect()
{
  if (!mqttClient.connected() && timeDiff(now, lastConnectTime) > RECONNECT_DELAY)
  {
    // not connected, try to reconnect
    Serial.print("trying to connect to MQTT server ... ");
    if (mqttClient.connect(clientId, mqttuser, mqttpw, willTopic, MQTTQOS0, true, willMessage))
    {
      // connected
      Serial.println("success");

      // publish metadata
      if (!lastConnectTime)
      {
        for (auto m: metadata)
        {
          mqttClient.publish(m.first.c_str(), m.second.c_str(), true);
        }
      }

      // resubscribe
      for (auto s: boolSubscriber)
      {
        mqttClient.subscribe(s.first.c_str());
      }
      for (auto s: intSubscriber)
      {
        mqttClient.subscribe(s.first.c_str());
      }
    } else {
      Serial.printf("failed, rc=%d\n", mqttClient.state());
    }
    lastConnectTime = now;
  }
}

void MQTTClient::loop()
{
  now = millis();

  // reconnect
  if (!mqttClient.connected()) {
    reconnect();
  }

  // receive subscription updates
  mqttClient.loop();
}

void MQTTClient::addMetadata(const char* topic, const char* message)
{
  metadata[topic] = message;
}

void MQTTClient::addSubscriber(const char* topic, void (*setter)(bool value))
{
  boolSubscriber[topic] = setter;
}

void MQTTClient::addSubscriber(const char* topic, void (*setter)(int value))
{
  intSubscriber[topic] = setter;
}

bool MQTTClient::isConnected()
{
  return mqttClient.connected();
}

/**
 * publish on change of payload
 *
 * @param topic
 * @param payload
 * @param retain
 * @param force
 * @return true if published
 */
bool MQTTClient::publish(const char* topic, const String& message, bool retain, bool force)
{
  if (lastConnectTime)
  {
    // payload change detection
    auto i = publications.find(topic);
    bool changed;
    if (i != publications.end())
    {
      changed = !i->second.equals(message);
    }
    else
    {
      changed = true;
    }

    // publish on change
    if (mqttClient.connected() && (changed || force))
    {
      bool published = mqttClient.publish(topic, message.c_str(), retain);
      if (published && changed)
      {
        publications[topic] = message;
      }
      return published;
    }
  }

  return false;
}
