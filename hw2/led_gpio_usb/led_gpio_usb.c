#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define BUTTON_PIN 15 // points to GP#, not pin#
#define LED_PIN    16

// ~100ms debounce time (in microseconds)
#define DEBOUNCE_US 100000

// Global count of button presses
static uint32_t press_count = 0;

// A buffer for printing event strings (optional)
static char event_str[128];

static const char *gpio_irq_str[] = {
    "LEVEL_LOW",   // 0x1
    "LEVEL_HIGH",  // 0x2
    "EDGE_FALL",   // 0x4
    "EDGE_RISE"    // 0x8
};

// Helper to convert the event bits into a string
void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
            events &= ~mask;
            if (events) {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf = '\0';
}

// Interrupt callback for the button
void gpio_callback(uint gpio, uint32_t events) {
    // Debounce logic: ignore events that occur too quickly after the last valid one
    static absolute_time_t last_call_time = {0};

    absolute_time_t now = get_absolute_time();
    int64_t diff_us = absolute_time_diff_us(last_call_time, now);

    // If this interrupt is too close to the last one, ignore it
    if (diff_us < DEBOUNCE_US) {
        return;
    }

    // Record the time of this valid event
    last_call_time = now;

    // Optional: convert the event bits to a string for debugging
    gpio_event_string(event_str, events);

    // We only want to *count* an actual press (usually on a falling edge if using pull-up).
    // But if you'd rather count every transition (rising + falling), remove the check below.
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Toggle LED
        static bool led_on = false;
        led_on = !led_on;
        gpio_put(LED_PIN, led_on);

        // Increment press count
        press_count++;

        // Print a message over USB
        printf("Button pressed %u times (GPIO %d %s)\n", press_count, gpio, event_str);
    }
}

int main() {
    // Initialize USB stdio
    stdio_init_all();
    printf("=== Pico Button + LED + USB Demo ===\n");

    // Initialize the LED pin as output, start off
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, false);

    // Initialize the button pin, enable internal pull-up
    gpio_init(BUTTON_PIN);
    gpio_pull_up(BUTTON_PIN);

    // Enable an interrupt on both rising + falling edges
    // (You can restrict it to just falling if you prefer.)
    gpio_set_irq_enabled_with_callback(
        BUTTON_PIN,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &gpio_callback
    );

    // Main loop does nothing; the callback handles the button logic
    while (true) {
        tight_loop_contents();
    }
}
