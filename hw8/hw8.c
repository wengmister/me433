#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "ws2812.pio.h"

#define NUM_PIXELS 4
#define WS2812_PIN 10    // your data pin for the NeoPixels
#define SERVO_PIN   9    // your servo signal pin
#define IS_RGBW    false // false = RGB only

//——— simple 3-byte color struct ———
typedef struct {
    uint8_t r, g, b;
} wsColor;

//——— HSB→RGB, returns 0–255 per channel ———
wsColor HSBtoRGB(float hue, float sat, float val) {
    float r = 0, g = 0, b = 0;
    if (sat <= 0.0f) {
        r = g = b = val;
    } else {
        if (hue >= 360.0f) hue -= 360.0f;
        int i = hue / 60.0f;
        float f = (hue / 60.0f) - i;
        float p = val * (1.0f - sat);
        float q = val * (1.0f - sat * f);
        float t = val * (1.0f - sat * (1.0f - f));
        switch (i) {
            case 0:  r=val; g=t;   b=p;   break;
            case 1:  r=q;   g=val; b=p;   break;
            case 2:  r=p;   g=val; b=t;   break;
            case 3:  r=p;   g=q;   b=val; break;
            case 4:  r=t;   g=p;   b=val; break;
            case 5:  r=val; g=p;   b=q;   break;
        }
    }
    wsColor c = {
        .r = (uint8_t)(r * 255.0f),
        .g = (uint8_t)(g * 255.0f),
        .b = (uint8_t)(b * 255.0f)
    };
    return c;
}

//——— push one GRB word into PIO ———
static inline void put_pixel(PIO pio, uint sm, uint32_t grb) {
    pio_sm_put_blocking(pio, sm, grb << 8u);
}

//——— send entire frame[], then latch (>50 µs) ———
void ws2812_show(PIO pio, uint sm, wsColor *frame) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        uint32_t grb =
            ((uint32_t)frame[i].g << 16) |
            ((uint32_t)frame[i].r <<  8) |
            ((uint32_t)frame[i].b      );
        put_pixel(pio, sm, grb);
    }
    sleep_ms(1);
}

int main() {
    stdio_init_all();
    printf("Rainbow + Servo demo\n");

    //——— set up PIO for NeoPixels ———
    PIO pio;
    uint sm, offset;
    bool ok = pio_claim_free_sm_and_add_program_for_gpio_range(
        &ws2812_program, &pio, &sm, &offset,
        WS2812_PIN, 1, true
    );
    if (!ok) {
        printf("PIO allocate failed!\n");
        return 1;
    }
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // set up PWM for servo @50 Hz (20 ms period)
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    uint chan  = pwm_gpio_to_channel(SERVO_PIN);

    pwm_config cfg = pwm_get_default_config();
    // sysclock 150 MHz / clkdiv=150 → 1 MHz tick → wrap=20000 → 20 ms period
    pwm_config_set_clkdiv(&cfg, 150.0f);
    pwm_init(slice, &cfg, false);
    pwm_set_wrap(slice, 20000);
    pwm_set_enabled(slice, true);

    //——— precompute LED hue offsets (0°,90°,180°,270°) ———
    float led_offset[NUM_PIXELS];
    for (int i = 0; i < NUM_PIXELS; i++)
        led_offset[i] = (360.0f / NUM_PIXELS) * i;

    //——— timing for 5 s full cycle ———
    const int total_steps   = 360;
    const int total_time_ms = 5000;
    const int delay_ms      = total_time_ms / total_steps;

    wsColor frame[NUM_PIXELS];

    //——— main loop: step h = 0→359, wrap ———
    for (int h = 0; ; h = (h + 1) % total_steps) {
        // -- update NeoPixels --
        for (int i = 0; i < NUM_PIXELS; i++) {
            float hue = h + led_offset[i];
            if (hue >= 360.0f) hue -= 360.0f;
            frame[i] = HSBtoRGB(hue, 1.0f, 1.0f);
        }
        ws2812_show(pio, sm, frame);

        // -- update servo: 0→180→0° triangular in same 360 steps --
        float angle;
        if (h < total_steps/2) {
            // rising half-cycle
            angle = (h / (float)(total_steps/2 - 1)) * 180.0f;
        } else {
            // falling half-cycle
            angle = ((total_steps - h) / (float)(total_steps/2 - 1)) * 180.0f;
        }
        // now map to [500, 2500] ticks (0.5–2.5 ms):
        uint16_t pulse = 500 + (uint16_t)((angle / 180.0f) * 2000.0f);
        pwm_set_chan_level(slice, chan, pulse);

        sleep_ms(delay_ms);
    }

    return 0;
}
