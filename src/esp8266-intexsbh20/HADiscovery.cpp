/*
 * project:  Intex PureSpa WiFi Controller
 *
 * file:     HADiscovery.cpp
 *
 * encoding: UTF-8
 *
 * Home Assistant MQTT autodiscovery support.
 *
 */

#include "HADiscovery.h"
#include "common.h"
#include <ArduinoJson.h>

static const char* HA_DISCOVERY_PREFIX = "homeassistant";

static String devId;
static String devName;

static void addDevice(JsonObject doc)
{
  JsonObject dev = doc.createNestedObject("dev");
  dev["ids"]  = devId;
  dev["name"] = devName;
  dev["mf"]   = "Intex";
  dev["mdl"]  = devName;
  dev["sw"]   = CONFIG::WIFI_VERSION;
}

static void addAvailability(JsonObject doc)
{
  JsonArray avail = doc.createNestedArray("avty");
  JsonObject a = avail.createNestedObject();
  a["t"]           = MQTT_TOPIC::STATE;
  a["pl_avail"]    = "online";
  a["pl_not_avail"]= "offline";
}

static bool publishDoc(PubSubClient& client, const char* component, const char* objectId, DynamicJsonDocument& doc, char* buf, size_t bufSize)
{
  char topic[128];
  snprintf(topic, sizeof(topic), "%s/%s/%s/config", HA_DISCOVERY_PREFIX, component, objectId);
  size_t len = serializeJson(doc, buf, bufSize);
  bool ok = client.publish(topic, buf, true);
  Serial.printf("  %s (%u bytes): %s\n", topic, len, ok ? "OK" : "FAIL");
  yield();
  return ok;
}

