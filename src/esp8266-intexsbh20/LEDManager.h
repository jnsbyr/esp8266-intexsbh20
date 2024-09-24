/**
 * DIYSCIP (c) by Geoffroy HUBERT - yorffoeg@gmail.com
 * This file is part of DIYSCIP <https://github.com/yorffoeg/diyscip>.
 *
 * DIYSCIP is licensed under a
 * Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 * License.
 *
 * You should have received a copy of the license along with this
 * work. If not, see <http://creativecommons.org/licenses/by-nc-sa/4.0/>.
 *
 * DIYSCIP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;
 */

#ifndef LEDMANAGER_H
#define LEDMANAGER_H

#include "Arduino.h"

#define MODE_PATTERN_INFO 1
#define MODE_PATTERN_MAX 10 * MODE_PATTERN_INFO
#define DELAY 200

#define BLINK_STA                                                               \
  { 10, 10, 0 }
#define BLINK_OK                                                                \
  {5,5, 5,5, 5,5, 0 }
#define BLINK_KO                                                                \
  {5,5, 1,1, 5,5, 0 }
#define BLINK_AP                                                                \
  { 1, 1, 0 }

class LEDManager {

public:
  LEDManager(unsigned char pin);

  void loop();
  void setMode(const std::initializer_list<unsigned char> &modePattern);

private:
  unsigned char pin;
  unsigned char modePattern[MODE_PATTERN_MAX + 1] = {};
  unsigned char modeStep = 0;
  unsigned char modeCounter = 0;
  unsigned long now = 0;
};

#endif // LEDMANAGER_H
