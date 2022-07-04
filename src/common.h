/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     common.h
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

#ifndef COMMON_H
#define COMMON_H

#include <limits.h>
#include <../d1_mini/pins_arduino.h>

//#define SERIAL_DEBUG

namespace CONFIG
{
  const char POOL_MODEL_NAME[] = "Intex PureSpa SB-H20";
  const char WIFI_VERSION[] = "1.0.2.0"; // 12.06.2022

  // WiFi parameters
  const unsigned long WIFI_MAX_DISCONNECT_DURATION = 900000; // [ms] 5 min until reboot

  // MQTT publish rates
  const unsigned int POOL_UPDATE_PERIOD = 500;           // [ms]
  const unsigned int WIFI_UPDATE_PERIOD = 30000;         // [ms] 30 sec
  const unsigned int FORCED_STATE_UPDATE_PERIOD = 10000; // [ms] 10 sec
}

// Config File Tags
namespace CONFIG_TAG
{
  const char FILENAME[] = "config.json";

  const char WIFI_SSID[] = "wifiSSID";
  const char WIFI_PASSPHRASE[] = "wifiPassphrase";

  const char MQTT_SERVER[] = "mqttServer";
  const char MQTT_USER[] = "mqttUser";
  const char MQTT_PASSWORD[] = "mqttPassword";
  const char MQTT_RETAIN[] = "mqttRetain";
};

// MQTT topics
namespace MQTT_TOPIC
{
  // publish
  const char BUBBLE[] = "pool/bubble";
  const char ERROR[] = "pool/error";
  const char FILTER[] = "pool/filter";
  const char HEATER[] = "pool/heater";
  const char MODEL[] = "pool/model";
  const char POWER[] = "pool/power";
  const char WATER_ACT[] = "pool/water/tempAct";
  const char WATER_SET[] = "pool/water/tempSet";
  const char VERSION[] = "wifi/version";
  const char IP[] = "wifi/ip";
  const char RSSI[] = "wifi/rssi";
  const char WIFI_TEMP[] = "wifi/temp";
  const char STATE[] = "wifi/state";

  // subscribe
  const char CMD_BUBBLE[] = "pool/command/bubble";
  const char CMD_FILTER[] = "pool/command/filter";
  const char CMD_HEATER[] = "pool/command/heater";
  const char CMD_POWER[] = "pool/command/power";
  const char CMD_WATER[] = "pool/command/water/tempSet";
}

// ESP8266 pins
namespace PIN
{
  const uint8_t CLOCK = D5;
  const uint8_t DATA = D6;
  const uint8_t LATCH = D7;
}

// serial debugging
#ifdef SERIAL_DEBUG
#define DEBUG_MSG(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_MSG(...)
#endif

// time delta with overflow support
static unsigned long timeDiff(unsigned long newTime, unsigned long oldTime)
{
  if (newTime >= oldTime)
  {
    return newTime - oldTime;
  }
  else
  {
    return ULONG_MAX - oldTime + newTime + 1;
  }
}

// unsigned int delta with overflow support
static unsigned long diff(unsigned int newVal, unsigned int oldVal)
{
  if (newVal >= oldVal)
  {
    return newVal - oldVal;
  }
  else
  {
    return UINT_MAX - oldVal + newVal + 1;
  }
}

#endif /* COMMON_H */
