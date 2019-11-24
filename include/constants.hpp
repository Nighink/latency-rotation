#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <mbed.h>
#include <L6474.h>

const uint32_t DIODE_READ_US = 250;

#define STEPS_1 (400)   /* 1 revolution given a 400 steps motor configured at 1/8 microstep mode. */

extern L6474_init_t motor_init;

#endif
