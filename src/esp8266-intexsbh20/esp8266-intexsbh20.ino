/*
 * project:  Intex PureSpa WiFi Controller
 *
 * file:     esp8266-intexsbh20.ino
 *
 * encoding: UTF-8
 * created:  13th March 2021
 *
 * Copyright (C) 2021 Jens B.
 *
 *
 * Receive data handling of class PureSpaIO based on code from:
 *
 * DIYSCIP <https://github.com/yorffoeg/diyscip> (c) by Geoffroy HUBERT - yorffoeg@gmail.com
 *
 * DIYSCIP is licensed under a
 * Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
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

/**
 * BUILD ENVIRONMENT:
 *
 * IDE: Arduino 1.8.19
 *
 * Libraries:
 *
 * - Arduino Core for ESP8266 3.1.2
 * - ArduinoJson 6.21.3
 * - PubSubClient (MQTT) 2.8
 *
 * Board: Wemos D1 mini (ESP8266)
 *
 * CPU:        160 MHz
 * Flash:      4M (FS: 1M, OTA: 1M)
 * Debug:      disabled
 * IwIP:       v2 lower memory
 * VTables:    IRAM
 * Exceptions: enabled
 *
 */

#include "common.h"
#include "ConfigurationFile.h"
#include "MQTTClient.h"
#include "MQTTPublisher.h"
#include "NTCThermometer.h"
#include "OTAUpdate.h"
#include "PureSpaIO.h"

#include <stdexcept>

ConfigurationFile config;
NTCThermometer thermometer;
OTAUpdate otaUpdate;
PureSpaIO pureSpaIO;

MQTTClient mqttClient;
MQTTPublisher mqttPublisher(mqttClient, pureSpaIO, thermometer);
String username;
String password;
String mqttPort;

unsigned long disconnectTime = 0;
LANG language = LANG::CODE;
bool initialized = false;


/**
 *  Arduino setup function
 */
