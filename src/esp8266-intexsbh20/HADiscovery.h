/*
 * project:  Intex PureSpa WiFi Controller
 *
 * file:     HADiscovery.h
 *
 * encoding: UTF-8
 *
 * Home Assistant MQTT autodiscovery support.
 * Publishes discovery configs so Home Assistant automatically creates
 * entities for the PureSpa without any manual YAML configuration.
 *
 * Reference: https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
 *
 */

#ifndef HA_DISCOVERY_H
#define HA_DISCOVERY_H

#include <PubSubClient.h>

class HADiscovery
{
public:
  /**
   * Publish Home Assistant MQTT discovery configs for all PureSpa entities.
   *
   * @param client    connected PubSubClient instance
   * @param modelName PureSpa model name (used as device name and for unique IDs)
   */
  static void publish(PubSubClient& client, const char* modelName);
};

#endif /* HA_DISCOVERY_H */
