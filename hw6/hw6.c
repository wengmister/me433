#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

// I2C on pins 14 (SDA) and 15 (SCL) at 400 kHz
#define I2C_PORT    i2c1
#define I2C_SDA     14
#define I2C_SCL     15
#define I2C_FREQ    400000

// MCP23008 7‑bit address (A2,A1,A0 = GND)
#define MCP_ADDR    0x20

// MCP23008 register addresses
#define REG_IODIR   0x00  // I/O direction (1 = input)
#define REG_GPPU    0x06  // Pull‑up resistor (1 = enabled)
#define REG_GPIO    0x09  // Port value (read)
#define REG_OLAT    0x0A  // Output latch (write)

// Heartbeat LED (onboard Pico LED is GPIO 25)
#define HEARTBEAT_LED  PICO_DEFAULT_LED_PIN
#define BLINK_MS       500

int main() {
    stdio_init_all();

    // Initialize I2C at 400 kHz
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Setup heartbeat LED
    gpio_init(HEARTBEAT_LED);
    gpio_set_dir(HEARTBEAT_LED, GPIO_OUT);

    // 1) Configure GP0 as input, GP7 as output
    uint8_t buf[2];
    buf[0] = REG_IODIR;
    buf[1] = 0x01;   // bit0 = 1 (input), bits1–7 = 0 (outputs)
    i2c_write_blocking(I2C_PORT, MCP_ADDR, buf, 2, false);

    // 2) Enable pull‑up on GP0
    buf[0] = REG_GPPU;
    buf[1] = 0x01;   // pull‑up on bit0
    i2c_write_blocking(I2C_PORT, MCP_ADDR, buf, 2, false);

    // 3) Initialize outputs to 0 (GP7 low)
    buf[0] = REG_OLAT;
    buf[1] = 0x00;
    i2c_write_blocking(I2C_PORT, MCP_ADDR, buf, 2, false);

    // Heartbeat timer
    absolute_time_t last_blink = get_absolute_time();
    bool led_state = false;

    while (true) {
        // --- Heartbeat blink every BLINK_MS ---
        if (absolute_time_diff_us(last_blink, get_absolute_time()) > BLINK_MS * 1000) {
            last_blink = get_absolute_time();
            led_state = !led_state;
            gpio_put(HEARTBEAT_LED, led_state);
        }

        // --- Read button state on GP0 ---
        buf[0] = REG_GPIO;
        // Tell MCP we want to read GPIO:
        i2c_write_blocking(I2C_PORT, MCP_ADDR, buf, 1, true);
        // Read one byte back:
        uint8_t gpio_val = 0;
        i2c_read_blocking(I2C_PORT, MCP_ADDR, &gpio_val, 1, false);

        // Determine output for GP7:
        //   if button pressed (GP0 low), set GP7 = 1
        //   else GP7 = 0
        bool pressed = !(gpio_val & 0x01);
        buf[0] = REG_OLAT;
        buf[1] = pressed ? (1u << 7) : 0x00;
        i2c_write_blocking(I2C_PORT, MCP_ADDR, buf, 2, false);

        // Small delay so we don’t hammer the bus
        sleep_ms(10);
    }

    return 0;
}
