/*
 * project:  generic
 *
 * file:     ConfigurationFile.cpp
 *
 * encoding: UTF-8
 * created:  23rd March 2021
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

#include "ConfigurationFile.h"

#include <LittleFS.h>
#include <stdexcept>


/**
 * load JSON config file into memory
 *
 * @param fileName name of file
 * @return true on success
 */
bool ConfigurationFile::load(const char* fileName)
{
  bool success = true;

  if (LittleFS.begin())
  {
    try
    {
      // open wifiConfig file
      File configFile = LittleFS.open(fileName, "r");
      if (!configFile)
      {
        snprintf_P(exceptionMessage, EXCEPTION_MESSAGE_SIZE, PSTR("config file '%s' not found"), fileName);
        throw std::runtime_error(exceptionMessage);
      }
      size_t fileSize = configFile.size();
      if (fileSize > configDoc.capacity())
      {
        snprintf_P(exceptionMessage, EXCEPTION_MESSAGE_SIZE, PSTR("max. config file size of %u bytes exceeded (has %u bytes)"), configDoc.capacity(), fileSize);
        throw std::runtime_error(exceptionMessage);
      }

      // parse buffer as JSON object
      DeserializationError error = deserializeJson(configDoc, configFile);
      if (error)
      {
        snprintf_P(exceptionMessage, EXCEPTION_MESSAGE_SIZE, PSTR("error parsing config file: %s"), error.f_str());
        throw std::runtime_error(exceptionMessage);
      }

      // close config file
      configFile.close();
      Serial.printf_P(PSTR("config file loaded successfully (has %d entries)\n"), configDoc.size());
    }
    catch (const std::runtime_error& re)
    {
      Serial.println(re.what());
      success = false;
    }
  }
  else
  {
    Serial.printf_P(PSTR("error mounting file system, unable to open config file\n"));
    success = false;
  }

  return success;
}

/**
 * try if config tag exists
 * @param tag
 * @return true if config tag exists
 */
bool ConfigurationFile::exists(const char* tag) const
{
  return configDoc.containsKey(tag);
}

/**
 * try to find config tag and return its value
 *
 * @param tag config tag
 * @return config value for given tag, might be empty
 *
 * @throws std::runtime_error if config tag is unknown
 */
const char* ConfigurationFile::get(const char* tag)
{
  if (configDoc.containsKey(tag))
  {
    return configDoc[tag];
  }
  else
  {
    snprintf_P(exceptionMessage, EXCEPTION_MESSAGE_SIZE, PSTR("required entry '%s' not found in config file"), tag);
    throw std::runtime_error(exceptionMessage);
    return NULL;
  }
}
