/*
 * project:  Intex PureSpa WiFi Controller
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

#include <climits>
#include <c_types.h>
#include <../d1_mini/pins_arduino.h>

/*****************************************************************************

                       C O N F I G U R A T I O N

 *****************************************************************************/

// select Intex PureSpa model by commenting in the desired variant
// A) PureSpa SB-H20, PureSpa SSP-H-20-1 and SimpleSpa SB–B20
//#define MODEL_SB_H20
// B) PureSpa SJB-HS
//#define MODEL_SJB_HS

// define a custom model name to be reported via MQTT
//#define CUSTOM_MODEL_NAME "Intex PureSpa"

// if changing the water temperature setpoint does not work reliably, commenting
// in the following option may improve the behaviour at the cost of a short
// disconnect to the MQTT server
//#define FORCE_WIFI_SLEEP

//#define SERIAL_DEBUG

/*****************************************************************************/

namespace CONFIG
{
  const char WIFI_VERSION[] = "1.0.8.2"; // 04.03.2025

  // WiFi parameters
  const unsigned long WIFI_MAX_DISCONNECT_DURATION = 900000; // [ms] 5 min until reboot

  // MQTT publish rates
  const unsigned int  POOL_UPDATE_PERIOD           =    500; // [ms]
  const unsigned int  WIFI_UPDATE_PERIOD           =  30000; // [ms] 30 sec
  const unsigned int  FORCED_STATE_UPDATE_PERIOD   =  10000; // [ms] 10 sec
}

// Config File Tags
namespace CONFIG_TAG
{
  const char FILENAME[]        = "config.json";

  const char WIFI_SSID[]       = "wifiSSID";
  const char WIFI_PASSPHRASE[] = "wifiPassphrase";
  const char WIFI_OTA_URL[]    = "firmwareURL";

  const char MQTT_SERVER[]     = "mqttServer";
  const char MQTT_PORT[]       = "mqttPort";
  const char MQTT_USER[]       = "mqttUser";
  const char MQTT_PASSWORD[]   = "mqttPassword";
  const char MQTT_RETAIN[]     = "mqttRetain";
  const char MQTT_ERROR_LANG[] = "errorLanguage";
};

// MQTT topics
namespace MQTT_TOPIC
{
  // publish
  const char BUBBLE[]       = "pool/bubble";
  const char DISINFECTION[] = "pool/disinfection"; // SJB-HS only
  const char ERROR[]        = "pool/error";
  const char FILTER[]       = "pool/filter";
  const char HEATER[]       = "pool/heater";
  const char JET[]          = "pool/jet"; // SJB-HS only
  const char MODEL[]        = "pool/model";
  const char POWER[]        = "pool/power";
  const char WATER_ACT[]    = "pool/water/tempAct";
  const char WATER_SET[]    = "pool/water/tempSet";
  const char VERSION[]      = "wifi/version";
  const char IP[]           = "wifi/ip";
  const char RSSI[]         = "wifi/rssi";
  const char WIFI_TEMP[]    = "wifi/temp";
  const char STATE[]        = "wifi/state";
  const char OTA[]          = "wifi/update";

  // subscribe
  const char CMD_BUBBLE[]       = "pool/command/bubble";
  const char CMD_DISINFECTION[] = "pool/command/disinfection"; // SJB-HS only
  const char CMD_FILTER[]       = "pool/command/filter";
  const char CMD_HEATER[]       = "pool/command/heater";
  const char CMD_JET[]          = "pool/command/jet"; // SJB-HS only
  const char CMD_POWER[]        = "pool/command/power";
  const char CMD_WATER[]        = "pool/command/water/tempSet";
  const char CMD_OTA[]          = "wifi/command/update";
}

// Languages
enum class LANG
{
  CODE = 0, EN = 1, DE = 2
};

// ESP8266 pins
namespace PIN
{
  const uint8 CLOCK = D5;
  const uint8 DATA  = D6;
  const uint8 LATCH = D7;
}

// serial debugging
#ifdef SERIAL_DEBUG
#define DEBUG_MSG(...) Serial.printf( __VA_ARGS__ )
#else
#define DEBUG_MSG(...)
#endif

// time delta with overflow support
static inline unsigned long timeDiff(unsigned long newTime, unsigned long oldTime)
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
static inline unsigned long diff(unsigned int newVal, unsigned int oldVal)
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
