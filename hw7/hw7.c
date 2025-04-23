#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

#include "ssd1306.h"
#include "font.h"

// -- pin assignments
#define SDA_PIN            12
#define SCL_PIN            13
#define I2C_PORT           i2c_default   // maps to i2c0
#define ADC1_GPIO          27            // ADC1 → GPIO27 on Pico
#ifndef PICO_DEFAULT_LED_PIN
  #define PICO_DEFAULT_LED_PIN 25
#endif

// ------------------------------------------------------------------
// drawChar & drawMessage as before...
void ssd1306_drawChar(uint8_t x, uint8_t y, char c) {
    if (c < 0x20 || c > 0x7F) return;
    const uint8_t *glyph = ASCII[c - 0x20];
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (uint8_t row = 0; row < 8; row++) {
            ssd1306_drawPixel(x + col, y + row, (bits >> row) & 1);
        }
    }
}

void ssd1306_drawMessage(uint8_t x, uint8_t y, const char *msg) {
    uint8_t cx = x;
    for (const char *p = msg; *p; ++p) {
        ssd1306_drawChar(cx, y, *p);
        cx += 6;
        if (cx + 5 >= 128) break;
    }
    ssd1306_update();
}
// ------------------------------------------------------------------

int main() {
    stdio_init_all();

    // LED heartbeat
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // I2C for SSD1306
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // ADC1 setup
    adc_init();
    adc_gpio_init(ADC1_GPIO);
    adc_select_input(1);

    // OLED init
    ssd1306_setup();

    // Main loop
    while (true) {
        // 1) toggle LED
        gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);

        // 2) clear display buffer
        ssd1306_clear();

        // 3) timestamp before drawing ADC value
        uint32_t t0 = to_us_since_boot(get_absolute_time());

        // 4) read ADC and print
        uint16_t raw = adc_read();  // 0–4095
        char adc_msg[32];
        sprintf(adc_msg, "ADC1 = %u", raw);
        ssd1306_drawMessage(0, 0, adc_msg);

        // 5) timestamp after drawMessage → compute FPS
        uint32_t t1 = to_us_since_boot(get_absolute_time());
        uint32_t dt = t1 - t0;
        uint32_t fps = dt ? (1000000u / dt) : 0;

        // 6) draw FPS on bottom (y=24)
        char fps_msg[32];
        sprintf(fps_msg, "FPS = %u", fps);
        ssd1306_drawMessage(0, 24, fps_msg);

        // 7) small delay to control sample rate
        sleep_ms(10);
    }

    return 0;
}
