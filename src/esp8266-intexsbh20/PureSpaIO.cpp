/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     PureSpaIO.cpp
 *
 * encoding: UTF-8
 * created:  14th March 2021
 *
 * Copyright (C) 2021 Jens B.
 *
 *
 * Receive data handling based on code from:
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

#include "PureSpaIO.h"

#include <ESP8266WiFi.h>


#if defined MODEL_SB_H20

#define DEFAULT_MODEL_NAME "Intex PureSpa SB-H20"

// bit mask for LEDs
namespace FRAME_LED
{
  const uint16 POWER          = 0x0001;
  const uint16 HEATER_ON      = 0x0080;  // max. 72 h, will start filter, will not stop filter
  const uint16 NO_BEEP        = 0x0100;
  const uint16 HEATER_STANDBY = 0x0200;
  const uint16 BUBBLE         = 0x0400;  // max. 30 min
  const uint16 FILTER         = 0x1000;  // max. 24 h
}

// bit mask of button
namespace FRAME_BUTTON
{
  const uint16 FILTER    = 0x0002;
  const uint16 BUBBLE    = 0x0008;
  const uint16 TEMP_DOWN = 0x0080;
  const uint16 POWER     = 0x0400;
  const uint16 TEMP_UP   = 0x1000;
  const uint16 TEMP_UNIT = 0x2000;
  const uint16 HEATER    = 0x8000;
}

#elif defined MODEL_SJB_HS

#define DEFAULT_MODEL_NAME "Intex PureSpa SJB-HS"

// bit mask for LEDs
namespace FRAME_LED
{
  const uint16 POWER          = 0x0001;
  const uint16 BUBBLE         = 0x0002;  // max. 30 min
  const uint16 HEATER_ON      = 0x0080;  // max. 72 h, will start filter, will not stop filter
  const uint16 NO_BEEP        = 0x0100;
  const uint16 HEATER_STANDBY = 0x0200;
  const uint16 JET            = 0x0400;
  const uint16 FILTER         = 0x1000;  // max. 24 h
  const uint16 DISINFECTION   = 0x2000;  // max. 8 h
}

// bit mask of button
namespace FRAME_BUTTON
{
  const uint16 DISINFECTION = 0x0001;
  const uint16 BUBBLE       = 0x0002;
  const uint16 JET          = 0x0008;
  const uint16 FILTER       = 0x0080;
  const uint16 TEMP_DOWN    = 0x0200;
  const uint16 POWER        = 0x0400;
  const uint16 TEMP_UP      = 0x1000;
  const uint16 TEMP_UNIT    = 0x2000;
  const uint16 HEATER       = 0x8000;
}

#endif

#ifdef CUSTOM_MODEL_NAME
const char MODEL_NAME[] = CUSTOM_MODEL_NAME;
#else
const char MODEL_NAME[] = DEFAULT_MODEL_NAME;
#endif  

namespace FRAME_DIGIT
{
  // bit mask of 7-segment display selector
  const uint16 POS_1 = 0x0040;
  const uint16 POS_2 = 0x0020;
  const uint16 POS_3 = 0x0800;
  const uint16 POS_4 = 0x0004;

  // bit mask of 7-segment display element
  const uint16 SEGMENT_A  = 0x2000;
  const uint16 SEGMENT_B  = 0x1000;
  const uint16 SEGMENT_C  = 0x0200;
  const uint16 SEGMENT_D  = 0x0400;
  const uint16 SEGMENT_E  = 0x0080;
  const uint16 SEGMENT_F  = 0x0008;
  const uint16 SEGMENT_G  = 0x0010;
  const uint16 SEGMENT_DP = 0x8000;
  const uint16 SEGMENTS   = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_F | SEGMENT_G;

  // bit mask of human readable value on 7-segment display
  const uint16 OFF   = 0x0000;
  const uint16 NUM_0 = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_F;
  const uint16 NUM_1 = SEGMENT_B | SEGMENT_C;
  const uint16 NUM_2 = SEGMENT_A | SEGMENT_B | SEGMENT_G | SEGMENT_E | SEGMENT_D;
  const uint16 NUM_3 = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_G;
  const uint16 NUM_4 = SEGMENT_F | SEGMENT_G | SEGMENT_B | SEGMENT_C;
  const uint16 NUM_5 = SEGMENT_A | SEGMENT_F | SEGMENT_G | SEGMENT_C | SEGMENT_D;
  const uint16 NUM_6 = SEGMENT_A | SEGMENT_F | SEGMENT_E | SEGMENT_D | SEGMENT_C | SEGMENT_G;
  const uint16 NUM_7 = SEGMENT_A | SEGMENT_B | SEGMENT_C;
  const uint16 NUM_8 = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_F | SEGMENT_G;
  const uint16 NUM_9 = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_F | SEGMENT_G;
  const uint16 LET_A = SEGMENT_E | SEGMENT_F | SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_G;
  const uint16 LET_C = SEGMENT_A | SEGMENT_F | SEGMENT_E | SEGMENT_D;
  const uint16 LET_D = SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_G;
  const uint16 LET_E = SEGMENT_A | SEGMENT_F | SEGMENT_E | SEGMENT_D | SEGMENT_G;
  const uint16 LET_F = SEGMENT_E | SEGMENT_F | SEGMENT_A | SEGMENT_G;
  const uint16 LET_H = SEGMENT_B | SEGMENT_C | SEGMENT_E | SEGMENT_F | SEGMENT_G;
  const uint16 LET_N = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_E | SEGMENT_F;
}

