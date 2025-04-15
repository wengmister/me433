#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/clocks.h"
#include "pico/time.h"
#include <math.h>

// -----------------------------------------------------------------------------
// SPI configuration
// Using SPI0 with shared pins for both external RAM and DAC.
// DAC chip-select (CS) is on GPIO20.
// External RAM chip-select (CS) is on GPIO21.
#define SPI_PORT      spi0
#define PIN_MISO      16
#define PIN_SCK       18
#define PIN_MOSI      19
#define PIN_DAC_CS    20   // For DAC
#define PIN_RAM_CS    21   // For external SRAM

// -----------------------------------------------------------------------------
// External SRAM op codes (23K256)
// WRITE: Write command opcode (0x02)
// READ:  Read command opcode (0x03)
// WRMR: Write Mode Register command opcode (0x01)
#define SRAM_WRITE    0x02
#define SRAM_READ     0x03
#define SRAM_WRMR     0x01

// Number of sine samples to store in external RAM.
// With one sample per millisecond, 1000 samples gives a 1 Hz cycle.
#define NUM_SAMPLES 1000

// -----------------------------------------------------------------------------
// DAC command construction (for a typical HW4 DAC)
// The DAC expects a 16-bit command word.
// For Channel A (using DAC CS on GPIO20) the configuration bits are:
//   Bit15 = 0 (channel A)
//   Bit14 = 1 (buffer enabled)
//   Bit13 = 1 (gain = 1x)
//   Bit12 = 1 (active mode)
// Thus, we shift 0b0111 into the upper nibble and OR in the 10-bit sample (shifted left by 2).
// (The DAC is assumed to be a 10-bit DAC.)
 
// -----------------------------------------------------------------------------
// Inline chip-select helper functions.
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

// -----------------------------------------------------------------------------
// spi_ram_init()
// Initializes the 23K256 SRAM in sequential mode.
// In this device, the Write Mode Register (WRMR) command (0x01)
// must be followed by the mode byte. Here, we use 0x40 (0b01000000)
// to set sequential mode.
void spi_ram_init() {
    uint8_t tx_buf[2];
    tx_buf[0] = SRAM_WRMR;  // Write Mode Register command
    tx_buf[1] = 0x40;       // 0b01000000 -> Sequential mode
    
    cs_select(PIN_RAM_CS);
    spi_write_blocking(SPI_PORT, tx_buf, 2);
    cs_deselect(PIN_RAM_CS);
}

