/*
 * project:  Intex PureSpa SB-H20 WiFi Controller
 *
 * file:     SBH20IO.h
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

#ifndef SBH20IO_H
#define SBH20IO_H

#include <c_types.h>
#include <WString.h>
#include "common.h"


/**
 * The Intex serial protocol between the mainboard of the SB-H20 model and its
 * control panel uses a 16-bit data frame where each bit is sampled by the
 * rising clock signal and the frame is sampled by the rising slave select
 * signal, similar to SPI mode 3 but unidirectional. The clock speed is 100 kHz,
 * the data bits are inverted and the bit order is big endian (MSB).
 *
 * There are 4 telegram types, one for the 7-segment display, one for the LEDs,
 * one for the buttons and the cue telegram that is send each time the telegram
 * type changes. Only the button telegram is used for sending commands to the
 * SB-H20. All other telegrams are receive telegrams.
 *
 * To signal when a button is pressed the data line must be pulled low for
 * 2 microseconds after the slave select signal turns high if a switch telegram
 * has been received that matches the button function.
 *
 *        |-------------------------------------------------------------------------------|
 * BIT    | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
 *        |-------------------------------------------------------------------------------|
 * CUE    |  0 |  0 |  0 |  0 |  0 |  0 |  0 |  1 |  0 |  0 |  0 |  0 |  0 |  0 |  0 |  0 |
 * DIGIT  | DP |  x |  A |  B | S3 |  D |  C |  x |  E | S1 | S2 |  G |  F | S4 |  x |  x |
 * LED    |  0 |  1 |  0 |  x |  0 |  x |  x |  x |  x |  0 |  0 |  0 |  0 |  0 |  0 |  x |
 * BUTTON |  x |  0 |  x |  x |  0 |  x |  0 |  1 |  0 |  0 |  0 |  0 |  x |  0 |  0 |  0 |
 *        |-------------------------------------------------------------------------------|
 *
 * display:
 *
 * bits S1, S2, S3 and S4 each select a 7-segment display, digits are from left to right
 *
 * DP .    A
 *       -----
 *      |     |
 *    F |     | B
 *      |     |
 *       --G--
 *      |     |
 *    E |     | C
 *      |     |
 *       -----
 *         D
 *
 * telegram order:
 *
 * 32 frames, repeating every 21 ms (5x digit 1-4, 5x LEDs, 1x buttons 1-7)
 *
 * CD1
 * CD2
 * CD3
 * CD4
 * CL
 * CD1
 * CD2
 * CD3
 * CD4
 * CL
 * CD1
 * CD2
 * CD3
 * CD4
 * CL
 * CD1
 * CD2
 * CD3
 * CD4
 * CL
 * CD1
 * CD2
 * CD3
 * CD4
 * CBBBBBBBL
 *
 *
 * Build Notes:
 *
 * The define of Arduino Core for ESP8266 named "DEBUG_ESP_PORT" must not be
 * enabled because core debugging will reduce the performance so much that stable
 * decoding of the 100 kHz clock is not possible. Instead use selective
 * Serial.printf() where needed.
 *
 * Any extended operation and especially serial debugging in an ISR might cause
 * a crash.
 *
 * Using member variables or calling member function from ISR slows down
 * execution causing the button operations to become unreliable or fail.
 *
 * The code is near the upper limit for using IRAM for VTables. Some
 * modifications may cause the linker to fail. You could change the VTables
 * settings to "Flash" but that will make the code run a little bit slower and
 * the button operations will become unreliable or fail. Avoiding plain members
 * and using structs or classes instead helps saving VTable space.
 *
 */

class SBH20IO
{
public:
  class UNDEF
  {
  public:
    static const uint8 BOOL     = 0xFF;
    static const uint16 USHORT = 0xFFFF;
  };

  class WATER_TEMP
  {
  public:
    static const int SET_MIN = 20; // °C
    static const int SET_MAX = 40; // °C
  };

public:
  void setup(LANG language);
  void loop();

public:
  bool isOnline() const;

  int getActWaterTempCelsius() const;
  int getDesiredWaterTempCelsius() const;

  uint8 isBubbleOn() const;
  uint8 isFilterOn() const;
  uint8 isHeaterOn() const;
  uint8 isHeaterStandby() const;
  uint8 isPowerOn() const;
  uint8 isBuzzerOn() const;

  void setDesiredWaterTempCelsius(int temp);

  void setBubbleOn(bool on);
  void setFilterOn(bool on);
  void setHeaterOn(bool on);
  void setPowerOn(bool on);

  unsigned int getErrorValue() const;
  String getErrorMessage(unsigned int errorValue) const;

  unsigned int getRawLedValue() const;

  unsigned int getTotalFrames() const;
  unsigned int getDroppedFrames() const;

private:

  class CYCLE
  {
  public:
    static const unsigned int TOTAL_FRAMES    = 32; // number of frames in each cycle
    static const unsigned int DISPLAY_FRAMES  = 5;  // number of digit frame groups in each cycle
    static const unsigned int PERIOD          = 21; // ms, period of frame cycle
    static const unsigned int RECEIVE_TIMEOUT = 50*CYCLE::PERIOD; // ms
  };

  class FRAME
  {
  public  :
    static const unsigned int BITS = 16;
    static const unsigned int FREQUENCY = CYCLE::TOTAL_FRAMES/CYCLE::PERIOD; // frames/ms
  };

  class BLINK
  {
  public:
    static const unsigned int PERIOD         = 500; // ms, temp will blink 8 times in 4000 ms
    static const unsigned int TEMP_FRAMES    = PERIOD/4*FRAME::FREQUENCY; // sample duration of desired temp after blank display
    static const unsigned int STOPPED_FRAMES = 2*PERIOD*FRAME::FREQUENCY; // must be longer than single blink duration
  };

  class CONFIRM_FRAMES
  {
  public:
    static const unsigned int LED            = 3;
    static const unsigned int DISP           = 3;
    static const unsigned int WATER_TEMP_SET = 3;
    static const unsigned int WATER_TEMP_ACT = BLINK::PERIOD/2*FRAME::FREQUENCY/CYCLE::DISPLAY_FRAMES; // ms, must be high enough to tell from blinking
  };

  class BUTTON
  {
  public:
    static const unsigned int PRESS_COUNT       = BLINK::PERIOD/CYCLE::PERIOD - 2; // must be long enough to trigger, but short enough to avoid double trigger
    static const unsigned int ACK_CHECK_PERIOD  = 10; // ms
    static const unsigned int ACK_TIMEOUT       = 2*PRESS_COUNT*CYCLE::PERIOD; // ms
  };

private:
  struct State
  {
    uint16 waterTemp   = UNDEF::USHORT;
    uint16 desiredTemp = UNDEF::USHORT;
    uint16 ledStatus   = UNDEF::USHORT;

    bool buzzer = false;
    uint16 error = 0;
    unsigned int lastErrorChangeFrameCounter = 0;

    bool online = false;
    bool stateUpdated = false;

    unsigned int frameCounter = 0;
    unsigned int frameDropped = 0;
  };

  struct IsrState
  {
    uint16 latestWaterTemp    = UNDEF::USHORT;
    uint16 latestBlinkingTemp = UNDEF::USHORT;
    uint16 latestLedStatus    = UNDEF::USHORT;
    uint8 latestTempUnit      = 0;

    uint16 frameValue = 0;
    uint16 receivedBits = 0;

    unsigned int lastBlankDisplayFrameCounter = 0;
    unsigned int blankCounter = 0;

    unsigned int stableDisplayValueCount     = CONFIRM_FRAMES::DISP;
    unsigned int stableDisplayBlankCount     = CONFIRM_FRAMES::DISP;
    unsigned int stableWaterTempCount        = CONFIRM_FRAMES::WATER_TEMP_ACT;
    unsigned int stableBlinkingWaterTempCount = 0;
    unsigned int stableLedStatusCount        = CONFIRM_FRAMES::LED;

    uint16 displayValue       = UNDEF::USHORT;
    uint16 latestDisplayValue = UNDEF::USHORT;

    uint16 receivedDigits = 0;

    bool isDisplayBlinking = false;

    bool reply = false;
  };

  struct Buttons
  {
    unsigned int toggleBubble   = 0;
    unsigned int toggleFilter   = 0;
    unsigned int toggleHeater   = 0;
    unsigned int togglePower    = 0;
    unsigned int toggleTempUp   = 0;
    unsigned int toggleTempDown = 0;
  };

private:
  // ISR and ISR helper
  static IRAM_ATTR void clockRisingISR(void* arg);
  static IRAM_ATTR inline void decodeDisplay();
  static IRAM_ATTR inline void decodeLED();
  static IRAM_ATTR inline void decodeButton();
/*
  IRAM_ATTR inline void decodeDisplay();
  IRAM_ATTR inline void decodeLED();
  IRAM_ATTR inline void decodeButton();
*/

private:
  // ISR variables
  static volatile State state;
  static volatile IsrState isrState;
  static volatile Buttons buttons;
/*
  volatile State state;
  volatile IsrState isrState;
  volatile Buttons buttons;
 */

private:
  uint16 convertDisplayToCelsius(uint16 value) const;
  bool waitBuzzerOff() const;
  bool pressButton(volatile unsigned int& buttonPressCount);
  bool changeWaterTemp(int up);

private:
  LANG language;
  unsigned long lastStateUpdateTime = 0;
};

#endif /* SBH20IO_H */