// frame type markers
namespace FRAME_TYPE
{
  const uint16 CUE    = 0x0100;
  const uint16 LED    = 0x4000;
  const uint16 DIGIT  = FRAME_DIGIT::POS_1 | FRAME_DIGIT::POS_2 | FRAME_DIGIT::POS_3 | FRAME_DIGIT::POS_4;

#if defined MODEL_SB_H20
  const uint16 BUTTON = CUE | FRAME_BUTTON::POWER | FRAME_BUTTON::FILTER | FRAME_BUTTON::HEATER | FRAME_BUTTON::BUBBLE | FRAME_BUTTON::TEMP_UP | FRAME_BUTTON::TEMP_DOWN | FRAME_BUTTON::TEMP_UNIT;
#elif defined MODEL_SJB_HS
  const uint16 BUTTON = CUE | FRAME_BUTTON::POWER | FRAME_BUTTON::FILTER | FRAME_BUTTON::HEATER | FRAME_BUTTON::BUBBLE | FRAME_BUTTON::TEMP_UP | FRAME_BUTTON::TEMP_DOWN | FRAME_BUTTON::TEMP_UNIT | FRAME_BUTTON::DISINFECTION | FRAME_BUTTON::JET;
#endif
}

namespace DIGIT
{
  // 7-segment display update control
  const uint8 POS_1     = 0x8;
  const uint8 POS_2     = 0x4;
  const uint8 POS_3     = 0x2;
  const uint8 POS_4     = 0x1;
  const uint8 POS_1_2   = POS_1 | POS_2;
  const uint8 POS_1_2_3 = POS_1 | POS_2 | POS_3;
  const uint8 POS_ALL   = POS_1 | POS_2 | POS_3 | POS_4;

  // ASCII values used to map non-numeric states of the 7-segment display
  const char OFF = ' ';
};

namespace ERROR
{
  // human readable error on display
  const char CODE_90[]    PROGMEM = "E90";
  const char CODE_91[]    PROGMEM = "E91";
  const char CODE_92[]    PROGMEM = "E92";
  const char CODE_94[]    PROGMEM = "E94";
  const char CODE_95[]    PROGMEM = "E95";
  const char CODE_96[]    PROGMEM = "E96";
  const char CODE_97[]    PROGMEM = "E97";
  const char CODE_99[]    PROGMEM = "E99";
  const char CODE_END[]   PROGMEM = "END";
  const char CODE_OTHER[] PROGMEM = "EXX";

  const unsigned int COUNT = 9;

  // English error messages
  const char EN_90[]    PROGMEM = "no water flow";
  const char EN_91[]    PROGMEM = "salt level too low";
  const char EN_92[]    PROGMEM = "salt level too high";
  const char EN_94[]    PROGMEM = "water temp too low";
  const char EN_95[]    PROGMEM = "water temp too high";
  const char EN_96[]    PROGMEM = "system error";
  const char EN_97[]    PROGMEM = "dry fire protection";
  const char EN_99[]    PROGMEM = "water temp sensor error";
  const char EN_END[]   PROGMEM = "heating aborted after 72h";
  const char EN_OTHER[] PROGMEM = "error";

  // German error messages
  const char DE_90[]    PROGMEM = "kein Wasserdurchfluss";
  const char DE_91[]    PROGMEM = "niedriges Salzniveau";
  const char DE_92[]    PROGMEM = "hohes Salzniveau";
  const char DE_94[]    PROGMEM = "Wassertemperatur zu niedrig";
  const char DE_95[]    PROGMEM = "Wassertemperatur zu hoch";
  const char DE_96[]    PROGMEM = "Systemfehler";
  const char DE_97[]    PROGMEM = "Trocken-Brandschutz";
  const char DE_99[]    PROGMEM = "Wassertemperatursensor defekt";
  const char DE_END[]   PROGMEM = "Heizbetrieb nach 72 h deaktiviert";
  const char DE_OTHER[] PROGMEM = "Störung";

  const char* const TEXT[3][COUNT+1] PROGMEM = {
                                                 { CODE_90, CODE_91, CODE_92, CODE_94, CODE_95, CODE_96, CODE_97, CODE_99, CODE_END, CODE_OTHER },
                                                 { EN_90,   EN_91,   EN_92,   EN_94,   EN_95,   EN_96,   EN_97,   EN_99,   EN_END,   EN_OTHER },
                                                 { DE_90,   DE_91,   DE_92,   DE_94,   DE_95,   DE_96,   DE_97,   DE_99,   DE_END,   DE_OTHER }
                                               };
}

