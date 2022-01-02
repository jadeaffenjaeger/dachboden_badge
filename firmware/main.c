#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "ws2812_config.h"
#include "light_ws2812.h"

#include "lfsr.h"

#define MAXPIX 16
#define LEN(x) sizeof(x)/sizeof(x[0])

lfsr_t random = {
    .state = 0xDEADBEEF, 
    .mask = 0xE10000,
};

struct cRGB colors[] = {
    {.r=0xff, .g=0x00, .b=0x00}, // .red
    {.r=0xfa, .g=0x80, .b=0x72}, // Salmon
    {.r=0xff, .g=0x7f, .b=0x50}, // Co.ral
    {.r=0xff, .g=0xd7, .b=0x00}, // .gold
    {.r=0xff, .g=0xff, .b=0x00}, // Yellow
    {.r=0x7c, .g=0xfc, .b=0x00}, // lawn.g.reen
    {.r=0x00, .g=0xff, .b=0x00}, // lime
    {.r=0x00, .g=0xff, .b=0x7f}, // sp.rin.g.g.reen
    {.r=0x2e, .g=0x8b, .b=0x57}, // sea.g.reen
    {.r=0x00, .g=0xff, .b=0xff}, // cyan
    {.r=0x40, .g=0xe0, .b=0xd0}, // tu.rquoise
    {.r=0x00, .g=0x80, .b=0x80}, // teal
    {.r=0x00, .g=0xbf, .b=0xff}, // deepskyblue
    {.r=0x00, .g=0x00, .b=0xff}, // blue
    {.r=0xff, .g=0x00, .b=0xff}, // ma.genta;
    {.r=0x90, .g=0x00, .b=0x80}, // pu.rple;
    {.r=0x90, .g=0x00, .b=0x80}, // ma.genta;
    {.r=0xff, .g=0x69, .b=0xb4} // hotpink;
}; 

uint8_t sin_lut[] = {
    0.,   0.,   1.,   1.,   2.,   4.,   6.,   8.,  10.,  13.,  16.,
    19.,  22.,  26.,  30.,  34.,  38.,  43.,  48.,  53.,  58.,  64.,
    69.,  75.,  81.,  87.,  93.,  99., 105., 112., 117., 124., 131.,
    137., 143., 149., 155., 162., 168., 175., 180., 186., 192., 197.,
    202., 207., 211., 217., 220., 226., 230., 233., 235., 239., 243.,
    245., 247., 249., 251., 253., 253., 255., 255., 255.
};

struct cRGB led[MAXPIX];

void set_all(struct cRGB col, uint8_t dimming) {
    for (uint8_t i = 0; i< MAXPIX; i++) {
        led[i].r = col.r >> dimming;
        led[i].g = col.g >> dimming;
        led[i].b = col.b >> dimming;
    }
}

void dim_all(uint8_t amount) {
    for (uint8_t i = 0; i< MAXPIX; i++) {
        led[i].r = led[i].r < amount ? 0 : led[i].r - amount;
        led[i].g = led[i].g < amount ? 0 : led[i].g - amount;
        led[i].b = led[i].b < amount ? 0 : led[i].b - amount;
    }
}


void rollercoaster() {
    static const uint8_t indices[] = {2, 3, 4, 5, 6, 7, 0, 1, 14, 13, 12, 11, 10, 9, 8, 15};

    struct cRGB c_curr = colors[lfsr_get8(&random) % LEN(colors)];
    for (uint8_t reps = 0; reps < 50; reps++) {
        struct cRGB c_next = colors[lfsr_get8(&random) % LEN(colors)];

        int16_t step_r = (c_next.r - c_curr.r)/LEN(indices);
        int16_t step_g = (c_next.g - c_curr.g)/LEN(indices);
        int16_t step_b = (c_next.b - c_curr.b)/LEN(indices);

        for (uint8_t i = 0; i < LEN(indices); i++) {
            dim_all(1);


            led[indices[i]].r = c_curr.r >> 4;
            led[indices[i]].g = c_curr.g >> 4;
            led[indices[i]].b = c_curr.b >> 4;

            c_curr.r += step_r;
            c_curr.g += step_g;
            c_curr.b += step_b;

            ws2812_sendarray((uint8_t*) led, MAXPIX*3);
            _delay_ms(50);
        }
        c_curr = c_next;
    }
}

