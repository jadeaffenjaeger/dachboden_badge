#include "lfsr.h"

/*
 * This file implements a basic 32bit Galois Linear Feedback Shift Register as a source of pseudo-randomness.
 * Shamelessly stolen from Wikipedia.
 */


/*
 *Run single LFSR update cycle
 */
void lfsr_update(lfsr_t* reg) {
    uint8_t lsb = reg->state & 1;
    reg->state >>= 1;
    if(lsb == 1) {
        reg->state ^= reg->mask;
    }
}

/*
 *Update 8 times and get lowest byte
 */
uint8_t lfsr_get8(lfsr_t* reg) {
    for(uint8_t i = 0; i < 8; i++) {
        lfsr_update(reg);
    }
    return (uint8_t) reg->state;
}

/*
 *Update 32 times and get 32 byte of pseudo-random data
 */
uint32_t lfsr_get32(lfsr_t* reg) {
    for(uint8_t i = 0; i < 32; i++) {
        lfsr_update(reg);
    }
    return reg->state;
}
