/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     WIFIManager.cpp
 *
 * encoding: UTF-8
 * created:  14th October 2022
 *
 *
 * Receive data handling of class PureSpaIO based on code from:
 *
 * DIYSCIP <https://github.com/yorffoeg/diyscip> (c) by Geoffroy HUBERT -
 * yorffoeg@gmail.com
 *
 * Update by Pierre-Yves MORDRET - titolini72@gmail.com
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

#include <LittleFS.h>
#include <stdexcept>

#include "MQTTClient.h"
#include "WIFIManager.h"
#include "common.h"

#include "lwip/dns.h"

#define DNS_PORT 53
#define HTTP_PORT 80

#define SETUP_TIMEOUT 300000 // 5 min (5min * 60sec * 1000ms)

#define CHECK_PENDING_LOOP_MAX 200

#define CHECK_ERROR 0x8000
#define CHECK_PENDING 0x4000
#define CHECK_RESTART 0xFF00

#define CHECK_WIFI 0x0100
#define CHECK_HOST 0x0200
#define CHECK_TCP 0x0400
#define CHECK_MQTT 0x0800

#define CHECK_EXTRA_CODE 0x00FF

#define IS_CHECK_FAILED(code) (code & CHECK_ERROR)
#define IS_CHECK_PENDING(code) (code & CHECK_PENDING)

#define JSON_ESCAPED_MAX 128

void check_dns_found_callback(const char *name, const ip_addr_t *ipaddr,
                              void *callback_arg);

const char *mime_json = "application/json; charset=UTF-8";

char *home_data   = NULL;
char *http_fields = NULL;
int home_len      = 0;

WIFIManager::WIFIManager(LEDManager& led) : _led(led) {}

void WIFIManager::setup(ConfigurationFile &config, const char* clientId) {
  _config = config;
  _clientId = clientId;

  if (strcmp(_config.get(CONFIG_TAG::WIFI_MODE), "SETUP") == 0) {
    bool success = true;

    if (LittleFS.begin()) {
      try {
        File homeFile = LittleFS.open(HOME_PAGE, "r");
        uint32_t i = 0;

        if (!homeFile) {
          snprintf_P(exceptionMessage, EXCEPTION_MESSAGE_SIZE,
                     PSTR("Home file '%s' not found"), "home.htm");
          throw std::runtime_error(exceptionMessage);
        }

        Serial.print("Home Filename: ");
        Serial.print(homeFile.fullName());
        Serial.print("  Size: ");
        Serial.println(homeFile.size());
        home_len = homeFile.size();

        home_data = (char *)calloc(homeFile.size(), sizeof(char));
        if (home_data == (char *)NULL) {
          Serial.println("No heap space for data");
          snprintf_P(exceptionMessage, EXCEPTION_MESSAGE_SIZE,
                     PSTR("No heap space for data"));
          throw std::runtime_error(exceptionMessage);
        }

        while (homeFile.available() && i < homeFile.size()) {
          home_data[i++] = homeFile.read();
        }

        homeFile.close();
        Serial.println("Home Page read successfull");

      } catch (const std::runtime_error &re) {
        Serial.println("Can't read Home page from LittleFS");
        Serial.println(re.what());
        success = false;
      }
    } else {
      success = false;
      Serial.printf_P(
          PSTR("error mounting file system, unable to open _config file\n"));
    }

    if (!success) {
      ESP.deepSleep(ESP.deepSleepMax(), RF_DISABLED);
    }

    // start AP
    // entering setup mode from running request...
    // timeout back to running if no setup
    _setupModeTime = millis();
    _mode = WIFIManager::AP;

    _dnsServer = new DNSServer();
    _httpServer = new ESP8266WebServer(HTTP_PORT);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_NAME);

    // delay(500);

    IPAddress ipAddr = WiFi.softAPIP();
    for (int i = 0; i < 3; i++)
      _ip += String((ipAddr >> (8 * i)) & 0xFF) + ".";
    _ip += String(((ipAddr >> 8 * 3)) & 0xFF);

    Serial.printf("AP ip = %s", _ip.c_str());

    _dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer->start(DNS_PORT, "*", ipAddr);

    _httpServer->on(String(F("/")).c_str(),
                    std::bind(&WIFIManager::handleRoot, this));
    _httpServer->on(String(F("/settings")).c_str(), HTTP_GET,
                    std::bind(&WIFIManager::handleGetSettings, this));
    _httpServer->on(String(F("/settings")).c_str(), HTTP_POST,
                    std::bind(&WIFIManager::handlePostSettings, this));
    _httpServer->on(String(F("/checks")).c_str(), HTTP_GET,
                    std::bind(&WIFIManager::handleGetChecks, this));
    _httpServer->on(String(F("/save")).c_str(), HTTP_POST,
                    std::bind(&WIFIManager::handlePostSave, this));

    _httpServer->onNotFound(std::bind(&WIFIManager::handle404, this));

    _httpServer->begin();
  } else { // start STA
    _mode = WIFIManager::STA;

    WiFi.mode(WIFI_STA);
    WiFi.enableAP(false);
    WiFi.hostname(_clientId);
    WiFi.begin((const char *)_config.get(CONFIG_TAG::WIFI_SSID),
               (const char *)_config.get(CONFIG_TAG::WIFI_PASSPHRASE));
  }
}