// special display values
inline char display2LastDigit(uint32 v) { return (v >> 24) & 0xFFU; }
inline uint16 display2Num(uint32 v)     { return (((v & 0xFFU) - '0')*100) + ((((v >> 8) & 0xFFU) - '0')*10) + (((v >> 16) & 0xFFU) - '0'); }
inline uint32 display2Error(uint32 v)   { return v & 0x00FFFFFFU; }
inline bool displayIsTemp(uint32 v)     { return display2LastDigit(v) == 'C' || display2LastDigit(v) == 'F'; }
inline bool displayIsTime(uint32 v)     { return display2LastDigit(v) == 'H'; }
inline bool displayIsError(uint32 v)    { return (v & 0xFFU) == 'E'; }
inline bool displayIsBlank(uint32 v)    { return (v & 0x00FFFFFFU) == (' ' << 16) + (' ' << 8) + ' '; }

volatile PureSpaIO::State PureSpaIO::state;
volatile PureSpaIO::IsrState PureSpaIO::isrState;
volatile PureSpaIO::Buttons PureSpaIO::buttons;


// @TODO detect when latch signal stays low
// @TODO detect act temp change during error
// @TODO improve reliability of water temp change (counter auto repeat and too short press)
void PureSpaIO::setup(LANG language)
{
  this->language = language;

  pinMode(PIN::CLOCK, INPUT);
  pinMode(PIN::DATA,  INPUT);
  pinMode(PIN::LATCH, INPUT);

  attachInterruptArg(digitalPinToInterrupt(PIN::CLOCK), PureSpaIO::clockRisingISR, this, RISING);
}

PureSpaIO::MODEL PureSpaIO::getModel() const
{
  return model;
}

const char* PureSpaIO::getModelName() const
{
  return MODEL_NAME;
}

void PureSpaIO::loop()
{
  // device online check
  unsigned long now = millis();
  if (state.stateUpdated)
  {
    lastStateUpdateTime = now;
    state.online = true;
    state.stateUpdated = false;
  }
  else if (timeDiff(now, lastStateUpdateTime) > CYCLE::RECEIVE_TIMEOUT)
  {
    state.online = false;
  }
}

bool PureSpaIO::isOnline() const
{
  return state.online;
}

unsigned int PureSpaIO::getTotalFrames() const
{
  return state.frameCounter;
}

unsigned int PureSpaIO::getDroppedFrames() const
{
  return state.frameDropped;
}

/**
 * @return actual water temperatur [°C] 0..60 or UNDEF::INT if unknown
 */
int PureSpaIO::getActWaterTempCelsius() const
{
  return (state.waterTemp != UNDEF::UINT) ? convertDisplayToCelsius(state.waterTemp) : UNDEF::INT;
}

/**
 * @return desired water temperatur [°C] or UNDEF::INT if unknown
 *
 * note: value is undefined after power up until value is changed
 */
int PureSpaIO::getDesiredWaterTempCelsius() const
{
  return (state.desiredTemp != UNDEF::UINT) ? convertDisplayToCelsius(state.desiredTemp) : UNDEF::INT;
}

/**
 * @return disinfection duration [h] 0/3/5/8 h or UNDEF::INT if unknown
 *
 * note: value is undefined after power up until disinfection is activated
 */
int PureSpaIO::getDisinfectionTime() const
{
  return isDisinfectionOn() ? (state.disinfectionTime != UNDEF::UINT ? display2Num(state.disinfectionTime) : UNDEF::INT) : 0;
}

String PureSpaIO::getErrorCode() const
{
  memcpy((void*)errorBuffer, (void*)&state.error, 4);
  return errorBuffer;
}

String PureSpaIO::getErrorMessage(const String& errorCode) const
{
  if (errorCode.length())
  {
    // get error index
    unsigned int errorIndex = UINT_MAX;
    for (unsigned int i=0; i<ERROR::COUNT; i++)
    {
      String ec = FPSTR(ERROR::TEXT[(unsigned int)LANG::CODE][i]);
      if (errorCode == ec)
      {
        errorIndex = i;
        break;
      }
    }

    // get error translation
    if (errorIndex != UINT_MAX)
    {
      return FPSTR(ERROR::TEXT[(unsigned int)language][errorIndex]);
    }
    else
    {
      // undefined error
      return errorCode;
    }
  }
  else
  {
    // no error
    return errorCode;
  }
}

unsigned int PureSpaIO::getRawLedValue() const
{
  return (state.ledStatus != UNDEF::USHORT) ? state.ledStatus : UNDEF::USHORT;
}

uint8 PureSpaIO::isPowerOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::POWER) != 0) : UNDEF::BOOL;
}

uint8 PureSpaIO::isFilterOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::FILTER) != 0) : UNDEF::BOOL;
}

uint8 PureSpaIO::isBubbleOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::BUBBLE) != 0) : UNDEF::BOOL;
}

uint8 PureSpaIO::isHeaterOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & (FRAME_LED::HEATER_ON | FRAME_LED::HEATER_STANDBY)) != 0) : UNDEF::BOOL;
}