// -----------------------------------------------------------------------------
// Main function
int main() {
    // Initialize stdio over USB.
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("USB connected. Starting program...\n");
    
    // Initialize SPI0 at 1 MHz.
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Initialize DAC and SRAM chip-select GPIOs as SIO.
    gpio_init(PIN_DAC_CS);
    gpio_set_dir(PIN_DAC_CS, GPIO_OUT);
    gpio_put(PIN_DAC_CS, 1);
    
    gpio_init(PIN_RAM_CS);
    gpio_set_dir(PIN_RAM_CS, GPIO_OUT);
    gpio_put(PIN_RAM_CS, 1);
    
    // Initialize external RAM (23K256) in sequential mode.
    spi_ram_init();

    // Get system clock frequency in Hz
    uint32_t freq_hz = clock_get_hz(clk_sys);
    uint32_t freq_mhz = freq_hz / 1000000; // e.g., 125 for 125 MHz
    printf("System clock frequency: %u Hz (%u MHz)\n", freq_hz, freq_mhz);

    // Part 1: Floating-point speed test
    volatile float f1, f2;
    printf("Enter two floats to use: ");
    scanf("%f %f", &f1, &f2);
    printf("f1 = %f, f2 = %f\n", f1, f2);

    volatile float f_add, f_sub, f_mult, f_div;
    int loop_count = 1000;

    absolute_time_t t1, t2;
    uint64_t elapsed_us, elapsed_cycles, single_cycle;

    // Addition
    t1 = get_absolute_time();
    for (int i = 0; i < loop_count; i++) {
        f_add = f1 + f2;
    }
    t2 = get_absolute_time();
    elapsed_us = to_us_since_boot(t2) - to_us_since_boot(t1);  // Correct order
    elapsed_cycles = elapsed_us * freq_mhz;  // Approximate cycles
    single_cycle = elapsed_cycles / loop_count;  // Average cycle per operation
    printf("Time to add %d times: %llu us (%llu cycles, %llu for a single cycle)\n", loop_count, elapsed_us, elapsed_cycles, single_cycle);

    // Subtraction
    t1 = get_absolute_time();
    for (int i = 0; i < loop_count; i++) {
        f_sub = f1 - f2;
    }
    t2 = get_absolute_time();
    elapsed_us = to_us_since_boot(t2) - to_us_since_boot(t1);
    elapsed_cycles = elapsed_us * freq_mhz;
    single_cycle = elapsed_cycles / loop_count;  // Average cycle per operation
    printf("Time to subtract %d times: %llu us (%llu cycles, %llu for a single cycle)\n", loop_count, elapsed_us, elapsed_cycles, single_cycle);

    // Multiplication
    t1 = get_absolute_time();
    for (int i = 0; i < loop_count; i++) {
        f_mult = f1 * f2;
    }
    t2 = get_absolute_time();
    elapsed_us = to_us_since_boot(t2) - to_us_since_boot(t1);
    elapsed_cycles = elapsed_us * freq_mhz;
    single_cycle = elapsed_cycles / loop_count;  // Average cycle per operation
    printf("Time to multiply %d times: %llu us (%llu cycles, %llu for a single cycle)\n", loop_count, elapsed_us, elapsed_cycles, single_cycle);

    // Division
    t1 = get_absolute_time();
    for (int i = 0; i < loop_count; i++) {
        f_div = f1 / f2;
    }
    t2 = get_absolute_time();
    elapsed_us = to_us_since_boot(t2) - to_us_since_boot(t1);
    elapsed_cycles = elapsed_us * freq_mhz;
    single_cycle = elapsed_cycles / loop_count;  // Average cycle per operation
    printf("Time to divide %d times: %llu us (%llu cycles, %llu for a single cycle)\n", loop_count, elapsed_us, elapsed_cycles, single_cycle);

    // Print result examples
    printf("\nResults:\n");
    printf("%f + %f = %f\n", f1, f2, f_add);
    printf("%f - %f = %f\n", f1, f2, f_sub);
    printf("%f * %f = %f\n", f1, f2, f_mult);
    printf("%f / %f = %f\n", f1, f2, f_div);

    printf("\n\n");

    printf("Starting sine wave generation...\n");
    
    // -------------------------------------------------------------------------
    // Prepare one full cycle of the sine wave samples (1000 floats).
    // The sine values are computed over 0 to 2*PI.
    // The computed sine (in the range -1 to 1) is normalized to 0 to 1,
    // then scaled by 3.3 to represent voltage.
    float sine_wave_samples[NUM_SAMPLES];
    for (int i = 0; i < NUM_SAMPLES; i++) {
        float phase = (2.0f * M_PI * i) / NUM_SAMPLES;
        float sample = ((sinf(phase) + 1.0f) / 2.0f) * 3.3f; // scale to 0-3.3V
        sine_wave_samples[i] = sample;
    }
    
    // -------------------------------------------------------------------------
    // Write the 1000 float samples into external RAM.
    // Use the WRITE command (0x02) followed by a 16-bit address (we start at 0).
    // In sequential mode the address automatically increments.
    uint8_t header[3];
    header[0] = SRAM_WRITE;
    header[1] = 0x00;  // high address byte (starting address 0x0000)
    header[2] = 0x00;  // low address byte
    
    cs_select(PIN_RAM_CS);
    // Send the header (command + starting address).
    spi_write_blocking(SPI_PORT, header, 3);
    // Send the sample data. Each float is 4 bytes.
    spi_write_blocking(SPI_PORT, (uint8_t *)sine_wave_samples, NUM_SAMPLES * sizeof(float));
    cs_deselect(PIN_RAM_CS);
    
    printf("Loaded %d sine wave samples into external RAM.\n", NUM_SAMPLES);
    
    // -------------------------------------------------------------------------
    // Playback loop:
    // In each loop iteration, read one float from external RAM, convert it for the DAC,
    // transmit the DAC command, and delay 1 ms.
    // This produces a 1 Hz sine wave output.
    int sample_index = 0;
    while (true) {
        // Prepare read header for external SRAM:
        // Use the READ command (0x03) and a 16-bit address. The address is sample_index * 4.
        uint8_t read_header[3];
        read_header[0] = SRAM_READ;
        uint16_t address = sample_index * sizeof(float);
        read_header[1] = (address >> 8) & 0xFF;
        read_header[2] = address & 0xFF;
        
        float sample;
        cs_select(PIN_RAM_CS);
        // Send header then read 4 bytes (the float)
        spi_write_blocking(SPI_PORT, read_header, 3);
        spi_read_blocking(SPI_PORT, 0, (uint8_t *)&sample, sizeof(float));
        cs_deselect(PIN_RAM_CS);
        
        // Convert the float (0-3.3V) from SRAM to a 10-bit DAC value.
        // Normalize by dividing by 3.3 and multiply by 1023.
        float normalized = sample / 3.3f;
        if (normalized > 1.0f) normalized = 1.0f;
        if (normalized < 0.0f) normalized = 0.0f;
        uint16_t dac_val = (uint16_t)(normalized * 1023);
        
        // Build DAC command word for channel A.
        // Command word format: (0b0111 << 12) | (dac_val << 2).
        uint16_t command_word = (0b0111 << 12) | (dac_val << 2);
        uint8_t dac_data[2];
        dac_data[0] = (command_word >> 8) & 0xFF;
        dac_data[1] = command_word & 0xFF;
        
        // Send the DAC command via SPI.
        cs_select(PIN_DAC_CS);
        spi_write_blocking(SPI_PORT, dac_data, 2);
        cs_deselect(PIN_DAC_CS);
        
        // Move to the next sample; wrap around after one full cycle.
        sample_index = (sample_index + 1) % NUM_SAMPLES;
        
        // Wait for 1 ms.
        sleep_ms(1);
    }
    
    return 0;
}
