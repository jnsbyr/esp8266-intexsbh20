/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     esp8266-intexsbh20.ino
 *
 * encoding: UTF-8
 * created:  13th March 2021
 *
 * Copyright (C) 2021 Jens B.
 *
 *
 * Receive data handling of class SBH20IO based on code from:
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
 * IDE: Arduino 1.8.13
 *
 * Libraries:
 *
 * - Arduino Core for ESP8266 2.7.4
 * - ArduinoJson 6.17.2
 * - PubSubClient (MQTT) 2.8.0
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

#include "NTCThermometer.h"
#include "OTAUpdate.h"
#include "SBH20IO.h"
#include "ConfigurationFile.h"
#include "MQTTClient.h"
#include "MQTTPublisher.h"
#include "common.h"


ConfigurationFile config;
NTCThermometer thermometer;
OTAUpdate otaUpdate;

SBH20IO poolIO;

MQTTClient mqttClient;
MQTTPublisher mqttPublisher(mqttClient, poolIO, thermometer);

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
  Serial.printf_P(PSTR("SB-H20 WiFi Controller %s\n"), CONFIG::WIFI_VERSION);
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

      // init MQTT client
      mqttClient.addMetadata(MQTT_TOPIC::MODEL, CONFIG::POOL_MODEL_NAME);
      mqttClient.addMetadata(MQTT_TOPIC::VERSION, CONFIG::WIFI_VERSION);
      mqttClient.addSubscriber(MQTT_TOPIC::CMD_BUBBLE, [](bool b) -> void { poolIO.setBubbleOn(b); });
      mqttClient.addSubscriber(MQTT_TOPIC::CMD_FILTER, [](bool b) -> void { poolIO.setFilterOn(b); });
      mqttClient.addSubscriber(MQTT_TOPIC::CMD_HEATER, [](bool b) -> void { poolIO.setHeaterOn(b); });
      mqttClient.addSubscriber(MQTT_TOPIC::CMD_POWER,  [](bool b) -> void { poolIO.setPowerOn(b); });
      mqttClient.addSubscriber(MQTT_TOPIC::CMD_WATER,  [](int i) -> void { poolIO.setDesiredWaterTempCelsius(i); });
      if (config.exists(CONFIG_TAG::WIFI_OTA_URL))
      {
        // enable OTA update if URL is defined in config
        mqttClient.addSubscriber(MQTT_TOPIC::CMD_OTA,  [](bool b) -> void { if (b) otaUpdate.start(config.get(CONFIG_TAG::WIFI_OTA_URL), mqttClient); });
      }
      if (config.exists(CONFIG_TAG::MQTT_ERROR_LANG))
      {
        // set language of error message if defined in config
        String lang = config.get(CONFIG_TAG::MQTT_ERROR_LANG);
        language = lang == "EN"? LANG::EN : (lang == "DE"? LANG::DE : LANG::CODE);
      }
      mqttClient.setup(config.get(CONFIG_TAG::MQTT_SERVER), config.get(CONFIG_TAG::MQTT_USER), config.get(CONFIG_TAG::MQTT_PASSWORD), CONFIG::POOL_MODEL_NAME, MQTT_TOPIC::STATE, "offline");

      // init NTC thermometer
      thermometer.setup(22000, 3.33f, 320.f/100.f); // measured: 21990, 3.327f, 319.f/99.6f

      // enable hardware watchdog (8.3 s) by disabling software watchdog
      ESP.wdtDisable();

      ready = true;
    }
    catch (std::runtime_error re)
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
      poolIO.setup(language);
      initialized = true;
    }
    else
    {
      // update MQTT
      mqttClient.loop();
      mqttPublisher.loop();

      // update pool
      poolIO.loop();

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