uint8 PureSpaIO::isHeaterStandby() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::HEATER_STANDBY) != 0) : UNDEF::BOOL;
}

uint8 PureSpaIO::isBuzzerOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::NO_BEEP) == 0) : UNDEF::BOOL;
}

uint8 PureSpaIO::isDisinfectionOn() const
{
#ifdef MODEL_SJB_HS
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::DISINFECTION) != 0) : UNDEF::BOOL;
#else
  return false;
#endif
}

uint8 PureSpaIO::isJetOn() const
{
#ifdef MODEL_SJB_HS
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::JET) != 0) : UNDEF::BOOL;
#else
  return false;
#endif
}

/**
 * set desired water temperature by performing button up or down actions
 * repeatedly depending on temperature delta
 *
 * notes:
 * - method will block until setting is completed
 * - WiFi is temporarily put to sleep to improve receive decoding reliability
 * - actual setpoint is not checked for verification because this
 *   would slow down the setpoint modification significantly
 *
 * @param temp water temperature setpoint [°C]
 */
void PureSpaIO::setDesiredWaterTempCelsius(int temp)
{
  if (temp >= WATER_TEMP::SET_MIN && temp <= WATER_TEMP::SET_MAX)
  {
    if (isPowerOn() && state.error == ERROR_NONE)
    {
#ifdef FORCE_WIFI_SLEEP
      // try to get initial temp
      WiFi.forceSleepBegin();
      int setTemp = getDesiredWaterTempCelsius();
      //DEBUG_MSG("\nBset %d", setTemp);
      bool modifying = false;
      if (setTemp == UNDEF::INT)
      {
        // trigger temp modification
        changeWaterTemp(-1);
        modifying = true;

        // wait for temp readback (will take 2-3 blink durations)
        int sleep = 20; // ms
        int tries = 4*BLINK::PERIOD/sleep;
        do
        {
          delay(sleep);
          setTemp = getDesiredWaterTempCelsius();
          tries--;
        } while (setTemp == UNDEF::INT && tries);

        // check success
        if (setTemp == UNDEF::INT)
        {
          // error, abort
          WiFi.forceSleepWake();
          delay(1);
          DEBUG_MSG("\naborted\n");
          return;
        }
      }

      // modify desired temp
      int deltaTemp = temp - setTemp;
      //DEBUG_MSG("\nBdelta %d", deltaTemp);
      while (deltaTemp)
      {
        ESP.wdtFeed();
        if (deltaTemp > 0)
        {
          //DEBUG_MSG("\nBU");
          changeWaterTemp(1);
          if (modifying)
          {
            deltaTemp--;
            setTemp++;
          }
        }
        else
        {
          //DEBUG_MSG("\nBD");
          changeWaterTemp(-1);
          if (modifying)
          {
            deltaTemp++;
            setTemp--;
          }
        }
        modifying = true;
      }

      WiFi.forceSleepWake();
      delay(1);
#else
      // trigger temp modification
      if (!changeWaterTemp(-1))
      {
        changeWaterTemp(+1);
      }

      int sleep = 5*CYCLE::PERIOD; // ms
      int changeTries = 3;
      int setTemp = UNDEF::INT;
      bool getActualSetpoint = true;
      do
      {
        // get actual temperature setpoint (will take 2-3 blink durations, especially inital) but skip when change has failed
        int readTries = 4*BLINK::PERIOD/sleep;
        int newSetTemp = getActualSetpoint? UNDEF::INT : setTemp;
        ESP.wdtFeed();
        if (getActualSetpoint)
        {
          // always wait after change, especially to catch double trigger
          waitBuzzerOff();
          delay(BLINK::PERIOD);
        }
        while (getActualSetpoint)
        {
          newSetTemp = getDesiredWaterTempCelsius();
          readTries--;
          getActualSetpoint = newSetTemp == setTemp && readTries;
          if (getActualSetpoint)
          {
            // only wait when not done
            delay(sleep);
          }
        }
        DEBUG_MSG("\nst:%d (rt:%d)", newSetTemp, readTries);

        // check success
        if (newSetTemp == UNDEF::INT)
        {
          // error, abort
          DEBUG_MSG("\naborted\n");
          return;
        }
        else
        {
          // update change tries based on inital delta
          if (setTemp == UNDEF::INT)
          {
            changeTries += abs(newSetTemp - temp);
            changeTries += changeTries/10;
          }

          // change temperature by 1 degree
          setTemp = newSetTemp;
          if (temp > setTemp)
          {
            getActualSetpoint = changeWaterTemp(+1);
            changeTries--;
          }
          else if (temp < setTemp)
          {
            getActualSetpoint = changeWaterTemp(-1);
            changeTries--;
          }
        }
      } while (temp != setTemp && changeTries);
      DEBUG_MSG("\ncT:%d", changeTries);
#endif
    }
  }
}

/**
 * set desired disinfection duration by performing button actions
 * repeatedly until the requested duration is displayed
 *
 * notes:
 * - method will block until setting is completed
 * - WiFi is temporarily put to sleep to improve receive decoding reliability
 *
 * @param hours disinfection duration 0/3/5/8 h
 */