void HADiscovery::publish(PubSubClient& client, const char* modelName)
{
  Serial.println("HA discovery: publishing ...");
  if (!client.connected())
  {
    Serial.println("HA discovery: MQTT not connected");
    return;
  }

  // Build device identifiers from model name
  devName = modelName;
  devId   = modelName;
  devId.toLowerCase();
  devId.replace(' ', '_');
  devId.replace('-', '_');

  DynamicJsonDocument doc(1024);
  char* buf = new char[1024];
  if (!buf)
  {
    Serial.println("HA discovery: out of memory");
    return;
  }

  // ---- CLIMATE: thermostat ----
  {
    doc.clear();
    String uid = devId + "_climate";
    doc["name"]          = "Heater";
    doc["uniq_id"]       = uid;
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["modes"][0]      = "off";
    doc["modes"][1]      = "heat";
    doc["mode_stat_t"]   = MQTT_TOPIC::HEATER;
    doc["mode_stat_tpl"] = "{% set v={'off':'off','on':'heat','standby':'heat'} %}{{v[value]|default('off')}}";
    doc["mode_cmd_t"]    = MQTT_TOPIC::CMD_HEATER;
    doc["mode_cmd_tpl"]  = "{% set v={'heat':'on','off':'off'} %}{{v[value]|default('off')}}";
    doc["act_t"]         = MQTT_TOPIC::HEATER;
    doc["act_tpl"]       = "{% set v={'off':'off','on':'heating','standby':'idle'} %}{{v[value]|default('off')}}";
    doc["curr_temp_t"]   = MQTT_TOPIC::WATER_ACT;
    doc["temp_stat_t"]   = MQTT_TOPIC::WATER_SET;
    doc["temp_cmd_t"]    = MQTT_TOPIC::CMD_WATER;
    doc["temp_step"]     = 1;
    doc["min_temp"]      = 20;
    doc["max_temp"]      = 40;
    doc["temp_unit"]     = "C";
    publishDoc(client, "climate", uid.c_str(), doc, buf, 1024);
  }

  // ---- SWITCH: power ----
  {
    doc.clear();
    String uid = devId + "_switch_power";
    doc["name"]    = "Power";
    doc["uniq_id"] = uid;
    doc["icon"]    = "mdi:power";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"]  = MQTT_TOPIC::POWER;
    doc["cmd_t"]   = MQTT_TOPIC::CMD_POWER;
    doc["pl_on"]   = "on";
    doc["pl_off"]  = "off";
    publishDoc(client, "switch", uid.c_str(), doc, buf, 1024);
  }

  // ---- SWITCH: filter ----
  {
    doc.clear();
    String uid = devId + "_switch_filter";
    doc["name"]    = "Filter";
    doc["uniq_id"] = uid;
    doc["icon"]    = "mdi:air-filter";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"]  = MQTT_TOPIC::FILTER;
    doc["cmd_t"]   = MQTT_TOPIC::CMD_FILTER;
    doc["pl_on"]   = "on";
    doc["pl_off"]  = "off";
    publishDoc(client, "switch", uid.c_str(), doc, buf, 1024);
  }

  // ---- SWITCH: bubble ----
  {
    doc.clear();
    String uid = devId + "_switch_bubble";
    doc["name"]    = "Bubbles";
    doc["uniq_id"] = uid;
    doc["icon"]    = "mdi:chart-bubble";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"]  = MQTT_TOPIC::BUBBLE;
    doc["cmd_t"]   = MQTT_TOPIC::CMD_BUBBLE;
    doc["pl_on"]   = "on";
    doc["pl_off"]  = "off";
    publishDoc(client, "switch", uid.c_str(), doc, buf, 1024);
  }

  // ---- BINARY SENSOR: heater active ----
  {
    doc.clear();
    String uid = devId + "_binary_heater";
    doc["name"]    = "Heater Active";
    doc["uniq_id"] = uid;
    doc["dev_cla"] = "heat";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"]  = MQTT_TOPIC::HEATER;
    doc["pl_on"]   = "on";
    doc["pl_off"]  = "standby";
    publishDoc(client, "binary_sensor", uid.c_str(), doc, buf, 1024);
  }

  // ---- SENSOR: current water temperature ----
  {
    doc.clear();
    String uid = devId + "_sensor_temp_act";
    doc["name"]          = "Water Temperature";
    doc["uniq_id"]       = uid;
    doc["icon"]          = "mdi:thermometer-water";
    doc["dev_cla"]       = "temperature";
    doc["stat_cla"]      = "measurement";
    doc["unit_of_meas"]  = "\u00B0C";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"]        = MQTT_TOPIC::WATER_ACT;
    publishDoc(client, "sensor", uid.c_str(), doc, buf, 1024);
  }

  // ---- SENSOR: target water temperature ----
  {
    doc.clear();
    String uid = devId + "_sensor_temp_set";
    doc["name"]          = "Target Temperature";
    doc["uniq_id"]       = uid;
    doc["icon"]          = "mdi:thermometer-water";
    doc["dev_cla"]       = "temperature";
    doc["unit_of_meas"]  = "\u00B0C";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"]        = MQTT_TOPIC::WATER_SET;
    publishDoc(client, "sensor", uid.c_str(), doc, buf, 1024);
  }

  // ---- SENSOR: error ----
  {
    doc.clear();
    String uid = devId + "_sensor_error";
    doc["name"]    = "Error";
    doc["uniq_id"] = uid;
    doc["icon"]    = "mdi:alert-circle";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"]  = MQTT_TOPIC::ERROR;
    publishDoc(client, "sensor", uid.c_str(), doc, buf, 1024);
  }

  // ---- SENSOR: WiFi RSSI ----
  {
    doc.clear();
    String uid = devId + "_sensor_rssi";
    doc["name"]         = "WiFi RSSI";
    doc["uniq_id"]      = uid;
    doc["icon"]         = "mdi:wifi";
    doc["dev_cla"]      = "signal_strength";
    doc["stat_cla"]     = "measurement";
    doc["unit_of_meas"] = "dBm";
    doc["ent_cat"]      = "diagnostic";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"]       = MQTT_TOPIC::RSSI;
    publishDoc(client, "sensor", uid.c_str(), doc, buf, 1024);
  }

  // ---- SENSOR: WiFi module temperature ----
  {
    doc.clear();
    String uid = devId + "_sensor_wifi_temp";
    doc["name"]         = "WiFi Module Temperature";
    doc["uniq_id"]      = uid;
    doc["icon"]         = "mdi:thermometer";
    doc["dev_cla"]      = "temperature";
    doc["stat_cla"]     = "measurement";
    doc["unit_of_meas"] = "\u00B0C";
    doc["ent_cat"]      = "diagnostic";
    addDevice(doc.as<JsonObject>());
    addAvailability(doc.as<JsonObject>());
    doc["stat_t"]       = MQTT_TOPIC::WIFI_TEMP;
    publishDoc(client, "sensor", uid.c_str(), doc, buf, 1024);
  }

  // ---- SENSOR: IP address ----
  {
    doc.clear();
    String uid = devId + "_sensor_ip";
    doc["name"]    = "IP Address";
    doc["uniq_id"] = uid;
    doc["icon"]    = "mdi:ip-network";
    doc["ent_cat"] = "diagnostic";
    addDevice(doc.as<JsonObject>());
    doc["stat_t"]  = MQTT_TOPIC::IP;
    publishDoc(client, "sensor", uid.c_str(), doc, buf, 1024);
  }

  delete[] buf;
  Serial.println("HA discovery: done");
}
