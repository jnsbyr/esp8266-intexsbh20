/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     OTAUpdate.h
 *
 * encoding: UTF-8
 * created:  27th March 2021
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

#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

class MQTTClient;


class OTAUpdate
{
public:
  bool start(const char* updateURL, MQTTClient& mqttClient);

};

#endif /* OTA_UPDATE_H */