void scanner() {
    static const uint8_t upper[] = {6,7,0,1,2,14,15,8,9,10};
    static const uint8_t lower[] = {6,5,4,3,2,14,13,12,11,10};

    for (uint8_t reps = 0; reps < 30; reps++) {
        uint8_t col = lfsr_get8(&random) % LEN(colors);
        for (uint8_t i = 0; i < LEN(upper); i++) {
            dim_all(1);
            led[lower[i]].r = colors[col].r >> 4;
            led[lower[i]].g = colors[col].g >> 4;
            led[lower[i]].b = colors[col].b >> 4;
            led[upper[i]].r = colors[col].r >> 4;
            led[upper[i]].g = colors[col].g >> 4;
            led[upper[i]].b = colors[col].b >> 4;

            ws2812_sendarray((uint8_t *)led, MAXPIX*3);
            _delay_ms(20);
        }
        for (uint8_t pause = 0; pause < 50; pause++) {
            dim_all(1);
            ws2812_sendarray((uint8_t*) led, MAXPIX*3);
            _delay_ms(20);
        }
    }
}

void sparkles() {
    for (uint8_t col = 0; col < LEN(colors); col++) {
        struct cRGB c = colors[col];
        for (uint8_t reps = 0; reps < 20; reps++) {

            dim_all(8);

            uint8_t ind1 = lfsr_get8(&random) % MAXPIX;
            uint8_t ind2 = lfsr_get8(&random) % MAXPIX;

            int8_t r_off = lfsr_get8(&random) >> 2;
            int8_t g_off = lfsr_get8(&random) >> 2;
            int8_t b_off = lfsr_get8(&random) >> 2;

            struct cRGB c_rand = {
                .r = (c.r + r_off) >> 3,
                .g = (c.g + g_off) >> 3,
                .b = (c.b + b_off) >> 3,
            };

            led[ind1] = c_rand;
            led[ind2] = c_rand;
            ws2812_sendarray((uint8_t*) led, MAXPIX*3);
            _delay_ms(80);
        }
    }
}

void flashing() {
    for (int reps = 0; reps < 15; reps++) {
        uint8_t idx = lfsr_get8(&random) % LEN(colors);
        struct cRGB c = colors[idx];
        for (int i = 0; i < LEN(sin_lut); i++) {
            struct cRGB dimmed = {
                .r = (c.r * sin_lut[i]) / 255,
                .g = (c.g * sin_lut[i]) / 255,
                .b = (c.b * sin_lut[i]) / 255
            };
            set_all(dimmed, 3);
            ws2812_sendarray((uint8_t*) led, (MAXPIX/2)*3);
            ws2812_sendarray((uint8_t*) led, (MAXPIX/2)*3);
            _delay_ms(20);
        }

        for (int i = LEN(sin_lut) -1; i >= 0; i--) {
            struct cRGB dimmed = {
                .r = (c.r * sin_lut[i]) / 255,
                .g = (c.g * sin_lut[i]) / 255,
                .b = (c.b * sin_lut[i]) / 255
            };
            set_all(dimmed, 3);
            ws2812_sendarray((uint8_t*) led, (MAXPIX/2)*3);
            ws2812_sendarray((uint8_t*) led, (MAXPIX/2)*3);
            _delay_ms(20);
        }
    }
}

void circles() {
    for (int reps = 0; reps < 25; reps++) {
        uint8_t idx = lfsr_get8(&random) % LEN(colors);
        for (int i = 0; i < MAXPIX/2; i++) {
            dim_all(2);
            led[i].r = colors[idx].r >> 4;
            led[i].g = colors[idx].g >> 4;
            led[i].b = colors[idx].b >> 4;
            ws2812_sendarray((uint8_t*) led, (MAXPIX/2)*3);
            ws2812_sendarray((uint8_t*) led, (MAXPIX/2)*3);
            _delay_ms(100);
        }
    }
}

int main(void)
{
    DDRB|=_BV(ws2812_pin);
    while(1)
    {
        flashing();
        circles();
        sparkles();
        scanner();
        rollercoaster();
    }
}

