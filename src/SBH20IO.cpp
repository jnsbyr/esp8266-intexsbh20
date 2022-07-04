/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     SBH20IO.cpp
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

#include "SBH20IO.h"
#include "esp32-hal.h"

// bit mask for LEDs
namespace FRAME_LED
{
  const uint16_t POWER = 0x0001;
  const uint16_t HEATER_ON = 0x0080; // max. 72 h, will start filter, will not stop filter
  const uint16_t NO_BEEP = 0x0100;
  const uint16_t HEATER_STANDBY = 0x0200;
  const uint16_t BUBBLE = 0x0400; // max. 30 min
  const uint16_t FILTER = 0x1000; // max. 24 h
}

namespace FRAME_DIGIT
{
  // bit mask of 7-segment display selector
  const uint16_t POS_1 = 0x0040;
  const uint16_t POS_2 = 0x0020;
  const uint16_t POS_3 = 0x0800;
  const uint16_t POS_4 = 0x0004;

  // bit mask of 7-segment display element
  const uint16_t SEGMENT_A = 0x2000;
  const uint16_t SEGMENT_B = 0x1000;
  const uint16_t SEGMENT_C = 0x0200;
  const uint16_t SEGMENT_D = 0x0400;
  const uint16_t SEGMENT_E = 0x0080;
  const uint16_t SEGMENT_F = 0x0008;
  const uint16_t SEGMENT_G = 0x0010;
  const uint16_t SEGMENT_DP = 0x8000;
  const uint16_t SEGMENTS = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_F | SEGMENT_G;

  // bit mask of human readable value on 7-segment display
  const uint16_t OFF = 0x0000;
  const uint16_t NUM_0 = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_F;
  const uint16_t NUM_1 = SEGMENT_B | SEGMENT_C;
  const uint16_t NUM_2 = SEGMENT_A | SEGMENT_B | SEGMENT_G | SEGMENT_E | SEGMENT_D;
  const uint16_t NUM_3 = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_G;
  const uint16_t NUM_4 = SEGMENT_F | SEGMENT_G | SEGMENT_B | SEGMENT_C;
  const uint16_t NUM_5 = SEGMENT_A | SEGMENT_F | SEGMENT_G | SEGMENT_C | SEGMENT_D;
  const uint16_t NUM_6 = SEGMENT_A | SEGMENT_F | SEGMENT_E | SEGMENT_D | SEGMENT_C | SEGMENT_G;
  const uint16_t NUM_7 = SEGMENT_A | SEGMENT_B | SEGMENT_C;
  const uint16_t NUM_8 = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_F | SEGMENT_G;
  const uint16_t NUM_9 = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_F | SEGMENT_G;
  const uint16_t LET_A = SEGMENT_E | SEGMENT_F | SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_G;
  const uint16_t LET_C = SEGMENT_A | SEGMENT_F | SEGMENT_E | SEGMENT_D;
  const uint16_t LET_D = SEGMENT_B | SEGMENT_C | SEGMENT_D | SEGMENT_E | SEGMENT_G;
  const uint16_t LET_E = SEGMENT_A | SEGMENT_F | SEGMENT_E | SEGMENT_D | SEGMENT_G;
  const uint16_t LET_F = SEGMENT_E | SEGMENT_F | SEGMENT_A | SEGMENT_G;
  const uint16_t LET_H = SEGMENT_B | SEGMENT_C | SEGMENT_E | SEGMENT_F | SEGMENT_G;
  const uint16_t LET_N = SEGMENT_A | SEGMENT_B | SEGMENT_C | SEGMENT_E | SEGMENT_F;
}

// bit mask of button
namespace FRAME_BUTTON
{
  const uint16_t POWER = 0x0400;
  const uint16_t FILTER = 0x0002;
  const uint16_t HEATER = 0x8000;
  const uint16_t BUBBLE = 0x0008;
  const uint16_t TEMP_UP = 0x1000;
  const uint16_t TEMP_DOWN = 0x0080;
  const uint16_t TEMP_UNIT = 0x2000;
}

