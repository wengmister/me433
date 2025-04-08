#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

// Adjust these if you have wired differently
#define LED_PIN    16
#define BUTTON_PIN 15

int main() {
    // Initialize stdio and wait for USB connection
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("USB connected. Starting program...\n");

    // Initialize LED and Button
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);  // LED off initially

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    // Enable internal pull-up or pull-down as needed;
    // here we enable a pull-up and assume the button ties the pin to ground when pressed:
    gpio_pull_up(BUTTON_PIN);

    // Initialize ADC
    adc_init();
    adc_gpio_init(26);     // GP26 is ADC0
    adc_select_input(0);   // select ADC0

    while (1) {
        // Turn on LED
        gpio_put(LED_PIN, 1);

        // Wait until button is pressed (pin goes low if wired to ground)
        while (gpio_get(BUTTON_PIN)) {
            tight_loop_contents();  // spin here until pressed
        }

        // Turn off LED
        gpio_put(LED_PIN, 0);

        // Prompt user for number of samples
        printf("Enter the number of samples to take (1-100): ");
        fflush(stdout);

        int num_samples = 0;
        if (scanf("%d", &num_samples) != 1) {
            // If scanf fails, clear input buffer
            printf("Invalid input. Please try again.\n");
            // Consume the leftover input
            while (getchar() != '\n');
            continue; // go back to the main loop
        }

        if (num_samples < 1 || num_samples > 100) {
            printf("Please enter a value between 1 and 100.\n");
            continue; // go back to the main loop
        }

        // Read and print the ADC values
        printf("Taking %d samples at 100Hz:\n", num_samples);
        for (int i = 0; i < num_samples; i++) {
            uint16_t raw = adc_read();  // 12-bit value (0 to 4095)
            float voltage = raw * (3.3f / (1 << 12));  // Convert to volts
            printf("%f V\n", voltage);

            // Sleep 1/100s = 10ms
            sleep_ms(10);
        }
        printf("Done sampling!\n\n");
    }

    return 0; // not reached
}
