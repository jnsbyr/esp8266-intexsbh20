/*
 * project:  Intex PureSpa WiFi Controller
 *
 * file:     WIFIManager.h
 *
 * encoding: UTF-8
 * created:  20th March 2024
 *
 * Copyright (C) 2021 Jens B.
 *
 *
 * Receive data handling of class PureSpaIO based on code from:
 *
 * DIYSCIP <https://github.com/yorffoeg/diyscip> (c) by Geoffroy HUBERT -
 * yorffoeg@gmail.com
 *
 * DIYSCIP is licensed under a
 * Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 * License.
 *
 * You should have received a copy of the license along with this
 * work. If not, see <https://creativecommons.org/licenses/by-nc-sa/4.0/>.
 *
 * DIYSCIP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY.
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 *
 */

#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

#include "ConfigurationFile.h"
#include "MQTTClient.h"

#include "LEDManager.h"

#define WIFIQUALITY_UPDATE_INTERVAL 5000000

#define WIFI_AP_NAME "ESP8266_Setup"
#define HOME_PAGE "/home.htm.gz"

#define LEN_MAX 512

class WIFIManager {

public:
  WIFIManager(LEDManager& led);

  void setup(ConfigurationFile &config, const char* clientId);
  void setConfig(ConfigurationFile &config);

  void loop();

  bool isSTA();
  bool isAP();
  bool isSTAConnected();

  static uint16_t getWifiQuality();

private:
  LEDManager _led;
  enum MODE { UNSET = 0, STA = 1, AP = 2 };

  ConfigurationFile _config;
  const char* _clientId = NULL;

  MODE _mode = MODE::UNSET;

  volatile uint16_t _checkCode = 0;
  uint16_t _checkLoopCounter = 0;
  IPAddress _checkIPAddress = static_cast<uint32_t>(0);
  ip_addr_t _checkip;
  WiFiClient *_checkWifi = NULL;
  MQTTClient *_checkMQTT = NULL;

  DNSServer *_dnsServer;
  ESP8266WebServer *_httpServer;

  String _ip;

  unsigned long _setupModeTime = 0;

  void handleRoot();
  void handleGetSettings();
  void handlePostSettings();
  void handlePostSave();
  void handleGetChecks();
  void handle404();
  bool redirect();
  bool readHTTPPOST(const char *data);
  const char *nextHTTPField(const char *data, char *field);
  uint16_t getBrokerPort(const char *port);

  void checks();

  uint16_t jsonEscapeString(const char *str, char *jsonStr, uint16_t max_len);

private:
  static const unsigned int EXCEPTION_MESSAGE_SIZE = 80; // [bytes]
  char exceptionMessage[EXCEPTION_MESSAGE_SIZE];
};

#endif // WIFIMANAGER_H