// frame type markers
namespace FRAME_TYPE
{
  const uint16_t CUE = 0x0100;
  const uint16_t LED = 0x4000;
  const uint16_t DIGIT = FRAME_DIGIT::POS_1 | FRAME_DIGIT::POS_2 | FRAME_DIGIT::POS_3 | FRAME_DIGIT::POS_4;
  const uint16_t BUTTON = CUE | FRAME_BUTTON::POWER | FRAME_BUTTON::FILTER | FRAME_BUTTON::HEATER | FRAME_BUTTON::BUBBLE | FRAME_BUTTON::TEMP_UP | FRAME_BUTTON::TEMP_DOWN | FRAME_BUTTON::TEMP_UNIT;
}

namespace DIGIT
{
  // 7-segment display update control
  const uint8_t POS_1 = 0x8;
  const uint8_t POS_2 = 0x4;
  const uint8_t POS_3 = 0x2;
  const uint8_t POS_4 = 0x1;
  const uint8_t POS_1_2 = POS_1 | POS_2;
  const uint8_t POS_1_2_3 = POS_1 | POS_2 | POS_3;
  const uint8_t POS_ALL = POS_1 | POS_2 | POS_3 | POS_4;

  // nibble value used to map a subset of non-numeric states of the 7-segment display
  const uint8_t LET_C = 0xC;
  const uint8_t LET_D = 0xD;
  const uint8_t LET_E = 0xE;
  const uint8_t LET_F = 0xF;

  const uint8_t LET_N = 0xA;
  const uint8_t OFF = 0xB;
};

namespace ERROR
{
  // internal binary value of error display (3 letters)
  const uint16_t NONE = 0;
  const uint16_t NO_WATER_FLOW = (DIGIT::LET_E << 8) | (9 << 4) | 0;
  const uint16_t WATER_TEMP_LOW = (DIGIT::LET_E << 8) | (9 << 4) | 4;
  const uint16_t WATER_TEMP_HIGH = (DIGIT::LET_E << 8) | (9 << 4) | 5;
  const uint16_t SYSTEM = (DIGIT::LET_E << 8) | (9 << 4) | 6;
  const uint16_t DRY_FIRE_PROTECT = (DIGIT::LET_E << 8) | (9 << 4) | 7;
  const uint16_t TEMP_SENSOR = (DIGIT::LET_E << 8) | (9 << 4) | 9;
  const uint16_t HEATING_ABORTED = (DIGIT::LET_E << 8) | (DIGIT::LET_N << 4) | DIGIT::LET_D;

  const uint16_t VALUES[] = {NO_WATER_FLOW, WATER_TEMP_LOW, WATER_TEMP_HIGH, SYSTEM, DRY_FIRE_PROTECT, TEMP_SENSOR, HEATING_ABORTED};
  const unsigned int COUNT = sizeof(VALUES) / sizeof(uint16_t);

  // human readable error on display
  const char CODE_90[] PROGMEM = "E90";
  const char CODE_94[] PROGMEM = "E94";
  const char CODE_95[] PROGMEM = "E95";
  const char CODE_96[] PROGMEM = "E96";
  const char CODE_97[] PROGMEM = "E97";
  const char CODE_99[] PROGMEM = "E99";
  const char CODE_END[] PROGMEM = "END";
  const char CODE_OTHER[] PROGMEM = "EXX";

  const char *const TEXT[1][COUNT + 1] PROGMEM = {
      {CODE_90, CODE_94, CODE_95, CODE_96, CODE_97, CODE_99, CODE_END, CODE_OTHER}};
}

