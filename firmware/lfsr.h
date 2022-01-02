#ifndef __LFSR_H
#define __LFSR_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 *Defines polynomial and current state of the LFSR
 */
typedef struct lfsr {
    uint32_t mask;
    uint32_t state;
} lfsr_t;

/*
 *Get pseudo-random 8 Bit sequence
 */
uint8_t lfsr_get8(lfsr_t* reg);

/*
 *Get pseudo-random 32 Bit sequence
 */
uint32_t lfsr_get32(lfsr_t* reg);

#ifdef __cplusplus
}
#endif
#endif
