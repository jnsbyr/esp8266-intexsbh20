/*
 * project:  generic
 *
 * file:     NTCThermometer.cpp
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

#include "NTCThermometer.h"

#include <Esp.h>


// set ADC to read from input pin A0
ADC_MODE(ADC_TOUT)

/**
 * @param refResistance resistance between NTC and GND [Ohm]
 * @param refVoltage voltage at NTC [V]
 * @param adcScale inverted voltage divider relation of analog input (Wemos D1 mini: 320/100)
 */
void NTCThermometer::setup(unsigned int refResistance, float refVoltage, float adcScale)
{
  this->refResistance = refResistance;
  this->refVoltage = refVoltage;
  this->adcScale = adcScale/1024; // -> [V/digit]
}

/**
 * read analog input multiple times and return average [digits]
 *
 * @return average digits
 */
float NTCThermometer::analogReadMultiple()
{
  long sum = 0;
  unsigned int count = 0;
  for (unsigned int i=0; i<CONSECUTIVE_SAMPLES; i++)
  {
    int sample = analogRead(A0);
    if (sample >= 0)
    {
      sum += sample;
      count++;
    }
  }

  if (count == 0)
  {
    count = 1;
  }

  return (float)sum/count;
}

/**
 * measure resistance R of a voltage divider:
 * Vref - R (NTC, var) - Rref - GND
 *
 * return resistance [Ohm]
 */
int NTCThermometer::getResistance()
{
  return round((refVoltage/(adcScale*analogReadMultiple()) - 1)*refResistance);
}

/**
 * sample current 10k NTC temperature [°C] and calculate block moving average
 *
 * see Arduino Playground: "Reading a Thermistor"
 *
 * @return temperature [°C]
 */
float NTCThermometer::getTemperature()
{
  // calculate current temperature and
  // update temperature history ring buffer
  float x = log(getResistance());
  history[historyHead++] = 1.0f / (0.001129148f + 0.000234125f*x + 0.0000000876741f*x*x*x) - 273.15f;
  historyHead %= HISTORY_DEPTH;
  if (historyDepth < HISTORY_DEPTH)
  {
    historyDepth++;
  }

  // calculate block moving average
  float sum = 0;
  for (unsigned int i=0; i<historyDepth; i++)
  {
    sum += history[i];
  }

  return sum/historyDepth;
}
