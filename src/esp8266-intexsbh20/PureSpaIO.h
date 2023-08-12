/*
 * project:  Intex PureSpa WiFi Controller
 *
 * file:     PureSpaIO.h
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

#ifndef PURE_SPA_IO_H
#define PURE_SPA_IO_H

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
 * 25 cue frames and 32/34 data frames (5x digit 1-4, 5x LEDs, 1x buttons 1-7/9), repeating every 21 ms
 *
 * C D1
 * C D2
 * C D3
 * C D4
 * C L
 * C D1
 * C D2
 * C D3
 * C D4
 * C L
 * C D1
 * C D2
 * C D3
 * C D4
 * C L
 * C D1
 * C D2
 * C D3
 * C D4
 * C L
 * C D1
 * C D2
 * C D3
 * C D4
 * C BBBBBBB L   [SB-H20]
 * C BBBBBBBBB L [SJB-HS]
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

class PureSpaIO
{
public:
  enum MODEL
  {
    SBH20 = 1,
    SJBHS = 2
  };

  class UNDEF
  {
  public:
    static const uint8  BOOL   = UCHAR_MAX;
    static const uint16 USHORT = USHRT_MAX;
    static const uint32 UINT   = UINT_MAX;
    static const sint32 INT    = -99;
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
  MODEL getModel() const;
  const char* getModelName() const;

  bool isOnline() const;

  int getActWaterTempCelsius() const;
  int getDesiredWaterTempCelsius() const;
  int getDisinfectionTime() const;

  uint8 isBubbleOn() const;
  uint8 isBuzzerOn() const;
  uint8 isDisinfectionOn() const;
  uint8 isFilterOn() const;
  uint8 isHeaterOn() const;
  uint8 isHeaterStandby() const;
  uint8 isJetOn() const;
  uint8 isPowerOn() const;

  void setDesiredWaterTempCelsius(int temp);
  void setDisinfectionTime(int hours);

  void setBubbleOn(bool on);
  void setFilterOn(bool on);
  void setHeaterOn(bool on);
  void setJetOn(bool on);
  void setPowerOn(bool on);

  String getErrorCode() const;
  String getErrorMessage(const String& errorCode) const;

  unsigned int getRawLedValue() const;

  unsigned int getTotalFrames() const;
  unsigned int getDroppedFrames() const;

private:
  class CYCLE
  {
  public:
#if defined MODEL_SB_H20
    static const unsigned int BUTTON_FRAMES = 7; // number of button frames in each cycle
#elif defined MODEL_SJB_HS
    static const unsigned int BUTTON_FRAMES = 9; // number of button frames in each cycle
#endif
    static const unsigned int TOTAL_FRAMES = 25 + BUTTON_FRAMES; // number of frames in each cycle
    static const unsigned int DISPLAY_FRAME_GROUPS =  5; // number of digit frame groups in each cycle
    static const unsigned int PERIOD = 21; // ms, period of frame cycle @todo might be longer for SJB-HS
    static const unsigned int RECEIVE_TIMEOUT = 50*CYCLE::PERIOD; // ms
  };

  class FRAME
  {
  public  :
    static const unsigned int BITS = 16; // bits per frame
    static const unsigned int FREQUENCY = CYCLE::TOTAL_FRAMES/CYCLE::PERIOD; // frames/ms
  };

  class BLINK
  {
  public:
    static const unsigned int PERIOD = 500; // ms, temp will blink 8 times in 4000 ms
    static const unsigned int TEMP_FRAMES = PERIOD/4*FRAME::FREQUENCY; // sample duration of desired temp after blank display
    static const unsigned int STOPPED_FRAMES = 2*PERIOD*FRAME::FREQUENCY; // must be longer than single blink duration
  };

  class CONFIRM_FRAMES
  {
  public:
    static const unsigned int REGULAR = 3; // frames, for values which do not blink
    static const unsigned int NOT_BLINKING = BLINK::PERIOD/2*FRAME::FREQUENCY/CYCLE::DISPLAY_FRAME_GROUPS; // frames, must be high enough to tell from blinking
  };

  class BUTTON
  {
  public:
    static const unsigned int PRESS_COUNT = BLINK::PERIOD/CYCLE::PERIOD; // cycles, must be long enough to activate buzzer
    static const unsigned int PRESS_SHORT_COUNT = 380/CYCLE::PERIOD; // cycles, must be long enough to trigger, but short enough to avoid double trigger
    static const unsigned int ACK_CHECK_PERIOD = 10; // ms
    static const unsigned int ACK_TIMEOUT = 2*PRESS_COUNT*CYCLE::PERIOD; // ms
  };

private:
  struct State
  {
    uint32 waterTemp        = UNDEF::UINT; // ASCII, 4 chars, includes unit
    uint32 desiredTemp      = UNDEF::UINT; // ASCII, 4 chars, includes unit
    uint32 disinfectionTime = UNDEF::UINT; // ASCII, 4 chars, includes unit
    uint32 error            = ERROR_NONE;  // ASCII, 3 chars, null terminated, requires power cycle to reset

    uint16 ledStatus        = UNDEF::USHORT;

    bool buzzer = false;
    bool online = false;
    bool stateUpdated = false;

    unsigned int lastErrorChangeFrameCounter = 0;
    unsigned int frameCounter = 0;
    unsigned int frameDropped = 0;
  };

  struct IsrState
  {
    uint32 latestWaterTemp        = UNDEF::UINT;
    uint32 latestBlinkingTemp     = UNDEF::UINT;
    uint32 latestDisinfectionTime = UNDEF::UINT;
    uint16 latestLedStatus = UNDEF::USHORT;

    uint16 frameValue = 0;
    uint16 receivedBits = 0;

    unsigned int lastBlankDisplayFrameCounter = 0;
    unsigned int blankCounter = 0;

    unsigned int stableDisplayValueCount      = CONFIRM_FRAMES::REGULAR;
    unsigned int stableDisplayBlankCount      = CONFIRM_FRAMES::REGULAR;
    unsigned int stableWaterTempCount         = CONFIRM_FRAMES::NOT_BLINKING;
    unsigned int stableBlinkingWaterTempCount = 0;
    unsigned int stableDisinfectionTimeCount  = CONFIRM_FRAMES::REGULAR;
    unsigned int stableLedStatusCount         = CONFIRM_FRAMES::REGULAR;

    uint32 displayValue       = UNDEF::UINT;
    uint32 latestDisplayValue = UNDEF::UINT;

    uint8 receivedDigits = 0;

    bool isDisplayBlinking = false;

    bool reply = false;
  };

  struct Buttons
  {
    unsigned int toggleBubble       = 0;
    unsigned int toggleDisinfection = 0;
    unsigned int toggleFilter       = 0;
    unsigned int toggleHeater       = 0;
    unsigned int toggleJet          = 0;
    unsigned int togglePower        = 0;
    unsigned int toggleTempUp       = 0;
    unsigned int toggleTempDown     = 0;
  };

private:
  static const uint32 ERROR_NONE = 0;

private:
  // ISR and ISR helper
  static IRAM_ATTR void clockRisingISR(void* arg);
  static IRAM_ATTR inline void decodeDisplay();
  static IRAM_ATTR inline void decodeLED();
  static IRAM_ATTR inline void decodeButton();
  static IRAM_ATTR inline void updateButtonState(volatile unsigned int& buttonPressCount);

private:
  // ISR variables
  static volatile State state;
  static volatile IsrState isrState;
  static volatile Buttons buttons;

private:
  int convertDisplayToCelsius(uint32 value) const;
  bool waitBuzzerOff() const;
  bool pressButton(volatile unsigned int& buttonPressCount);
  bool changeWaterTemp(int up);

private:
#if defined MODEL_SB_H20
  MODEL model = MODEL::SBH20;
#elif defined MODEL_SJB_HS
  MODEL model = MODEL::SJBHS;
#else
  MODEL model;
  #error no model (MODEL_SB_H20 or MODEL_SJB_HS) selected in common.h
#endif

private:
  LANG language;
  unsigned long lastStateUpdateTime = 0;
  char errorBuffer[4];
};

#endif /* PURE_SPA_IO_H */
