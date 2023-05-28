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
  const uint16 POWER     = 0x0400;
  const uint16 FILTER    = 0x0002;
  const uint16 HEATER    = 0x8000;
  const uint16 BUBBLE    = 0x0008;
  const uint16 TEMP_UP   = 0x1000;
  const uint16 TEMP_DOWN = 0x0080;
  const uint16 TEMP_UNIT = 0x2000;
}

#elif defined MODEL_SJB_HS

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

  // nibble value used to map a subset of non-numeric states of the 7-segment display
  const uint8 LET_C = 0xC;
  const uint8 LET_D = 0xD;
  const uint8 LET_E = 0xE;
  const uint8 LET_F = 0xF;

  const uint8 LET_N = 0xA;
  const uint8 OFF   = 0xB;
};

namespace ERROR
{
  // internal binary value of error display (3 letters)
  const uint16 NONE             = 0;
  const uint16 NO_WATER_FLOW    = (DIGIT::LET_E << 8) | (9 << 4) | 0;
  const uint16 SALT_LEVEL_LOW   = (DIGIT::LET_E << 8) | (9 << 4) | 1;
  const uint16 SALT_LEVEL_HIGH  = (DIGIT::LET_E << 8) | (9 << 4) | 2;
  const uint16 WATER_TEMP_LOW   = (DIGIT::LET_E << 8) | (9 << 4) | 4;
  const uint16 WATER_TEMP_HIGH  = (DIGIT::LET_E << 8) | (9 << 4) | 5;
  const uint16 SYSTEM           = (DIGIT::LET_E << 8) | (9 << 4) | 6;
  const uint16 DRY_FIRE_PROTECT = (DIGIT::LET_E << 8) | (9 << 4) | 7;
  const uint16 TEMP_SENSOR      = (DIGIT::LET_E << 8) | (9 << 4) | 9;
  const uint16 HEATING_ABORTED  = (DIGIT::LET_E << 8) | (DIGIT::LET_N << 4) | DIGIT::LET_D;

