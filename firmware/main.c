#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "ws2812_config.h"
#include "light_ws2812.h"

#include "lfsr.h"

#define MAXPIX 16
#define LEN(x) sizeof(x)/sizeof(x[0])

// LFSR configuration to generate some (pseudo-)randomness
lfsr_t random = {
    .state = 0xDEADBEEF, 
    .mask = 0xE10000,
};

// Predefinded colors to play with - names refer to HTML color codes.
struct cRGB colors[] = {
    {.r=0xff, .g=0x00, .b=0x00}, // red
    {.r=0xfa, .g=0x80, .b=0x72}, // salmon
    {.r=0xff, .g=0x7f, .b=0x50}, // coral
    {.r=0xff, .g=0xd7, .b=0x00}, // gold
    {.r=0xff, .g=0xff, .b=0x00}, // yellow
    {.r=0x7c, .g=0xfc, .b=0x00}, // lawn green
    {.r=0x00, .g=0xff, .b=0x00}, // lime
    {.r=0x00, .g=0xff, .b=0x7f}, // spring green
    {.r=0x2e, .g=0x8b, .b=0x57}, // sea green
    {.r=0x00, .g=0xff, .b=0xff}, // cyan
    {.r=0x40, .g=0xe0, .b=0xd0}, // turquoise
    {.r=0x00, .g=0x80, .b=0x80}, // teal
    {.r=0x00, .g=0xbf, .b=0xff}, // deepskyblue
    {.r=0x00, .g=0x00, .b=0xff}, // blue
    {.r=0xff, .g=0x00, .b=0xff}, // magenta;
    {.r=0x90, .g=0x00, .b=0x80}, // purple;
    {.r=0x90, .g=0x00, .b=0x80}, // magenta;
    {.r=0xff, .g=0x69, .b=0xb4} // hotpink;
}; 

// Quarter Sine wave for dimming. 
// Already includes an additional square function for linear increase in perceived brightness.
uint8_t sin_lut[] = {
    0.,   0.,   1.,   1.,   2.,   4.,   6.,   8.,  10.,  13.,  16.,
    19.,  22.,  26.,  30.,  34.,  38.,  43.,  48.,  53.,  58.,  64.,
    69.,  75.,  81.,  87.,  93.,  99., 105., 112., 117., 124., 131.,
    137., 143., 149., 155., 162., 168., 175., 180., 186., 192., 197.,
    202., 207., 211., 217., 220., 226., 230., 233., 235., 239., 243.,
    245., 247., 249., 251., 253., 253., 255., 255., 255.
};

// Pixel buffer. What's in here gets written out to the LEDs.
struct cRGB led[MAXPIX];

// Set all array elements to the same value. Shift color values left before writing to array for dimming.
void set_all(struct cRGB *arr, uint8_t len, struct cRGB col, uint8_t dimming) {
    for (uint8_t i = 0; i< len; i++) {
        arr[i].r = col.r >> dimming;
        arr[i].g = col.g >> dimming;
        arr[i].b = col.b >> dimming;
    }
}

// Dim all LEDs by a fixed amount
void dim_all(uint8_t amount) {
    for (uint8_t i = 0; i< MAXPIX; i++) {
        led[i].r = led[i].r < amount ? 0 : led[i].r - amount;
        led[i].g = led[i].g < amount ? 0 : led[i].g - amount;
        led[i].b = led[i].b < amount ? 0 : led[i].b - amount;
    }
}