// special display values
inline uint8_t display2Byte(uint16_t v) { return v & 0x000F; }
inline uint16_t display2Num(uint16_t v) { return (((v >> 12) & 0x000F) * 100) + (((v >> 8) & 0x000F) * 10) + ((v >> 4) & 0x000F); }
inline uint16_t display2Error(uint16_t v) { return (v >> 4) & 0x0FFF; }
inline bool displayIsTemp(uint16_t v) { return display2Byte(v) == DIGIT::LET_C || display2Byte(v) == DIGIT::LET_F; }
inline bool displayIsError(uint16_t v) { return (v & 0xF000) == 0xE000; }
inline bool displayIsBlank(uint16_t v) { return (v & 0xFFF0) == ((DIGIT::OFF << 12) + (DIGIT::OFF << 8) + (DIGIT::OFF << 4)); }

volatile SBH20IO::State SBH20IO::state;
volatile SBH20IO::IsrState SBH20IO::isrState;
volatile SBH20IO::Buttons SBH20IO::buttons;

// @TODO detect when latch signal stays low
// @TODO detect act temp change during error
// @TODO improve reliability of water temp change (counter auto repeat and too short press)
void SBH20IO::setup()
{
  pinMode(PIN::CLOCK, INPUT);
  pinMode(PIN::DATA, INPUT);
  pinMode(PIN::LATCH, INPUT);

  attachInterruptArg(digitalPinToInterrupt(PIN::CLOCK), SBH20IO::clockRisingISR, this, RISING);
}

void SBH20IO::loop()
{
  // device online check
  unsigned long now = millis();
  if (state.stateUpdated)
  {
    lastStateUpdateTime = now;
    state.online = true;
    state.stateUpdated = false;
  }
  else if (DIFF::timeDiff(now, lastStateUpdateTime) > CYCLE::RECEIVE_TIMEOUT)
  {
    state.online = false;
  }
}

bool SBH20IO::isOnline() const
{
  return state.online;
}

unsigned int SBH20IO::getTotalFrames() const
{
  return state.frameCounter;
}

unsigned int SBH20IO::getDroppedFrames() const
{
  return state.frameDropped;
}

int SBH20IO::getActWaterTempCelsius() const
{
  return (state.waterTemp != UNDEF::USHORT) ? convertDisplayToCelsius(state.waterTemp) : UNDEF::USHORT;
}

int SBH20IO::getDesiredWaterTempCelsius() const
{
  return (state.desiredTemp != UNDEF::USHORT) ? convertDisplayToCelsius(state.desiredTemp) : UNDEF::USHORT;
}

unsigned int SBH20IO::getErrorValue() const
{
  return state.error;
}

String SBH20IO::getErrorMessage(unsigned int errorValue) const
{
  if (errorValue)
  {
    // get error text index of error value
    unsigned int i;
    for (i = 0; i < ERROR::COUNT; i++)
    {
      if (ERROR::VALUES[i] == errorValue)
      {
        break;
      }
    }

    // load error text from PROGMEM
    return FPSTR(ERROR::TEXT[0][i]);
  }
  else
  {
    // no error
    return "";
  }
}

unsigned int SBH20IO::getRawLedValue() const
{
  return (state.ledStatus != UNDEF::USHORT) ? state.ledStatus : UNDEF::USHORT;
}

uint8_t SBH20IO::isPowerOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::POWER) != 0) : UNDEF::BOOL;
}

uint8_t SBH20IO::isFilterOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::FILTER) != 0) : UNDEF::BOOL;
}

uint8_t SBH20IO::isBubbleOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::BUBBLE) != 0) : UNDEF::BOOL;
}

uint8_t SBH20IO::isHeaterOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & (FRAME_LED::HEATER_ON | FRAME_LED::HEATER_STANDBY)) != 0) : UNDEF::BOOL;
}

uint8_t SBH20IO::isHeaterStandby() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::HEATER_STANDBY) != 0) : UNDEF::BOOL;
}

