#include "pixy2.h"
#include "config.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include <stdio.h>

bool pixy2_init() {
    // Initialize I2C
    i2c_init(i2c0, 400000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    
    printf("Pixy2 I2C initialized\n");
    printf("=== POWER DIAGNOSTIC ===\n");
    printf("Check these voltages with multimeter:\n");
    printf("- Pico2 3V3(OUT) Pin 36: Should be ~3.3V\n");
    printf("- Pico2 VSYS Pin 39: Should be your battery voltage\n");
    printf("- Between your 5V supply + and -: Should be 5.0V\n");
    printf("- Pixy2 VCC to GND: Should match your power source\n");
    printf("========================\n");
    
    return true;
}

int pixy2_get_line_error() {
    // Try a simpler approach - just request version first to test communication
    printf("Testing Pixy2 communication...\n");
    
    // Simple version command
    uint8_t cmd[] = {0xae, 0xc1, 0x0e, 0x00}; // getVersion command
    uint8_t response[64];
    
    // Send command
    int write_result = i2c_write_timeout_us(i2c0, PIXY2_I2C_ADDRESS, cmd, sizeof(cmd), false, 100000);
    if (write_result < 0) {
        printf("Failed to send command: %d\n", write_result);
        return LINE_NOT_FOUND;
    }
    
    // Wait for processing
    sleep_ms(50);
    
    // Read response
    int read_result = i2c_read_timeout_us(i2c0, PIXY2_I2C_ADDRESS, response, sizeof(response), false, 100000);
    if (read_result < 0) {
        printf("Failed to read response: %d\n", read_result);
        return LINE_NOT_FOUND;
    }
    
    printf("Version response - Received %d bytes: ", read_result);
    int display_count = (read_result < 16) ? read_result : 16;
    for (int i = 0; i < display_count; i++) {
        printf("0x%02x ", response[i]);
    }
    printf("\n");
    
    // Now try line tracking with a different approach
    printf("Requesting line features...\n");
    
    // Alternative line command format
    uint8_t line_cmd[] = {0xae, 0xc1, 0x30, 0x02, 0x21, 0x01}; // getMainFeatures with different params
    
    write_result = i2c_write_timeout_us(i2c0, PIXY2_I2C_ADDRESS, line_cmd, sizeof(line_cmd), false, 100000);
    if (write_result < 0) {
        printf("Failed to send line command: %d\n", write_result);
        return LINE_NOT_FOUND;
    }
    
    sleep_ms(50);
    
    read_result = i2c_read_timeout_us(i2c0, PIXY2_I2C_ADDRESS, response, sizeof(response), false, 100000);
    if (read_result < 0) {
        printf("Failed to read line response: %d\n", read_result);
        return LINE_NOT_FOUND;
    }
    
    printf("Line response - Received %d bytes: ", read_result);
    display_count = (read_result < 16) ? read_result : 16;
    for (int i = 0; i < display_count; i++) {
        printf("0x%02x ", response[i]);
    }
    printf("\n");
    
    // Check if we get a proper response
    if (read_result >= 6) {
        uint8_t response_type = response[2];
        uint16_t length = response[3] | (response[4] << 8);
        printf("Response type: 0x%02x, Length: %d\n", response_type, length);
        
        // Accept various response types that might contain line data
        if (response_type == 0x31 || response_type == 0x21) {
            if (length > 0 && read_result >= 10) {
                uint8_t x0 = response[6];
                uint8_t y0 = response[7]; 
                uint8_t x1 = response[8];
                uint8_t y1 = response[9];
                
                printf("Potential line vector: (%d,%d) to (%d,%d)\n", x0, y0, x1, y1);
                
                // Check for valid coordinates (not 0x80 or 0xff)
                if (x0 != 0x80 && x0 != 0xff && x1 != 0x80 && x1 != 0xff) {
                    int line_center_x = (x0 + x1) / 2;
                    int frame_center = 79;
                    int error = ((line_center_x - frame_center) * 100) / frame_center;
                    
                    if (error > 100) error = 100;
                    if (error < -100) error = -100;
                    
                    printf("*** LINE DETECTED! Center: %d, Error: %d ***\n", line_center_x, error);
                    return error;
                }
            }
        }
    }
    
    // For testing, return occasional simulated detections to verify motor control
    static int counter = 0;
    counter++;
    if (counter % 20 == 0) {
        printf("*** SIMULATION: Line detected for testing ***\n");
        return (counter / 20) % 21 - 10; // Cycle through -10 to +10
    }
    
    printf("No valid line data found\n");
    return LINE_NOT_FOUND;
}