void rollercoaster() {
    // Pre-calculated indices for figure eight loop
    static const uint8_t indices[] = {2, 3, 4, 5, 6, 7, 0, 1, 14, 13, 12, 11, 10, 9, 8, 15};

    // Pick random color
    struct cRGB c_curr = colors[lfsr_get8(&random) % LEN(colors)];

    for (uint8_t reps = 0; reps < 50; reps++) {
        // Pick next color for continuous mixing
        struct cRGB c_next = colors[lfsr_get8(&random) % LEN(colors)];

        // Calculate stepwise mixing between current and next color
        int16_t step_r = (c_next.r - c_curr.r)/LEN(indices);
        int16_t step_g = (c_next.g - c_curr.g)/LEN(indices);
        int16_t step_b = (c_next.b - c_curr.b)/LEN(indices);

        // Do one full figure eight, gradually shifting color from current to next
        for (uint8_t i = 0; i < LEN(indices); i++) {
            dim_all(1);

            // Write current color
            led[indices[i]].r = c_curr.r >> 4;
            led[indices[i]].g = c_curr.g >> 4;
            led[indices[i]].b = c_curr.b >> 4;

            // Apply mixing to current color
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
    // Indices for upper and lower half of eye from left to right
    static const uint8_t upper[] = {6,7,0,1,2,14,15,8,9,10};
    static const uint8_t lower[] = {6,5,4,3,2,14,13,12,11,10};

    for (uint8_t reps = 0; reps < 30; reps++) {
        // Pick random color
        uint8_t col = lfsr_get8(&random) % LEN(colors);

        // Do a sweep from left to right
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
        
        // Short pause before next sweep
        for (uint8_t pause = 0; pause < 50; pause++) {
            dim_all(1);
            ws2812_sendarray((uint8_t*) led, MAXPIX*3);
            _delay_ms(20);
        }
    }
}

void sparkles() {
    for (uint8_t col = 0; col < LEN(colors); col++) {
        // Pick a base color
        struct cRGB c = colors[col];
        for (uint8_t reps = 0; reps < 20; reps++) {

            dim_all(8);

            // Pick two random positions that will light up
            uint8_t ind1 = lfsr_get8(&random) % MAXPIX;
            uint8_t ind2 = lfsr_get8(&random) % MAXPIX;

            // Pick random offset from base color
            int8_t r_off = lfsr_get8(&random) >> 2;
            int8_t g_off = lfsr_get8(&random) >> 2;
            int8_t b_off = lfsr_get8(&random) >> 2;

            // Add random offset to base color
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


void flashing_back_forth() {
    struct cRGB last = {.r = 0, .g = 0, .b = 0};
    for (int reps = 0; reps < 20; reps++) {
        uint8_t idx = lfsr_get8(&random) % LEN(colors);
        struct cRGB c = colors[idx];
        
        // Increase left eye brightness, decrease right eye brightness
        for (int i = 0; i < LEN(sin_lut); i++) {
            struct cRGB c1 = {
                .r = (c.r * sin_lut[i]) / 255,
                .g = (c.g * sin_lut[i]) / 255,
                .b = (c.b * sin_lut[i]) / 255
            };
            set_all(led, MAXPIX/2, c1, 3);

            struct cRGB c2 = {
                .r = (last.r * sin_lut[LEN(sin_lut) -1 -i]) / 255,
                .g = (last.g * sin_lut[LEN(sin_lut) -1 -i]) / 255,
                .b = (last.b * sin_lut[LEN(sin_lut) -1 -i]) / 255
            };
            set_all(led+MAXPIX/2, MAXPIX/2, c2, 3);

            ws2812_sendarray((uint8_t*) led, (MAXPIX)*3);
            _delay_ms(16);
        }

        last = c;

        // Increase right eye brightness (using color that was previously in the left eye)
        // Decrese left eye brightness
        for (int i = 0; i < LEN(sin_lut); i++) {
            struct cRGB c1 = {
                .r = (c.r * sin_lut[LEN(sin_lut) -1 -i]) / 255,
                .g = (c.g * sin_lut[LEN(sin_lut) -1 -i]) / 255,
                .b = (c.b * sin_lut[LEN(sin_lut) -1 -i]) / 255
            };
            set_all(led, MAXPIX/2, c1, 3);

            struct cRGB c2 = {
                .r = (last.r * sin_lut[i]) / 255,
                .g = (last.g * sin_lut[i]) / 255,
                .b = (last.b * sin_lut[i]) / 255
            };
            set_all(led+MAXPIX/2, MAXPIX/2, c2, 3);

            ws2812_sendarray((uint8_t*) led, (MAXPIX)*3);
            _delay_ms(8);
        }
    }
}

void flashing() {
    for (int reps = 0; reps < 10; reps++) {
        uint8_t idx = lfsr_get8(&random) % LEN(colors);
        struct cRGB c = colors[idx];
        for (int i = 0; i < LEN(sin_lut); i++) {
            struct cRGB dimmed = {
                .r = (c.r * sin_lut[i]) / 255,
                .g = (c.g * sin_lut[i]) / 255,
                .b = (c.b * sin_lut[i]) / 255
            };
            set_all(led, MAXPIX, dimmed, 3);
            
            // Send first 8 buffer elements to both eyes
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
            set_all(led, MAXPIX, dimmed, 3);
            ws2812_sendarray((uint8_t*) led, (MAXPIX/2)*3);
            ws2812_sendarray((uint8_t*) led, (MAXPIX/2)*3);
            _delay_ms(20);
        }
    }
}

void circles() {
    for (int reps = 0; reps < 50; reps++) {
        uint8_t idx = lfsr_get8(&random) % LEN(colors);
        for (int i = 0; i < MAXPIX/2; i++) {
            dim_all(2);
            led[i].r = colors[idx].r >> 4;
            led[i].g = colors[idx].g >> 4;
            led[i].b = colors[idx].b >> 4;

            // Add 180 degree offset to right eye
            int idx2 = MAXPIX/2 + ((MAXPIX/4 + i) % (MAXPIX/2));
            led[idx2].r = colors[idx].r >> 4;
            led[idx2].g = colors[idx].g >> 4;
            led[idx2].b = colors[idx].b >> 4;

            ws2812_sendarray((uint8_t*) led, MAXPIX*3);
            _delay_ms(80);
        }
    }
}

int main(void)
{
    DDRB|=_BV(ws2812_pin);
    while(1)
    {
        flashing();
        scanner();
        circles();
        flashing_back_forth();
        sparkles();
        rollercoaster();
    }
}