void WIFIManager::loop() {
  if (_mode == WIFIManager::AP) {

    _dnsServer->processNextRequest();
    _httpServer->handleClient();

    checks();
  }
}

bool WIFIManager::isSTA() { return _mode == WIFIManager::STA; }

bool WIFIManager::isAP() { return _mode == WIFIManager::AP; }

bool WIFIManager::isSTAConnected() {
  return (_mode == WIFIManager::STA) && (WiFi.status() == WL_CONNECTED);
}

void WIFIManager::handleRoot() {
  if (!redirect()) {
    if (home_data == (char *)NULL)
      return;

    _httpServer->sendHeader("Content-Encoding", String("gzip"));
    _httpServer->send_P(200, "text/html; charset=UTF-8", home_data, home_len);
  }
}

void WIFIManager::handleGetSettings() {

  if (_setupModeTime != 0) {
    _setupModeTime = millis();
  }

  String json("{\"wifis\":[");
  char jsonEscaped[JSON_ESCAPED_MAX + 1];
  int nbWifis = WiFi.scanNetworks();

  if (nbWifis > 0) {
    for (int i = 0; i < nbWifis; i++) {
      if (i > 0) {
        json += ",";
      }

      json += "{\"ssid\":\"";
      jsonEscapeString(WiFi.SSID(i).c_str(), jsonEscaped, JSON_ESCAPED_MAX);
      json += jsonEscaped;
      json += "\", \"rssi\":\"";
      json += WiFi.RSSI(i);
      json += "\",\"secure\":";
      json += (WiFi.encryptionType(i) != ENC_TYPE_NONE) ? "true" : "false";
      json += "}";
    }
  }

  json += "],\"ssid\":\"";
  jsonEscapeString(_config.get(CONFIG_TAG::WIFI_SSID), jsonEscaped,
                   JSON_ESCAPED_MAX);
  json += jsonEscaped;

  json += "\",\"wifipwd\":\"";
  jsonEscapeString(_config.get(CONFIG_TAG::WIFI_PASSPHRASE), jsonEscaped,
                   JSON_ESCAPED_MAX);
  json += jsonEscaped;

  json += "\",\"otafw\":\"";
  jsonEscapeString(_config.get(CONFIG_TAG::WIFI_OTA_URL), jsonEscaped,
                   JSON_ESCAPED_MAX);
  json += jsonEscaped;

  json += "\",\"host\":\"";
  jsonEscapeString(_config.get(CONFIG_TAG::MQTT_SERVER), jsonEscaped,
                   JSON_ESCAPED_MAX);
  json += jsonEscaped;

  json += "\",\"port\":\"";
  json += _config.get(CONFIG_TAG::MQTT_PORT);

  json += "\",\"user\":\"";
  jsonEscapeString(_config.get(CONFIG_TAG::MQTT_USER), jsonEscaped,
                   JSON_ESCAPED_MAX);
  json += jsonEscaped;

  json += "\",\"pwd\":\"";
  jsonEscapeString(_config.get(CONFIG_TAG::MQTT_PASSWORD), jsonEscaped,
                   JSON_ESCAPED_MAX);
  json += jsonEscaped;

  json += "\",\"mqttretain\":\"";
  jsonEscapeString(_config.get(CONFIG_TAG::MQTT_RETAIN), jsonEscaped,
                   JSON_ESCAPED_MAX);
  json += jsonEscaped;

  json += "\",\"lang\":\"";
  jsonEscapeString(_config.get(CONFIG_TAG::MQTT_ERROR_LANG), jsonEscaped,
                   JSON_ESCAPED_MAX);
  json += jsonEscaped;

  json += "\",\"check\":\"";
  json += _checkCode;
  json += "\"}";

  _httpServer->send(200, mime_json, json);
}

