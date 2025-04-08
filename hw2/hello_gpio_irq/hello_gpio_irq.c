#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define GPIO_WATCH_PIN 15

// ~50ms debounce time (in microseconds)
#define DEBOUNCE_US 50000

static char event_str[128];

// Forward declaration
void gpio_event_string(char *buf, uint32_t events);

// Callback function for GPIO interrupts
void gpio_callback(uint gpio, uint32_t events) {
    static absolute_time_t last_call_time = {0};

    // Get the current time
    absolute_time_t now = get_absolute_time();
    // Calculate time difference (microseconds) since the last interrupt
    int64_t diff_us = absolute_time_diff_us(last_call_time, now);

    // If this interrupt is too close to the last one, ignore it
    if (diff_us < DEBOUNCE_US) {
        return;
    }

    // Update last_call_time
    last_call_time = now;

    // Convert event bits to human-readable string
    gpio_event_string(event_str, events);
    printf("GPIO %d %s\n", gpio, event_str);
}

int main() {
    stdio_init_all();
    printf("Hello GPIO IRQ with Debounce\n");

    // Initialize the GPIO
    gpio_init(GPIO_WATCH_PIN);

    // Optionally, enable internal pull-up or pull-down to have a stable default state.
    // For a button that connects GPIO 15 to GND when pressed, you'd enable a pull-up:
    gpio_pull_up(GPIO_WATCH_PIN);  

    // Now enable the interrupt on RISE and FALL, and register our callback
    gpio_set_irq_enabled_with_callback(
        GPIO_WATCH_PIN,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true,
        &gpio_callback
    );

    // Idle forever, waiting for interrupts to fire the callback
    while (1) {
        tight_loop_contents();
    }
}


// String labels for the events corresponding to bits 0x1, 0x2, 0x4, 0x8
static const char *gpio_irq_str[] = {
    "LEVEL_LOW",  // 0x1
    "LEVEL_HIGH", // 0x2
    "EDGE_FALL",  // 0x4
    "EDGE_RISE"   // 0x8
};

void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            // Copy this event string into the user string
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
            events &= ~mask;

            // If more events remain, add a comma and space
            if (events) {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf = '\0';
}