void PureSpaIO::setDisinfectionTime(int hours)
{
  // use nearest available value
  if (hours > 5)      hours = 8;
  else if (hours > 3) hours = 5;
  else if (hours > 0) hours = 3;
  else                hours = 0;

  if (isPowerOn() && state.error == ERROR_NONE)
  {
#ifdef FORCE_WIFI_SLEEP
    WiFi.forceSleepBegin();
#endif

    int tries = 8;
    do
    {
      // get actual disinfection time
      int actHours = getDisinfectionTime();
      if (actHours == UNDEF::INT)
      {
        // error reading actual time, abort
        DEBUG_MSG("\naborted\n");
        break;
      }
      else if (actHours == hours)
      {
        // set and act time matches, done
        break;
      }

      // toggle disinfection time
      pressButton(buttons.toggleDisinfection);
      tries--;
    } while (tries);

#ifdef FORCE_WIFI_SLEEP
    WiFi.forceSleepWake();
    delay(1);
#endif
  }
}

/**
 * press specific button and wait for confirmation (blocking)
 *
 * notes:
 * - WiFi is temporarily put to sleep to improve receive decoding reliability
 *
 * @param buttonPressCount
 * @return true if beep was received, false if no beep was received until timeout
 */
bool PureSpaIO::pressButton(volatile unsigned int& buttonPressCount)
{
  waitBuzzerOff();
  unsigned int tries = BUTTON::ACK_TIMEOUT/BUTTON::ACK_CHECK_PERIOD;
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  buttonPressCount = BUTTON::PRESS_COUNT;
  while (buttonPressCount && tries)
  {
    delay(BUTTON::ACK_CHECK_PERIOD);
    tries--;
  }
  bool success = state.buzzer;
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  return success;
}

void PureSpaIO::setBubbleOn(bool on)
{
  if (on ^ (isBubbleOn() == true))
  {
    pressButton(buttons.toggleBubble);
  }
}

void PureSpaIO::setFilterOn(bool on)
{
  if (on ^ (isFilterOn() == true))
  {
    pressButton(buttons.toggleFilter);
  }
}

void PureSpaIO::setHeaterOn(bool on)
{
  if (on ^ (isHeaterOn() == true || isHeaterStandby() == true))
  {
    pressButton(buttons.toggleHeater);
  }
}

void PureSpaIO::setJetOn(bool on)
{
  if (on ^ (isJetOn() == true))
  {
    pressButton(buttons.toggleJet);
  }
}

void PureSpaIO::setPowerOn(bool on)
{
  bool active = isPowerOn() == true;
  if (on ^ active)
  {
    pressButton(buttons.togglePower);
  }
}

/**
 * wait for buzzer to go off or timeout
 * and delay for a cycle period
 *
 * @return true if buzzer is off, false if buzzer is still on after timeout
 */
bool PureSpaIO::waitBuzzerOff() const
{
  int tries = BUTTON::ACK_TIMEOUT/BUTTON::ACK_CHECK_PERIOD;
  while (state.buzzer && tries)
  {
    delay(BUTTON::ACK_CHECK_PERIOD);
    tries--;
  }

  // extra delay reduces chance to trigger auto repeat
  if (tries)
  {
    delay(2*CYCLE::PERIOD);
    return true;
  }
  else
  {
    DEBUG_MSG("\nwBO fail");
    return false;
  }
}

/**
 * change water temperature setpoint by 1 degree and wait for confirmation (blocking)
 *
 * notes:
 * - WiFi is temporarily put to sleep to improve receive decoding reliability
 *
 * @param up press up (> 0) or down (< 0) button
 * @return true if beep was received, false if no beep was received until timeout
 */
bool PureSpaIO::changeWaterTemp(int up)
{
  bool success = false;

  if (isPowerOn() && state.error == ERROR_NONE)
  {
    //DEBUG_MSG("\nP ");
    waitBuzzerOff();

#ifndef FORCE_WIFI_SLEEP
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
#endif

    // perform button action
    int tries = BUTTON::PRESS_SHORT_COUNT*CYCLE::PERIOD/BUTTON::ACK_CHECK_PERIOD;
    if (up > 0)
    {
      buttons.toggleTempUp = BUTTON::PRESS_SHORT_COUNT;
      while (buttons.toggleTempUp && tries)
      {
        delay(BUTTON::ACK_CHECK_PERIOD);
        tries--;
      }
      buttons.toggleTempUp = 0;
    }
    else if (up < 0)
    {
      buttons.toggleTempDown = BUTTON::PRESS_SHORT_COUNT;
      while (buttons.toggleTempDown && tries)
      {
        delay(BUTTON::ACK_CHECK_PERIOD);
        tries--;
      }
      buttons.toggleTempDown = 0;
    }

    // wait for buzzer on
    tries = (BUTTON::PRESS_COUNT - BUTTON::PRESS_SHORT_COUNT)*CYCLE::PERIOD/BUTTON::ACK_CHECK_PERIOD;
    while (!state.buzzer && tries)
    {
      delay(BUTTON::ACK_CHECK_PERIOD);
      tries--;
    }

    success = state.buzzer;

#ifndef FORCE_WIFI_SLEEP
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
#endif

    if (!success)
    {
      DEBUG_MSG("\ncWT fail");
    }
  }

  return success;
}