  const uint16 VALUES[] = { NO_WATER_FLOW, SALT_LEVEL_LOW, SALT_LEVEL_HIGH, WATER_TEMP_LOW, WATER_TEMP_HIGH, WATER_TEMP_HIGH, SYSTEM, DRY_FIRE_PROTECT, TEMP_SENSOR, HEATING_ABORTED };
  const unsigned int COUNT = sizeof(VALUES)/sizeof(uint16);

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
inline uint8 display2Byte(uint16 v)    { return v & 0x000F; }
inline uint16 display2Num(uint16 v)   { return (((v >> 12) & 0x000F)*100) + (((v >> 8) & 0x000F)*10) + ((v >> 4) & 0x000F); }
inline uint16 display2Error(uint16 v) { return (v >> 4) & 0x0FFF; }
inline bool displayIsTemp(uint16 v)   { return display2Byte(v) == DIGIT::LET_C || display2Byte(v) == DIGIT::LET_F; }
inline bool displayIsError(uint16 v)  { return (v & 0xF000) == 0xE000; }
inline bool displayIsBlank(uint16 v)  { return (v & 0xFFF0) == ((DIGIT::OFF << 12) + (DIGIT::OFF << 8) + (DIGIT::OFF <<4)); }

volatile PureSpaIO::State PureSpaIO::state;
volatile PureSpaIO::IsrState PureSpaIO::isrState;
volatile PureSpaIO::Buttons PureSpaIO::buttons;


// @TODO detect when latch signal stays low
// @TODO detect act temp change during error
// @TODO improve reliability of water temp change (counter auto repeat and too short press)
void PureSpaIO::setup(LANG language)
{
#if defined MODEL_SB_H20
  model = MODEL::SBH20;
#elif defined MODEL_SJB_HS
  model = MODEL::SJBHS;
#else
  #error no model (MODEL_SB_H20 or MODEL_SJB_HS) selected in common.h
#endif

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
  switch (model)
  {
    case MODEL::SBH20:
      return PSTR("Intex PureSpa SB-H20");
    case MODEL::SJBHS:
      return PSTR("Intex PureSpa SJB-HS");
    default:
      return PSTR("unsupported Intex PureSpa model");
  }
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

int PureSpaIO::getActWaterTempCelsius() const
{
  return (state.waterTemp != UNDEF::USHORT) ? convertDisplayToCelsius(state.waterTemp) : UNDEF::USHORT;
}

int PureSpaIO::getDesiredWaterTempCelsius() const
{
  return (state.desiredTemp != UNDEF::USHORT) ? convertDisplayToCelsius(state.desiredTemp) : UNDEF::USHORT;
}

unsigned int PureSpaIO::getErrorValue() const
{
  return state.error;
}

String PureSpaIO::getErrorMessage(unsigned int errorValue) const
{
  if (errorValue)
  {
    // get error text index of error value
    unsigned int i;
    for (i=0; i<ERROR::COUNT; i++)
    {
      if (ERROR::VALUES[i] == errorValue)
      {
        break;
      }
    }

    // load error text from PROGMEM
    return FPSTR(ERROR::TEXT[(unsigned int)language][i]);
  }
  else
  {
    // no error
    return "";
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
    if (isPowerOn() == true && state.error == ERROR::NONE)
    {
      // try to get initial temp
      WiFi.forceSleepBegin();
      int setTemp = getDesiredWaterTempCelsius();
      bool modifying = false;
      if (setTemp == UNDEF::USHORT)
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
        } while (setTemp == UNDEF::USHORT && tries);

        // check success
        if (setTemp == UNDEF::USHORT)
        {
          // error, abort
          DEBUG_MSG("\naborted\n");
          WiFi.forceSleepWake();
          delay(1);
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
    }
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
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  //WiFi.forceSleepBegin();
  waitBuzzerOff();
  unsigned int tries = BUTTON::ACK_TIMEOUT/BUTTON::ACK_CHECK_PERIOD;
  buttonPressCount = BUTTON::PRESS_COUNT;
  while (buttonPressCount && tries)
  {
    delay(BUTTON::ACK_CHECK_PERIOD);
    tries--;
  }
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  //WiFi.forceSleepWake();
  //delay(1);

  return tries;
}

void PureSpaIO::setBubbleOn(bool on)
{
  if (on ^ (isBubbleOn() == true))
  {
    pressButton(buttons.toggleBubble);
  }
}

void PureSpaIO::setDisinfectionOn(bool on)
{
  if (on ^ (isDisinfectionOn() == true))
  {
    pressButton(buttons.toggleDisinfection);
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
  if (on ^ (isDisinfectionOn() == true))
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
 * @param up press up (> 0) or down (< 0) button
 * @return true if beep was received, false if no beep was received until timeout
 */
bool PureSpaIO::changeWaterTemp(int up)
{
  if (isPowerOn() == true && state.error == ERROR::NONE)
  {
    // perform button action
    waitBuzzerOff();
    //DEBUG_MSG("\nP ");
    int tries = BUTTON::ACK_TIMEOUT/BUTTON::ACK_CHECK_PERIOD;
    if (up > 0)
    {
      buttons.toggleTempUp = BUTTON::PRESS_COUNT;
      while (buttons.toggleTempUp && tries)
      {
        delay(BUTTON::ACK_CHECK_PERIOD);
        tries--;
      }
    }
    else if (up < 0)
    {
      buttons.toggleTempDown = BUTTON::PRESS_COUNT;
      while (buttons.toggleTempDown && tries)
      {
        delay(BUTTON::ACK_CHECK_PERIOD);
        tries--;
      }
    }

    if (tries && state.buzzer)
    {
      return true;
    }
    else
    {
      DEBUG_MSG("\ncWT fail");
    }
  }

  return false;
}

uint16 PureSpaIO::convertDisplayToCelsius(uint16 value) const
{
  uint16 celsiusValue = display2Num(value);
  uint16 tempUint = display2Byte(value);
  if (tempUint == DIGIT::LET_F)
  {
    // convert °F to °C
    float fValue = (float)celsiusValue;
    celsiusValue = (uint16)round(((fValue - 32) * 5) / 9);
  }
  else if (tempUint != DIGIT::LET_C)
  {
    celsiusValue = UNDEF::USHORT;
  }

  return (celsiusValue >= 0) && (celsiusValue <= 60) ? celsiusValue : UNDEF::USHORT;
}

ICACHE_RAM_ATTR void PureSpaIO::clockRisingISR(void* arg)
{
  bool data = !digitalRead(PIN::DATA);
  bool enable = digitalRead(PIN::LATCH) == LOW;

/*
  PureSpaIO* sbh20io = (PureSpaIO*)arg;
  volatile State& state = sbh20io->state;
  volatile IsrState& isrState = sbh20io->isrState;
*/

  if (enable || isrState.receivedBits == (FRAME::BITS - 1))
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

ICACHE_RAM_ATTR inline void PureSpaIO::decodeDisplay()
{
  uint8 digit;
  switch (isrState.frameValue & FRAME_DIGIT::SEGMENTS)
  {
    case FRAME_DIGIT::OFF:
      digit = DIGIT::OFF;
      break;
    case FRAME_DIGIT::NUM_0:
      digit = 0x0;
      break;
    case FRAME_DIGIT::NUM_1:
      digit = 0x1;
      break;
    case FRAME_DIGIT::NUM_2:
      digit = 0x2;
      break;
    case FRAME_DIGIT::NUM_3:
      digit = 0x3;
      break;
    case FRAME_DIGIT::NUM_4:
      digit = 0x4;
      break;
    case FRAME_DIGIT::NUM_5:
      digit = 0x5;
      break;
    case FRAME_DIGIT::NUM_6:
      digit = 0x6;
      break;
    case FRAME_DIGIT::NUM_7:
      digit = 0x7;
      break;
    case FRAME_DIGIT::NUM_8:
      digit = 0x8;
      break;
    case FRAME_DIGIT::NUM_9:
      digit = 0x9;
      break;
    case FRAME_DIGIT::LET_C:
      digit = DIGIT::LET_C; // for °C
      break;
    case FRAME_DIGIT::LET_D:
      digit = DIGIT::LET_D; // for error code "END"
      break;
    case FRAME_DIGIT::LET_E:
      digit = DIGIT::LET_E; // for error code
      break;
    case FRAME_DIGIT::LET_F:
      digit = DIGIT::LET_F; // for °F
      break;
    case FRAME_DIGIT::LET_N:
      digit = DIGIT::LET_N; // for error code "END"
      break;

    default:
      // unsupported, ignore
      return;
  }

  switch (isrState.frameValue & FRAME_TYPE::DIGIT)
  {
    case FRAME_DIGIT::POS_1:
      //DEBUG_MSG("1");
      isrState.displayValue = (isrState.displayValue & 0x0FFF) + (digit << 12);
      isrState.receivedDigits = DIGIT::POS_1;
      break;

    case FRAME_DIGIT::POS_2:
      //DEBUG_MSG("2");
      if (isrState.receivedDigits == DIGIT::POS_1)
      {
        isrState.displayValue = (isrState.displayValue & 0xF0FF) + (digit << 8);
        isrState.receivedDigits |= DIGIT::POS_2;
      }
      break;

    case FRAME_DIGIT::POS_3:
      //DEBUG_MSG("3");
      if (isrState.receivedDigits == DIGIT::POS_1_2)
      {
        isrState.displayValue = (isrState.displayValue & 0xFF0F) + (digit << 4);
        isrState.receivedDigits |= DIGIT::POS_3;
      }
      break;

    case FRAME_DIGIT::POS_4:
      //DEBUG_MSG("4");
      if (isrState.receivedDigits == DIGIT::POS_1_2_3)
      {
        isrState.displayValue = (isrState.displayValue & 0xFFF0) + digit;
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
        //DEBUG_MSG("S%x ", isrState.displayValue);
        isrState.stableDisplayValueCount = CONFIRM_FRAMES::DISP;
        //DEBUG_MSG("O");
        if (isrState.isDisplayBlinking)
        {
          if (diff(state.frameCounter, isrState.lastBlankDisplayFrameCounter) > BLINK::STOPPED_FRAMES)
          {
            // blinking is over, clear desired temp
            //DEBUG_MSG("b");
            isrState.isDisplayBlinking = false;
            isrState.latestBlinkingTemp = UNDEF::USHORT;
          }
        }

        if (!displayIsError(isrState.displayValue))
        {
          // display does not show an error
          //DEBUG_MSG("e");
          if (displayIsTemp(isrState.displayValue))
          {
            // display shows a temperature
            //DEBUG_MSG("T");
            if (isrState.isDisplayBlinking)
            {
              // display is blinking
              if (isrState.displayValue == isrState.latestBlinkingTemp)
              {
                // blinking temp is stable
                isrState.stableBlinkingWaterTempCount++;
                //DEBUG_MSG(" DS%d ", isrState.stableDesiredWaterTempCount);
              }
              else if (diff(state.frameCounter, isrState.lastBlankDisplayFrameCounter) < BLINK::TEMP_FRAMES)
              {
                // blinking temp has changed (is read after a blank screen and set at next black screen)
                //DEBUG_MSG(" DC%x ", isrState.displayValue);
                isrState.latestBlinkingTemp = isrState.displayValue;
                isrState.stableBlinkingWaterTempCount = 0;
              }
            }
            else
            {
              // display is not blinking
              if (isrState.displayValue == isrState.latestWaterTemp)
              {
                // new actual temp is stable
                //DEBUG_MSG("A%d", stableWaterTempCount);
                isrState.stableWaterTempCount--;
                if (isrState.stableWaterTempCount == 0)
                {
                  // save actual temp
                  if (state.waterTemp != isrState.displayValue)
                  {
                    //DEBUG_MSG(" AT ");
                    state.waterTemp = isrState.displayValue;
                  }

                  // get temp unit
                  uint16 tempUnit = display2Byte(isrState.displayValue);
                  if (tempUnit != isrState.latestTempUnit)
                  {
                    isrState.latestTempUnit = tempUnit;
                  }

                  isrState.stableWaterTempCount = CONFIRM_FRAMES::WATER_TEMP_ACT;

                  // clear error
                  //state.error = ERROR::NONE;
                }
              }
              else
              {
                // actual temp is changed
                //DEBUG_MSG("a");
                isrState.latestWaterTemp = isrState.displayValue;
                isrState.stableWaterTempCount = CONFIRM_FRAMES::WATER_TEMP_ACT;
              }
            }
          }
          else
          {
            // unsupported display state (no error, no temperature)
            //DEBUG_MSG("t");
          }
        }
        else
        {
          // display shows error code
          //DEBUG_MSG("E");
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
          if (isrState.latestBlinkingTemp != UNDEF::USHORT)
          {
            // new temp
            isrState.blankCounter++;
          }

          // if display was already blinking several times, save desired temp
          // otherwise could be start of error
          if (!state.error && isrState.blankCounter > 2
              && isrState.stableBlinkingWaterTempCount >= CONFIRM_FRAMES::WATER_TEMP_SET
              && state.desiredTemp != isrState.latestBlinkingTemp)
          {
            //DEBUG_MSG("\nDT%x ", isrState.displayValue);
            state.desiredTemp = isrState.latestBlinkingTemp;
          }

          isrState.latestBlinkingTemp = UNDEF::USHORT;
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
      isrState.stableDisplayValueCount = CONFIRM_FRAMES::DISP;
      isrState.stableDisplayBlankCount = CONFIRM_FRAMES::DISP;
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
      isrState.stableLedStatusCount = CONFIRM_FRAMES::LED;

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
    isrState.stableLedStatusCount = CONFIRM_FRAMES::LED;
  }
}

ICACHE_RAM_ATTR inline void PureSpaIO::decodeButton()
{
  if (isrState.frameValue & FRAME_BUTTON::FILTER)
  {
    //DEBUG_MSG("F");
    if (buttons.toggleFilter)
    {
      isrState.reply = true;
      buttons.toggleFilter--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::HEATER)
  {
    //DEBUG_MSG("H");
    if (buttons.toggleHeater)
    {
      isrState.reply = true;
      buttons.toggleHeater--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::BUBBLE)
  {
    //DEBUG_MSG("B");
    if (buttons.toggleBubble)
    {
      isrState.reply = true;
      buttons.toggleBubble--;
      if (!buttons.toggleBubble)
      {
        DEBUG_MSG("\nFBO");
      }
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::POWER)
  {
    //DEBUG_MSG(" P");
    if (buttons.togglePower)
    {
      isrState.reply = true;
      buttons.togglePower--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::TEMP_UP)
  {
    //DEBUG_MSG("U");
    if (buttons.toggleTempUp)
    {
      isrState.reply = true;
      buttons.toggleTempUp--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::TEMP_DOWN)
  {
    //DEBUG_MSG("D");
    if (buttons.toggleTempDown)
    {
      isrState.reply = true;
      buttons.toggleTempDown--;
    }
  }
#ifdef MODEL_SJB_HS
  else if (isrState.frameValue & FRAME_BUTTON::DISINFECTION)
  {
    if (buttons.toggleDisinfection)
    {
      isrState.reply = true;
      buttons.toggleDisinfection--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::JET)
  {
    if (buttons.toggleJet)
    {
      isrState.reply = true;
      buttons.toggleJet--;
    }
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
    // pulse should be around 2 µs and must be completed before next falling edge of clock
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
