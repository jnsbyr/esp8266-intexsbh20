/*
 * project:  generic
 *
 * file:     ConfigurationFile.h
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

#ifndef CONFIGURATION_FILE_H
#define CONFIGURATION_FILE_H

#include <ArduinoJson.h>


class ConfigurationFile
{
public:
  bool load(const char* fileName);
  bool exists(const char* tag) const;
  const char* get(const char* tag);

private:
  static const unsigned int CONFIG_BUFFER_SIZE     = 512; // [bytes]
  static const unsigned int EXCEPTION_MESSAGE_SIZE =  80; // [bytes]

private:
  ArduinoJson::StaticJsonDocument<CONFIG_BUFFER_SIZE> configDoc;
  char exceptionMessage[EXCEPTION_MESSAGE_SIZE];
};

#endif /* CONFIGURATION_FILE_H */

