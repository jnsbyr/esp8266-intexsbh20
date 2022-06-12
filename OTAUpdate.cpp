/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     OTAUpdate.cpp
 *
 * encoding: UTF-8
 * created:  27th March 2021
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

#include "OTAUpdate.h"

#include <ESP8266httpUpdate.h>
#include "MQTTClient.h"
#include "common.h"


bool OTAUpdate::start(const char* updateURL, MQTTClient& mqttClient)
{
  bool success = false;

  mqttClient.publish(MQTT_TOPIC::OTA, F("in progress"), false, true);

  // try to perform OTA update
  WiFiClient client;
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, updateURL, CONFIG::WIFI_VERSION);
  const unsigned int BUFFER_SIZE = 128;
  char buf[BUFFER_SIZE];
  switch (ret)
  {
    case HTTP_UPDATE_FAILED:
      snprintf_P(buf, BUFFER_SIZE, PSTR("failed: %s (error code %d)"), ESPhttpUpdate.getLastErrorString().c_str(), ESPhttpUpdate.getLastError());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      snprintf_P(buf, BUFFER_SIZE, PSTR("none available"));
      break;

    case HTTP_UPDATE_OK:
      snprintf_P(buf, BUFFER_SIZE, PSTR("success"));
      success = true;
      break;

    default:
      snprintf_P(buf, BUFFER_SIZE, PSTR("unknown result: %d"), ret);
  }
  mqttClient.publish(MQTT_TOPIC::OTA, buf, false, true);

  return success;
}