int PureSpaIO::convertDisplayToCelsius(uint32 value) const
{
  int celsiusValue = display2Num(value);
  char tempUnit = display2LastDigit(value);
  //DEBUG_MSG("\nDtC 0x%x =  %d%c", value, celsiusValue, tempUnit);
  if (tempUnit == 'F')
  {
    // convert °F to °C
    float fValue = (float)celsiusValue;
    celsiusValue = (int)round(((fValue - 32) * 5) / 9);
  }
  else if (tempUnit != 'C')
  {
    celsiusValue = UNDEF::INT;
  }

  return (celsiusValue >= 0) && (celsiusValue <= 60) ? celsiusValue : UNDEF::INT;
}

IRAM_ATTR void PureSpaIO::clockRisingISR(void* arg)
{
  bool data = !digitalRead(PIN::DATA);
  bool enabled = !digitalRead(PIN::LATCH);

  if (enabled || isrState.receivedBits == (FRAME::BITS - 1))
  {
    isrState.frameValue = (isrState.frameValue << 1) + data;
    isrState.receivedBits++;

    if (isrState.receivedBits == FRAME::BITS)
    {
      state.frameCounter++;
      if (isrState.frameValue == FRAME_TYPE::CUE)
      {
        // cue frame, ignore
        //DEBUG_MSG("\nC");
      }
      else if (isrState.frameValue & FRAME_TYPE::DIGIT)
      {
        // display frame
        //DEBUG_MSG("\nD");
        decodeDisplay();
      }
      else if (isrState.frameValue & FRAME_TYPE::LED)
      {
        // LED frame
        //DEBUG_MSG("\nL");
        decodeLED();
      }
      else if (isrState.frameValue & FRAME_TYPE::BUTTON)
      {
        // button frame
        //DEBUG_MSG("\nB");
        decodeButton();
      }
      else if (isrState.frameValue != 0)
      {
        // unsupported frame
        //DEBUG_MSG("\nU");
      }

      isrState.receivedBits = 0;
    }
  }
  else
  {
    //DEBUG_MSG(" %d ", receivedBits);
    isrState.receivedBits = 0;
    state.frameCounter++;
  }
}

