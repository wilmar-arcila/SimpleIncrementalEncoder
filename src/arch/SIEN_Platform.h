#ifndef SIEN_PLATFORM_H
#define SIEN_PLATFORM_H

#include <Arduino.h>

#if defined(__AVR_ATmega328P__)
    #include "SIEN_ATmega328P.h"
#else
    #include "SIEN_Generic.h"
#endif

#endif