void setup()
{
  Serial.begin(74880); // 74880 Baud is the default data rate of the ESP8266 bootloader

  // print versions
  Serial.printf_P(PSTR("%s MQTT WiFi Controller %s\n"), pureSpaIO.getModelName(), CONFIG::WIFI_VERSION);
  Serial.printf_P(PSTR("build with Arduino Core for ESP8266 %s\n"), ESP.getCoreVersion().c_str());
  Serial.printf_P(PSTR("based on Espressif NONOS SDK %s\n"), ESP.getSdkVersion());

  // try to load config file
  bool ready = false;
  if (config.load(CONFIG_TAG::FILENAME))
  {
    try
    {
      // init WiFi (station mode, DHCP, auto modem sleep after 10 s idle, auto wakeup every 100 ms * AP DTIM interval)
      WiFi.mode(WIFI_STA);
      WiFi.begin(config.get(CONFIG_TAG::WIFI_SSID), config.get(CONFIG_TAG::WIFI_PASSPHRASE));

      // init MQTT
      bool retainAll = config.exists(CONFIG_TAG::MQTT_RETAIN)? strcmp(config.get(CONFIG_TAG::MQTT_RETAIN), "no") != 0 : false;
      mqttPublisher.setRetainAll(retainAll);

      mqttClient.addMetadata(MQTT_TOPIC::MODEL, pureSpaIO.getModelName());
      mqttClient.addMetadata(MQTT_TOPIC::VERSION, CONFIG::WIFI_VERSION);

      mqttClient.addSubscriber(MQTT_TOPIC::CMD_BUBBLE, [](bool b) -> void { pureSpaIO.setBubbleOn(b); });
      mqttClient.addSubscriber(MQTT_TOPIC::CMD_FILTER, [](bool b) -> void { pureSpaIO.setFilterOn(b); });
      mqttClient.addSubscriber(MQTT_TOPIC::CMD_HEATER, [](bool b) -> void { pureSpaIO.setHeaterOn(b); });
      mqttClient.addSubscriber(MQTT_TOPIC::CMD_POWER,  [](bool b) -> void { pureSpaIO.setPowerOn(b); });
      mqttClient.addSubscriber(MQTT_TOPIC::CMD_WATER,  [](int i) -> void { pureSpaIO.setDesiredWaterTempCelsius(i); });
      if (pureSpaIO.getModel() == PureSpaIO::MODEL::SJBHS)
      {
        mqttClient.addSubscriber(MQTT_TOPIC::CMD_DISINFECTION, [](int i) -> void { pureSpaIO.setDisinfectionTime(i); });
        mqttClient.addSubscriber(MQTT_TOPIC::CMD_JET,          [](bool b) -> void { pureSpaIO.setJetOn(b); });
      }

      // enable OTA update if URL is defined in config
      if (config.exists(CONFIG_TAG::WIFI_OTA_URL))
      {
        mqttClient.addSubscriber(MQTT_TOPIC::CMD_OTA,  [](bool b) -> void { if (b) otaUpdate.start(config.get(CONFIG_TAG::WIFI_OTA_URL), mqttClient); });
      }

      // set language of error message if defined in config
      if (config.exists(CONFIG_TAG::MQTT_ERROR_LANG))
      {
        String lang = config.get(CONFIG_TAG::MQTT_ERROR_LANG);
        language = lang == "EN"? LANG::EN : (lang == "DE"? LANG::DE : LANG::CODE);
      }

      // init MQTT client
      if (config.exists(CONFIG_TAG::MQTT_USER))
      {
        username = config.get(CONFIG_TAG::MQTT_USER);
        password = config.get(CONFIG_TAG::MQTT_PASSWORD);
      }
      uint16 mqttPort;
      if (config.exists(CONFIG_TAG::MQTT_PORT))
      {
        mqttPort = atoi(config.get(CONFIG_TAG::MQTT_PORT));
      }
      else
      {
        mqttPort = 1883;
      }
      mqttClient.setup(config.get(CONFIG_TAG::MQTT_SERVER), mqttPort, username.c_str(), password.c_str(), pureSpaIO.getModelName(), MQTT_TOPIC::STATE, "offline");

      // init NTC thermometer
      thermometer.setup(22000, 3.33f, 320.f/100.f); // measured: 21990, 3.327f, 319.f/99.6f

      // enable hardware watchdog (8.3 s) by disabling software watchdog
      ESP.wdtDisable();

      ready = true;
    }
    catch (const std::runtime_error& re)
    {
      Serial.println(re.what());
    }
  }

  // stop CPU if init failed
  if (!ready)
  {
    ESP.deepSleep(ESP.deepSleepMax(), RF_DISABLED);
  }
}

/**
 * Arduino loop function
 */
void loop()
{
  // keep hardware watchdog alive
  ESP.wdtFeed();

  wl_status_t wifiStatus = WiFi.status(); //  WL_IDLE_STATUS 0, WL_NO_SSID_AVAIL 1, WL_SCAN_COMPLETED 2, WL_CONNECTED 3, WL_CONNECT_FAILED 4, WL_CONNECTION_LOST 5, WL_DISCONNECTED 6, WL_NO_SHIELD 255
  unsigned long now = millis();
  if (wifiStatus == WL_CONNECTED)
  {
    // WiFi is connected
    disconnectTime = 0;

    if (!initialized)
    {
      // publish client IP address
      mqttClient.addMetadata(MQTT_TOPIC::IP, WiFi.localIP().toString().c_str());

      // init whirlpool I/O after first WiFi connect
      pureSpaIO.setup(language);
      initialized = true;
    }
    else
    {
      // update MQTT
      mqttClient.loop();
      mqttPublisher.loop();

      // update pool
      pureSpaIO.loop();

      // force idle
      delay(100);
    }
  }
  else
  {
    // restart ESP8266 if WiFi connection cannot be established
    if (!disconnectTime)
    {
      // WiFi disconnected
      disconnectTime = now;
    }
    else if (timeDiff(now, disconnectTime) > CONFIG::WIFI_MAX_DISCONNECT_DURATION)
    {
      // WiFi disconnected too long, restart ESP
      DEBUG_MSG("restarting ... (no WiFi connection for several minutes)\n");
      ESP.restart();
    }
  }
}