IRAM_ATTR inline void PureSpaIO::decodeDisplay()
{
  char digit;
  switch (isrState.frameValue & FRAME_DIGIT::SEGMENTS)
  {
    case FRAME_DIGIT::OFF:
      digit = DIGIT::OFF;
      break;
    case FRAME_DIGIT::NUM_0:
      digit = '0';
      break;
    case FRAME_DIGIT::NUM_1:
      digit = '1';
      break;
    case FRAME_DIGIT::NUM_2:
      digit = '2';
      break;
    case FRAME_DIGIT::NUM_3:
      digit = '3';
      break;
    case FRAME_DIGIT::NUM_4:
      digit = '4';
      break;
    case FRAME_DIGIT::NUM_5:
      digit = '5';
      break;
    case FRAME_DIGIT::NUM_6:
      digit = '6';
      break;
    case FRAME_DIGIT::NUM_7:
      digit = '7';
      break;
    case FRAME_DIGIT::NUM_8:
      digit = '8';
      break;
    case FRAME_DIGIT::NUM_9:
      digit = '9';
      break;
    case FRAME_DIGIT::LET_C:
      digit = 'C'; // temp unit °C
      break;
    case FRAME_DIGIT::LET_D:
      digit = 'D';
      break;
    case FRAME_DIGIT::LET_E:
      digit = 'E';
      break;
    case FRAME_DIGIT::LET_F:
      digit = 'F'; // temp unit °F
      break;
    case FRAME_DIGIT::LET_H:
      digit = 'H';
      break;
    case FRAME_DIGIT::LET_N:
      digit = 'N';
      break;

    default:
      // unsupported, ignore
      return;
  }

  switch (isrState.frameValue & FRAME_TYPE::DIGIT)
  {
    case FRAME_DIGIT::POS_1:
      //DEBUG_MSG("1");
      isrState.displayValue = (isrState.displayValue & 0xFFFFFF00U) + digit;
      isrState.receivedDigits = DIGIT::POS_1;
      break;

    case FRAME_DIGIT::POS_2:
      //DEBUG_MSG("2");
      if (isrState.receivedDigits == DIGIT::POS_1)
      {
        isrState.displayValue = (isrState.displayValue & 0xFFFF00FFU) + (digit << 8);
        isrState.receivedDigits |= DIGIT::POS_2;
      }
      break;

    case FRAME_DIGIT::POS_3:
      //DEBUG_MSG("3");
      if (isrState.receivedDigits == DIGIT::POS_1_2)
      {
        isrState.displayValue = (isrState.displayValue & 0xFF00FFFFU) + (digit << 16);
        isrState.receivedDigits |= DIGIT::POS_3;
      }
      break;

    case FRAME_DIGIT::POS_4:
      //DEBUG_MSG("4");
      if (isrState.receivedDigits == DIGIT::POS_1_2_3)
      {
        isrState.displayValue = (isrState.displayValue & 0x00FFFFFFU) + (digit << 24);
        isrState.receivedDigits = DIGIT::POS_ALL;
      }
      break;
  }

  if (isrState.receivedDigits == DIGIT::POS_ALL)
  {
    if (isrState.displayValue == isrState.latestDisplayValue)
    {
      // display is stable, might be blinking
      //DEBUG_MSG(" s%x", isrState.displayValue);
      isrState.stableDisplayValueCount--;
      if (isrState.stableDisplayValueCount == 0)
      {
        //DEBUG_MSG(" C"); // confirmed
        isrState.stableDisplayValueCount = CONFIRM_FRAMES::REGULAR;
        if (isrState.isDisplayBlinking)
        {
          //DEBUG_MSG("B");
          if (diff(state.frameCounter, isrState.lastBlankDisplayFrameCounter) > BLINK::STOPPED_FRAMES)
          {
            // blinking is over, clear desired temp
            //DEBUG_MSG("b");
            isrState.isDisplayBlinking = false;
            isrState.latestBlinkingTemp = UNDEF::UINT;
          }
        }

        if (!displayIsError(isrState.displayValue))
        {
          // display does not show an error
          //DEBUG_MSG("e");
#ifdef MODEL_SJB_HS
          if (displayIsTime(isrState.displayValue))
          {
            // display shows a time
            //DEBUG_MSG("C");
            if (isrState.displayValue == isrState.latestDisinfectionTime)
            {
              // new time is stable
              //DEBUG_MSG("C%d", stableWaterTempCount);
              isrState.stableDisinfectionTimeCount--;
              if (isrState.stableDisinfectionTimeCount == 0)
              {
                // save time
                if (state.disinfectionTime != isrState.displayValue)
                {
                  //DEBUG_MSG(" AC ");
                  state.disinfectionTime = isrState.displayValue;
                }

                isrState.stableDisinfectionTimeCount = CONFIRM_FRAMES::REGULAR;
              }
            }
            else
            {
              // time has changed
              //DEBUG_MSG("c");
              isrState.latestDisinfectionTime = isrState.displayValue;
              isrState.stableWaterTempCount = CONFIRM_FRAMES::NOT_BLINKING;
            }
          }
          else
#endif
          {
            if (displayIsTemp(isrState.displayValue))
            {
              // display shows a temperature
              //DEBUG_MSG("T");
              if (isrState.isDisplayBlinking)
              {
                // display is blinking
                //DEBUG_MSG("B");
                if (isrState.displayValue == isrState.latestBlinkingTemp)
                {
                  // blinking temp is stable
                  isrState.stableBlinkingWaterTempCount++;
                  //DEBUG_MSG("DS ");
                }
                else if (diff(state.frameCounter, isrState.lastBlankDisplayFrameCounter) < BLINK::TEMP_FRAMES)
                {
                  // blinking temp has changed (is read after a blank screen and set at next black screen)
                  //DEBUG_MSG("DC ");
                  isrState.latestBlinkingTemp = isrState.displayValue;
                  isrState.stableBlinkingWaterTempCount = 0;
                }
              }
              else
              {
                // display is not blinking
                //DEBUG_MSG("b");
                if (isrState.displayValue == isrState.latestWaterTemp)
                {
                  // new actual temp is stable
                  //DEBUG_MSG("A ");
                  isrState.stableWaterTempCount--;
                  if (isrState.stableWaterTempCount == 0)
                  {
                    // save actual temp
                    if (state.waterTemp != isrState.displayValue)
                    {
                      //DEBUG_MSG(" T");
                      state.waterTemp = isrState.displayValue;
                    }

                    isrState.stableWaterTempCount = CONFIRM_FRAMES::NOT_BLINKING;
                  }
                }
                else
                {
                  // actual temp is changed
                  //DEBUG_MSG("a ");
                  isrState.latestWaterTemp = isrState.displayValue;
                  isrState.stableWaterTempCount = CONFIRM_FRAMES::NOT_BLINKING;
                }
              }
            }
            else
            {
              // unsupported display state (no error, no temperature)
              //DEBUG_MSG("t ");
            }
          }
        }
        else
        {
          // display shows error code
          state.error = display2Error(isrState.displayValue);
        }
      }
    }
    else if (displayIsBlank(isrState.displayValue))
    {
      // display is blank
      if (isrState.stableDisplayBlankCount)
      {
        isrState.stableDisplayBlankCount--;
      }
      else
      {
        // display is blank
        //DEBUG_MSG("B");
        if (isrState.isDisplayBlinking)
        {
          // already blinking
          if (isrState.latestBlinkingTemp != UNDEF::UINT)
          {
            // new temp
            //DEBUG_MSG("bc ");
            isrState.blankCounter++;
          }

          // if display was already blinking several times, save desired temp
          // otherwise could be start of error
          if (state.error == ERROR_NONE && isrState.blankCounter > 2
              && isrState.stableBlinkingWaterTempCount >= CONFIRM_FRAMES::REGULAR
              && state.desiredTemp != isrState.latestBlinkingTemp)
          {
            //DEBUG_MSG("\nDT%x ", isrState.displayValue);
            state.desiredTemp = isrState.latestBlinkingTemp;
          }

          isrState.latestBlinkingTemp = UNDEF::UINT;
          isrState.stableBlinkingWaterTempCount = 0;
        }
        else
        {
          // blinking start
          isrState.isDisplayBlinking = true;
          isrState.blankCounter = 0;
        }
        isrState.lastBlankDisplayFrameCounter = state.frameCounter;
      }
    }
    else
    {
      // display value changed
      isrState.latestDisplayValue = isrState.displayValue;
      isrState.stableDisplayValueCount = CONFIRM_FRAMES::REGULAR;
      isrState.stableDisplayBlankCount = CONFIRM_FRAMES::REGULAR;
    }
  }
  // else not all digits set yet
}

