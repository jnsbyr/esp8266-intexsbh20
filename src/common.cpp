#include "common.h"

// time delta with overflow support
unsigned long DIFF::timeDiff(unsigned long newTime, unsigned long oldTime)
{
  if (newTime >= oldTime)
  {
    return newTime - oldTime;
  }
  else
  {
    return ULONG_MAX - oldTime + newTime + 1;
  }
};

// unsigned int delta with overflow support
unsigned int DIFF::intDiff(unsigned int newVal, unsigned int oldVal)
{
  if (newVal >= oldVal)
  {
    return newVal - oldVal;
  }
  else
  {
    return UINT_MAX - oldVal + newVal + 1;
  }
};