const char *WIFIManager::nextHTTPField(const char *data, char *field) {
  int len = 0;
  char c;

  while ((c = *data) != '\0') {
    data++;
    if (c != '\n') {
      field[len++] = c;
    } else {
      break;
    }
  }

  field[len] = '\0';
  return data;
}

bool WIFIManager::readHTTPPOST(const char *data) {
  char *field = NULL;

  http_fields = (char *)calloc(strlen(data), sizeof(char));
  if (http_fields == (char *)NULL) {
    Serial.println("No heap space for data");
    snprintf_P(exceptionMessage, EXCEPTION_MESSAGE_SIZE,
              PSTR("No heap space for data"));
    throw std::runtime_error(exceptionMessage);
  }

  field = http_fields;

  data = nextHTTPField(data, field);
  _config.set(CONFIG_TAG::WIFI_SSID, field);       field += (strlen(field) + 1);

  data = nextHTTPField(data, field);
  _config.set(CONFIG_TAG::WIFI_PASSPHRASE, field); field += (strlen(field) + 1);

  data = nextHTTPField(data, field);
  _config.set(CONFIG_TAG::WIFI_OTA_URL, field);    field += (strlen(field) + 1);

  data = nextHTTPField(data, field);
  _config.set(CONFIG_TAG::MQTT_SERVER, field);     field += (strlen(field) + 1);

  data = nextHTTPField(data, field);
  _config.set(CONFIG_TAG::MQTT_PORT, field);       field += (strlen(field) + 1);

  data = nextHTTPField(data, field);
  _config.set(CONFIG_TAG::MQTT_USER, field);       field += (strlen(field) + 1);

  data = nextHTTPField(data, field);
  _config.set(CONFIG_TAG::MQTT_PASSWORD, field);   field += (strlen(field) + 1);

  data = nextHTTPField(data, field);
  _config.set(CONFIG_TAG::MQTT_RETAIN, field);     field += (strlen(field) + 1);

  data = nextHTTPField(data, field);
  _config.set(CONFIG_TAG::MQTT_ERROR_LANG, field); field += (strlen(field) + 1);

  return !strcmp(data, "EOD");
}

void WIFIManager::handlePostSettings() {

  if (!IS_CHECK_PENDING(_checkCode)) {
    if (_httpServer->hasArg("plain")) {
      if (readHTTPPOST(_httpServer->arg("plain").c_str())) {
        _checkCode = CHECK_PENDING | CHECK_WIFI;
        _checkLoopCounter = 0;

        DEBUG_MSG("WiFi.begin %s=%s", _config.get(CONFIG_TAG::WIFI_SSID),
                  _config.get(CONFIG_TAG::WIFI_PASSPHRASE));

        WiFi.hostname(_clientId);
        WiFi.begin(_config.get(CONFIG_TAG::WIFI_SSID),
                   _config.get(CONFIG_TAG::WIFI_PASSPHRASE));

        _httpServer->send(200, mime_json, "{}");
      } else {
        _httpServer->send(200, mime_json,
                          "{\"error\": \"Error in settings data\"}");
      }
    } else {
      _httpServer->send(200, mime_json,
                        "{\"error\": \"Missing settings data\"}");
    }
  } else {
    _httpServer->send(
        200, mime_json,
        "{\"error\": \"Previous settings are currently checking\"}");
  }
}