ICACHE_RAM_ATTR inline void PureSpaIO::decodeLED()
{
  if (isrState.frameValue == isrState.latestLedStatus)
  {
    // wait for confirmation
    isrState.stableLedStatusCount--;
    if (isrState.stableLedStatusCount == 0)
    {
      //DEBUG_MSG("\nL%x", frameValue);
      state.ledStatus = isrState.frameValue;
      state.buzzer = !(state.ledStatus & FRAME_LED::NO_BEEP);
      state.stateUpdated = true;
      isrState.stableLedStatusCount = CONFIRM_FRAMES::REGULAR;

      // clear buttons if buzzer is on
      if (state.buzzer)
      {
        buttons.toggleBubble = 0;
        buttons.toggleDisinfection = 0;
        buttons.toggleFilter = 0;
        buttons.toggleHeater = 0;
        buttons.toggleJet = 0;
        buttons.togglePower = 0;
        buttons.toggleTempUp = 0;
        buttons.toggleTempDown = 0;
      }
    }
  }
  else
  {
    // LED status changed
    isrState.latestLedStatus = isrState.frameValue;
    isrState.stableLedStatusCount = CONFIRM_FRAMES::REGULAR;
  }
}

IRAM_ATTR inline void PureSpaIO::updateButtonState(volatile unsigned int& buttonPressCount)
{
  if (buttonPressCount)
  {
    if (state.buzzer)
    {
      buttonPressCount = 0;
    }
    else
    {
      isrState.reply = true;
      buttonPressCount--;
    }
  }
}

IRAM_ATTR inline void PureSpaIO::decodeButton()
{
  if (isrState.frameValue & FRAME_BUTTON::FILTER)
  {
    //DEBUG_MSG("F");
    updateButtonState(buttons.toggleFilter);
  }
  else if (isrState.frameValue & FRAME_BUTTON::HEATER)
  {
    //DEBUG_MSG("H");
    updateButtonState(buttons.toggleHeater);
  }
  else if (isrState.frameValue & FRAME_BUTTON::BUBBLE)
  {
    //DEBUG_MSG("B");
    updateButtonState(buttons.toggleBubble);
  }
  else if (isrState.frameValue & FRAME_BUTTON::POWER)
  {
    //DEBUG_MSG(" P");
    updateButtonState(buttons.togglePower);
  }
  else if (isrState.frameValue & FRAME_BUTTON::TEMP_UP)
  {
    //DEBUG_MSG("U");
    updateButtonState(buttons.toggleTempUp);
  }
  else if (isrState.frameValue & FRAME_BUTTON::TEMP_DOWN)
  {
    //DEBUG_MSG("D");
    updateButtonState(buttons.toggleTempDown);
  }
#ifdef MODEL_SJB_HS
  else if (isrState.frameValue & FRAME_BUTTON::DISINFECTION)
  {
    updateButtonState(buttons.toggleDisinfection);
  }
  else if (isrState.frameValue & FRAME_BUTTON::JET)
  {
    updateButtonState(buttons.toggleJet);
  }
#endif
  else if (isrState.frameValue & FRAME_BUTTON::TEMP_UNIT)
  {
    //DEBUG_MSG("T");
  }
  else
  {
    //DEBUG_MSG(" B%x", frameValue);
  }

  if (isrState.reply)
  {
    // delay around 5 µs relative to rising edge of latch signal before pulsing
    // pulse should be around 2 µs and MUST be completed BEFORE next falling edge of clock
#if F_CPU == 160000000L
    delayMicroseconds(1);
    pinMode(PIN::DATA, OUTPUT);
    delayMicroseconds(3);
    pinMode(PIN::DATA, INPUT);
#else
    #error 160 MHz CPU frequency required! Pulse timing not possible at 80 MHz, because the code above takes too long to reach this point.
    // at least using Arduino methods
#endif
    isrState.reply = false;
  }
}
