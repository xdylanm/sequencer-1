#ifndef _board_target_h
#define _board_target_h

#if defined(ADAFRUIT_QTPY_M0) || defined(ADAFRUIT_ITSYBITSY_M0) && !defined(__SAMD21__)
#define __SAMD21__
#elif defined(ADAFRUIT_ITSYBITSY_M4_EXPRESS) && !defined(__SAMD51__)
#define __SAMD51__
#endif

#if defined(__SAMD21__)
#define DAC_NUM_BITS 10
#elif defined(__SAMD51__)
#define DAC_NUM_BITS 12
#else
#error Unsupported target microprocess architecture
#endif

#endif