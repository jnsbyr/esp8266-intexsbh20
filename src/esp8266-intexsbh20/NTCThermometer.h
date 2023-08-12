/*
 * project:  generic
 *
 * file:     NTCThermometer.h
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

#ifndef NTC_THERMOMETER_H
#define NTC_THERMOMETER_H


class NTCThermometer
{
public:
  void setup(unsigned int refResistance, float refVoltage, float adcScale);

public:
  float getTemperature();

private:
  // quality parameters
  static const unsigned int CONSECUTIVE_SAMPLES = 10; // [samples] number of temperature samples taken in a row to be averaged
  static const unsigned int HISTORY_DEPTH       = 10; // [samples] number of temperature moving average history depth

private:
  float analogReadMultiple();
  int getResistance();

private:
  unsigned int refResistance = 22000;              // [Ohm] reference resistance
  float refVoltage           =  3.3f;              // [V] reference voltage, Wemos D1 mini voltage regulator output
  float adcScale             = 320.0f/100.0f/1024; // [V/digit] ESP8266 10-bit ADC, Wemos D1 mini voltage divider 220 kOhm - 100 kOhm

private:
  float history[HISTORY_DEPTH];
  unsigned int historyDepth = 0;
  unsigned int historyHead = 0;
};

#endif /* NTC_THERMOMETER_H */