uint8_t SBH20IO::isBuzzerOn() const
{
  return (state.ledStatus != UNDEF::USHORT) ? ((state.ledStatus & FRAME_LED::NO_BEEP) == 0) : UNDEF::BOOL;
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
void SBH20IO::setDesiredWaterTempCelsius(int temp)
{
  if (temp >= WATER_TEMP::SET_MIN && temp <= WATER_TEMP::SET_MAX)
  {
    if (isPowerOn() == true && state.error == ERROR::NONE)
    {
      int setTemp = getDesiredWaterTempCelsius();
      bool modifying = false;
      if (setTemp == UNDEF::USHORT)
      {
        // trigger temp modification
        changeWaterTemp(-1);
        modifying = true;

        // wait for temp readback (will take 2-3 blink durations)
        int sleep = 20; // ms
        int tries = 4 * BLINK::PERIOD / sleep;
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
          delay(1);
          return;
        }
      }

      // modify desired temp
      int deltaTemp = temp - setTemp;
      // DEBUG_MSG("\nBdelta %d", deltaTemp);
      while (deltaTemp)
      {
        if (deltaTemp > 0)
        {
          // DEBUG_MSG("\nBU");
          changeWaterTemp(1);
          if (modifying)
          {
            deltaTemp--;
            setTemp++;
          }
        }
        else
        {
          // DEBUG_MSG("\nBD");
          changeWaterTemp(-1);
          if (modifying)
          {
            deltaTemp++;
            setTemp--;
          }
        }
        modifying = true;
      }
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
bool SBH20IO::pressButton(volatile unsigned int &buttonPressCount)
{
  waitBuzzerOff();
  unsigned int tries = BUTTON::ACK_TIMEOUT / BUTTON::ACK_CHECK_PERIOD;
  buttonPressCount = BUTTON::PRESS_COUNT;
  while (buttonPressCount && tries)
  {
    delay(BUTTON::ACK_CHECK_PERIOD);
    tries--;
  }

  return tries;
}

void SBH20IO::setBubbleOn(bool on)
{
  if (on ^ (isBubbleOn() == true))
  {
    pressButton(buttons.toggleBubble);
  }
}

void SBH20IO::setFilterOn(bool on)
{
  if (on ^ (isFilterOn() == true))
  {
    pressButton(buttons.toggleFilter);
  }
}

void SBH20IO::setHeaterOn(bool on)
{
  if (on ^ (isHeaterOn() == true || isHeaterStandby() == true))
  {
    pressButton(buttons.toggleHeater);
  }
}

void SBH20IO::setPowerOn(bool on)
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
bool SBH20IO::waitBuzzerOff() const
{
  int tries = BUTTON::ACK_TIMEOUT / BUTTON::ACK_CHECK_PERIOD;
  while (state.buzzer && tries)
  {
    delay(BUTTON::ACK_CHECK_PERIOD);
    tries--;
  }

  // extra delay reduces chance to trigger auto repeat
  if (tries)
  {
    delay(2 * CYCLE::PERIOD);
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
bool SBH20IO::changeWaterTemp(int up)
{
  if (isPowerOn() == true && state.error == ERROR::NONE)
  {
    // perform button action
    waitBuzzerOff();
    // DEBUG_MSG("\nP ");
    int tries = BUTTON::ACK_TIMEOUT / BUTTON::ACK_CHECK_PERIOD;
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
      return false;
    }
  }
  return false;
}

uint16_t SBH20IO::convertDisplayToCelsius(uint16_t value) const
{
  uint16_t celsiusValue = display2Num(value);
  uint16_t tempUint = display2Byte(value);
  if (tempUint == DIGIT::LET_F)
  {
    // convert °F to °C
    float fValue = (float)celsiusValue;
    celsiusValue = (uint16_t)round(((fValue - 32) * 5) / 9);
  }
  else if (tempUint != DIGIT::LET_C)
  {
    celsiusValue = UNDEF::USHORT;
  }

  return (celsiusValue >= 0) && (celsiusValue <= 60) ? celsiusValue : UNDEF::USHORT;
}

void SBH20IO::clockRisingISR(void *arg)
{
  bool data = !digitalRead(PIN::DATA);
  bool enable = digitalRead(PIN::LATCH) == LOW;

  /*
    SBH20IO* sbh20io = (SBH20IO*)arg;
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
        // DEBUG_MSG("\nC");
      }
      else if (isrState.frameValue & FRAME_TYPE::DIGIT)
      {
        // display frame
        // DEBUG_MSG("\nD");
        decodeDisplay();
      }
      else if (isrState.frameValue & FRAME_TYPE::LED)
      {
        // LED frame
        // DEBUG_MSG("\nL");
        decodeLED();
      }
      else if (isrState.frameValue & FRAME_TYPE::BUTTON)
      {
        // button frame
        // DEBUG_MSG("\nB");
        decodeButton();
      }
      else if (isrState.frameValue != 0)
      {
        // unsupported frame
        // DEBUG_MSG("\nU");
      }

      isrState.receivedBits = 0;
    }
  }
  else
  {
    // DEBUG_MSG(" %d ", receivedBits);
    isrState.receivedBits = 0;
    state.frameCounter++;
  }
}

inline void SBH20IO::decodeDisplay()
{
  uint8_t digit;
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
    // DEBUG_MSG("1");
    isrState.displayValue = (isrState.displayValue & 0x0FFF) + (digit << 12);
    isrState.receivedDigits = DIGIT::POS_1;
    break;

  case FRAME_DIGIT::POS_2:
    // DEBUG_MSG("2");
    if (isrState.receivedDigits == DIGIT::POS_1)
    {
      isrState.displayValue = (isrState.displayValue & 0xF0FF) + (digit << 8);
      isrState.receivedDigits |= DIGIT::POS_2;
    }
    break;

  case FRAME_DIGIT::POS_3:
    // DEBUG_MSG("3");
    if (isrState.receivedDigits == DIGIT::POS_1_2)
    {
      isrState.displayValue = (isrState.displayValue & 0xFF0F) + (digit << 4);
      isrState.receivedDigits |= DIGIT::POS_3;
    }
    break;

  case FRAME_DIGIT::POS_4:
    // DEBUG_MSG("4");
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
      // DEBUG_MSG(" s%x", isrState.displayValue);
      isrState.stableDisplayValueCount--;
      if (isrState.stableDisplayValueCount == 0)
      {
        // DEBUG_MSG("S%x ", isrState.displayValue);
        isrState.stableDisplayValueCount = CONFIRM_FRAMES::DISP;
        // DEBUG_MSG("O");
        if (isrState.isDisplayBlinking)
        {
          if (DIFF::intDiff(state.frameCounter, isrState.lastBlankDisplayFrameCounter) > BLINK::STOPPED_FRAMES)
          {
            // blinking is over, clear desired temp
            // DEBUG_MSG("b");
            isrState.isDisplayBlinking = false;
            isrState.latestBlinkingTemp = UNDEF::USHORT;
          }
        }

        if (!displayIsError(isrState.displayValue))
        {
          // display does not show an error
          // DEBUG_MSG("e");
          if (displayIsTemp(isrState.displayValue))
          {
            // display shows a temperature
            // DEBUG_MSG("T");
            if (isrState.isDisplayBlinking)
            {
              // display is blinking
              if (isrState.displayValue == isrState.latestBlinkingTemp)
              {
                // blinking temp is stable
                isrState.stableBlinkingWaterTempCount++;
                // DEBUG_MSG(" DS%d ", isrState.stableDesiredWaterTempCount);
              }
              else if (DIFF::intDiff(state.frameCounter, isrState.lastBlankDisplayFrameCounter) < BLINK::TEMP_FRAMES)
              {
                // blinking temp has changed (is read after a blank screen and set at next black screen)
                // DEBUG_MSG(" DC%x ", isrState.displayValue);
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
                // DEBUG_MSG("A%d", stableWaterTempCount);
                isrState.stableWaterTempCount--;
                if (isrState.stableWaterTempCount == 0)
                {
                  // save actual temp
                  if (state.waterTemp != isrState.displayValue)
                  {
                    // DEBUG_MSG(" AT ");
                    state.waterTemp = isrState.displayValue;
                  }

                  // get temp unit
                  uint16_t tempUnit = display2Byte(isrState.displayValue);
                  if (tempUnit != isrState.latestTempUnit)
                  {
                    isrState.latestTempUnit = tempUnit;
                  }

                  isrState.stableWaterTempCount = CONFIRM_FRAMES::WATER_TEMP_ACT;

                  // clear error
                  // state.error = ERROR::NONE;
                }
              }
              else
              {
                // actual temp is changed
                // DEBUG_MSG("a");
                isrState.latestWaterTemp = isrState.displayValue;
                isrState.stableWaterTempCount = CONFIRM_FRAMES::WATER_TEMP_ACT;
              }
            }
          }
          else
          {
            // unsupported display state (no error, no temperature)
            // DEBUG_MSG("t");
          }
        }
        else
        {
          // display shows error code
          // DEBUG_MSG("E");
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
        // DEBUG_MSG("B");
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
          if (!state.error && isrState.blankCounter > 2 && isrState.stableBlinkingWaterTempCount >= CONFIRM_FRAMES::WATER_TEMP_SET && state.desiredTemp != isrState.latestBlinkingTemp)
          {
            // DEBUG_MSG("\nDT%x ", isrState.displayValue);
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

inline void SBH20IO::decodeLED()
{
  if (isrState.frameValue == isrState.latestLedStatus)
  {
    // wait for confirmation
    isrState.stableLedStatusCount--;
    if (isrState.stableLedStatusCount == 0)
    {
      // DEBUG_MSG("\nL%x", frameValue);
      state.ledStatus = isrState.frameValue;
      state.buzzer = !(state.ledStatus & FRAME_LED::NO_BEEP);
      state.stateUpdated = true;
      isrState.stableLedStatusCount = CONFIRM_FRAMES::LED;

      // clear buttons if buzzer is on
      if (state.buzzer)
      {
        buttons.toggleBubble = 0;
        buttons.toggleFilter = 0;
        buttons.toggleHeater = 0;
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

inline void SBH20IO::decodeButton()
{
  if (isrState.frameValue & FRAME_BUTTON::FILTER)
  {
    // DEBUG_MSG("F");
    if (buttons.toggleFilter)
    {
      isrState.reply = true;
      buttons.toggleFilter--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::HEATER)
  {
    // DEBUG_MSG("H");
    if (buttons.toggleHeater)
    {
      isrState.reply = true;
      buttons.toggleHeater--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::BUBBLE)
  {
    // DEBUG_MSG("B");
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
    // DEBUG_MSG(" P");
    if (buttons.togglePower)
    {
      isrState.reply = true;
      buttons.togglePower--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::TEMP_UP)
  {
    // DEBUG_MSG("U");
    if (buttons.toggleTempUp)
    {
      isrState.reply = true;
      buttons.toggleTempUp--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::TEMP_DOWN)
  {
    // DEBUG_MSG("D");
    if (buttons.toggleTempDown)
    {
      isrState.reply = true;
      buttons.toggleTempDown--;
    }
  }
  else if (isrState.frameValue & FRAME_BUTTON::TEMP_UNIT)
  {
    // DEBUG_MSG("T");
  }
  else
  {
    // DEBUG_MSG(" B%x", frameValue);
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
    //#error "160 MHz CPU frequency required! Pulse timing not possible at 80 MHz, because the code above takes too long to reach this point."
    // at least using Arduino methods
#endif
    isrState.reply = false;
  }
}