void WIFIManager::handleGetChecks() {

  if (_setupModeTime != 0) {
    _setupModeTime = millis();
  }

  String json("{\"checks\":");
  json += _checkCode;
  json += "}";

  _httpServer->send(200, mime_json, json);
}

void WIFIManager::handlePostSave() {

  if (_checkCode == CHECK_MQTT) {

    _config.set(CONFIG_TAG::WIFI_MODE, "RUNNING");
    if (_config.save(CONFIG_TAG::FILENAME)) {
      _httpServer->send(200, mime_json, "{}");

      // do not restart immediatly otherwise response will not be send
      _checkLoopCounter = 5;
      _checkCode = CHECK_RESTART;
      _led.setMode(BLINK_OK);
    } else {
      _httpServer->send(200, mime_json,
                        "{\"error\": \"Configuration commit failed\"}");
      _led.setMode(BLINK_KO);
    }
  } else {
    _httpServer->send(200, mime_json,
                      "{\"error\": \"Settings must pass checks\"}");
    _led.setMode(BLINK_KO);
  }
}

void WIFIManager::handle404() {
  if (!redirect()) {
    _httpServer->send(404, "text/plain", "");
  }
}

bool WIFIManager::redirect() {
  const String host = _httpServer->hostHeader();

  if ((host != "") && host.compareTo(_ip)) {

    _httpServer->sendHeader("Location", String("http://") + _ip, true);
    _httpServer->send(302, "text/plain", "");
    _httpServer->client().stop();

    return true;
  }
  return false;
}

uint16_t WIFIManager::getBrokerPort(const char *port) {
  char *value = (char *)port;
  uint16_t uint16Value = 0;

  while (*value != 0) {
    if (*value >= '0' && *value <= '9') {

      uint16Value = uint16Value * 10 + (*value - '0');
      value++;
    } else {
      DEBUG_MSG("BrokerPort wrong uint16 value");
      break;
    }
  }

  return uint16Value;
}

