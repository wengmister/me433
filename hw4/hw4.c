#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <math.h>

// SPI Defines. These pins are used for SPI0:
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   20
#define PIN_SCK  18
#define PIN_MOSI 19

// Chip select functions. The inline assembly nops provide a small delay before/after toggling.
static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop");
}

int main() {
    stdio_init_all();

    // Initialize SPI0 with a baud rate of 1mHz.
    spi_init(SPI_PORT, 1000*1000); // 1 MHz SPI clock
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    // Set the chip-select pin for manual control.
    gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // Sine wave parameters (Channel A):
    // A 2 Hz sine wave has a period of 0.5 sec.
    // With an update rate of 100 Hz (every 10 ms), we need 50 samples per cycle.
    const float update_interval_ms = 10.0f;  // 10 ms update period
    const int sine_samples_per_cycle = 50;
    const float sine_phase_increment = (2.0f * M_PI) / sine_samples_per_cycle;
    float sine_phase = 0.0f;

    // Triangle wave parameters (Channel B):
    // A 1 Hz triangle wave has a period of 1 sec.
    // With a 100 Hz update rate, we need 100 samples per cycle.
    const int triangle_samples_per_cycle = 100;
    int triangle_index = 0; // Ranges from 0 to 99 over one full cycle.

    while (true) {
        /////////// Channel A: 2 Hz Sine Wave ///////////
        // Compute the sine wave sample.
        // sinf returns values between -1 and 1, so we normalize to a 0–1 range.
        float sine_val = sinf(sine_phase);
        float sine_normalized = (sine_val + 1.0f) / 2.0f;
        // Scale to a 10-bit value (0–1023).
        uint16_t sine_dac_value = (uint16_t)(sine_normalized * 1023);
        // Build the 16-bit command word:
        // For Channel A, configuration bits are:
        //   Bit15 = 0 (Channel A)
        //   Bit14 = 1 (buffer enabled)
        //   Bit13 = 1 (gain = 1x)
        //   Bit12 = 1 (active mode)
        // Hence: (0b0111 << 12) | (sine_dac_value << 2)
        uint16_t command_word_A = (0b0111 << 12) | (sine_dac_value << 2);
        // Pack the command word into two bytes (big-endian).
        uint8_t data_A[2];
        data_A[0] = (command_word_A >> 8) & 0xFF;
        data_A[1] = command_word_A & 0xFF;
        // Transmit the command for Channel A:
        cs_select(PIN_CS);
        spi_write_blocking(SPI_PORT, data_A, 2);
        cs_deselect(PIN_CS);

        /////////// Channel B: 1 Hz Triangle Wave ///////////
        // Generate the triangle wave sample.
        float triangle_normalized;
        if (triangle_index < (triangle_samples_per_cycle / 2)) {
            // Rising slope: from 0 to 1 over the first half-cycle.
            triangle_normalized = (float)triangle_index / (triangle_samples_per_cycle / 2);
        } else {
            // Falling slope: from 1 back to 0 during the second half-cycle.
            triangle_normalized = (float)(triangle_samples_per_cycle - triangle_index) / (triangle_samples_per_cycle / 2);
        }
        // Convert the normalized value to a 10-bit value.
        uint16_t triangle_dac_value = (uint16_t)(triangle_normalized * 1023);
        // Build the 16-bit command word for Channel B:
        // For Channel B, set Bit15 = 1 (to select Channel B) while keeping other configuration bits:
        //   Bit15 = 1 (Channel B)
        //   Bit14 = 1 (buffer enabled)
        //   Bit13 = 1 (gain = 1x)
        //   Bit12 = 1 (active mode)
        // Hence: (0b1111 << 12) | (triangle_dac_value << 2)
        uint16_t command_word_B = (0b1111 << 12) | (triangle_dac_value << 2);
        // Pack the command word into two bytes (big-endian).
        uint8_t data_B[2];
        data_B[0] = (command_word_B >> 8) & 0xFF;
        data_B[1] = command_word_B & 0xFF;
        // Transmit the command for Channel B:
        cs_select(PIN_CS);
        spi_write_blocking(SPI_PORT, data_B, 2);
        cs_deselect(PIN_CS);

        // Update the sine wave phase.
        sine_phase += sine_phase_increment;
        if (sine_phase >= 2.0f * M_PI) {
            sine_phase -= 2.0f * M_PI;
        }

        // Update the triangle wave index.
        triangle_index++;
        if (triangle_index >= triangle_samples_per_cycle)
            triangle_index = 0;

        // Wait for the next update.
        sleep_ms((int)update_interval_ms);
    }

    return 0;
}