void WIFIManager::checks() {
  if (_checkCode == CHECK_RESTART) {
    _checkLoopCounter--;
    if (_checkLoopCounter <= 0) {
      free(home_data);
      free(http_fields);
      DEBUG_MSG("Restarting");
      ESP.restart();
    }
    return;
  }

  if ((_setupModeTime != 0) && ((millis() - _setupModeTime) > SETUP_TIMEOUT)) {
    DEBUG_MSG("Server Timeout. Going to Sleep");
    ESP.deepSleep(ESP.deepSleepMax(), RF_DISABLED);
    return;
  }

  if (IS_CHECK_PENDING(_checkCode)) {
    if (_checkCode & CHECK_WIFI) {
      wl_status_t wifiStatus = WiFi.status();

      if (wifiStatus == WL_CONNECTED) {
        if (!_checkIPAddress.fromString(_config.get(CONFIG_TAG::MQTT_SERVER))) {
          err_t err =
              dns_gethostbyname(_config.get(CONFIG_TAG::MQTT_SERVER), &_checkip,
                                &check_dns_found_callback, &_checkIPAddress);
          if (err == ERR_OK) {
            _checkIPAddress = IPAddress(_checkip);
            _checkCode = CHECK_PENDING | CHECK_TCP;
          } else if (err == ERR_INPROGRESS) {
            _checkCode = CHECK_PENDING | CHECK_HOST;
          } else {
            _checkCode &= ~CHECK_PENDING;
            _checkCode |= CHECK_ERROR;
          }
        } //  else  host is an ip

        _checkCode = CHECK_PENDING | CHECK_HOST;
        _checkLoopCounter = 0;
      } else if (wifiStatus == WL_CONNECT_FAILED) {
        _checkCode = CHECK_ERROR | CHECK_WIFI;
      }
    } else if (_checkCode & CHECK_HOST) {
      if (_checkIPAddress != IPADDR_ANY) {
        if (_checkIPAddress != IPADDR_BROADCAST) {
          _checkWifi = new WiFiClient();
          _checkWifi->connect(_checkIPAddress, atoi(_config.get(CONFIG_TAG::MQTT_PORT)));

          _checkCode = CHECK_PENDING | CHECK_TCP;
          _checkLoopCounter = 0;
        } else {
          _checkCode &= ~CHECK_PENDING;
          _checkCode |= CHECK_ERROR;
        }
      }
    } else if (_checkCode & CHECK_TCP) {
      if (_checkWifi->connected()) {
        _checkMQTT = new MQTTClient(_checkWifi);
        if ( _checkMQTT->check_connect(_config.get(CONFIG_TAG::MQTT_SERVER),
                                        atoi(_config.get(CONFIG_TAG::MQTT_PORT)),
                                        (const char *)_config.get(CONFIG_TAG::MQTT_USER),
                                        (const char *)_config.get(CONFIG_TAG::MQTT_PASSWORD),
                                        _clientId, MQTT_TOPIC::STATE, "offline")) {
            _checkCode = CHECK_PENDING | CHECK_MQTT;
        } else {
          _checkCode &= ~CHECK_PENDING;
          _checkCode |= CHECK_ERROR;
        }
        _checkLoopCounter = 0;
      }
    } else if (_checkCode & CHECK_MQTT) {
        if (!_checkMQTT->isConnected()) {
          _checkCode |= CHECK_ERROR;
        }
        _checkCode &= ~CHECK_PENDING;
    }

    _checkLoopCounter++;
    if (_checkLoopCounter >= CHECK_PENDING_LOOP_MAX) {
      _checkCode &= ~CHECK_PENDING;
      _checkCode |= CHECK_ERROR;
    }

    if (!IS_CHECK_PENDING(_checkCode)) {
      _checkLoopCounter = 0;
      _checkIPAddress = static_cast<uint32_t>(0);

      if (_checkMQTT != NULL) {
        delete _checkMQTT;
        _checkMQTT = NULL;
      }

      if (_checkWifi != NULL) {
        _checkWifi->stop();
        delete _checkWifi;
        _checkWifi = NULL;
      }

      WiFi.disconnect(true);
    }
  }
}

uint16_t WIFIManager::jsonEscapeString(const char *str, char *jsonStr,
                                       uint16_t max_len) {
  int i = 0;

  while ((*str != '\0') && (i < max_len)) {
    switch (*str) {
    case '"':
    case '\\':
    case '/':
      if ((i + 1) < max_len) {
        jsonStr[i++] = '\\';
        jsonStr[i] = *str;
      }
      break;

    case '\b':
      if ((i + 1) < max_len) {
        jsonStr[i++] = '\\';
        jsonStr[i] = 'b';
      }
      break;

    case '\f':
      if ((i + 1) < max_len) {
        jsonStr[i++] = '\\';
        jsonStr[i] = 'f';
      }
      break;

    case '\n':
      if ((i + 1) < max_len) {
        jsonStr[i++] = '\\';
        jsonStr[i] = 'n';
      }
      break;

    case '\r':
      if ((i + 1) < max_len) {
        jsonStr[i++] = '\\';
        jsonStr[i] = 'r';
      }
      break;

    case '\t':
      if ((i + 1) < max_len) {
        jsonStr[i++] = '\\';
        jsonStr[i] = 't';
      }
      break;

    default:
      jsonStr[i] = *str;
    }

    str++;
    i++;
  }

  jsonStr[i] = '\0';
  return i;
}

void check_dns_found_callback(const char *name, const ip_addr_t *ipaddr,
                              void *callback_arg) {
  if (ipaddr) {
    (*reinterpret_cast<IPAddress *>(callback_arg)) = IPAddress(ipaddr);
  } else {
    (*reinterpret_cast<IPAddress *>(callback_arg)) = IPADDR_BROADCAST;
  }
}
