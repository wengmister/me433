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
    
    printf("Pixy2 I2C initialized on SDA=GP%d, SCL=GP%d\n", I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Test communication with version command
    uint8_t version_cmd[] = {0xae, 0xc1, 0x0e, 0x00};
    uint8_t response[16];
    
    int write_result = i2c_write_timeout_us(i2c0, PIXY2_I2C_ADDRESS, version_cmd, sizeof(version_cmd), false, 100000);
    if (write_result < 0) {
        printf("Warning: Could not communicate with Pixy2 (write failed: %d)\n", write_result);
        printf("Check I2C connections and Pixy2 power\n");
        return false;
    }
    
    sleep_ms(50);
    
    int read_result = i2c_read_timeout_us(i2c0, PIXY2_I2C_ADDRESS, response, sizeof(response), false, 100000);
    if (read_result < 0) {
        printf("Warning: Could not read from Pixy2 (read failed: %d)\n", read_result);
        return false;
    }
    
    printf("Pixy2 communication successful - received %d bytes\n", read_result);
    printf("Pixy2 ready for line following\n");
    return true;
}

int pixy2_get_line_error() {
    // Request line features from Pixy2
    uint8_t line_cmd[] = {0xae, 0xc1, 0x30, 0x02, 0x21, 0x01}; // getMainFeatures command
    uint8_t response[64];
    
    // Send command
    int write_result = i2c_write_timeout_us(i2c0, PIXY2_I2C_ADDRESS, line_cmd, sizeof(line_cmd), false, 50000);
    if (write_result < 0) {
        printf("Pixy2 write failed: %d\n", write_result);
        return LINE_NOT_FOUND;
    }
    
    // Wait for processing
    sleep_ms(30);
    
    // Read response
    int read_result = i2c_read_timeout_us(i2c0, PIXY2_I2C_ADDRESS, response, sizeof(response), false, 50000);
    if (read_result < 0) {
        printf("Pixy2 read failed: %d\n", read_result);
        return LINE_NOT_FOUND;
    }
    
    // Parse response for line data
    if (read_result >= 6) {
        uint8_t response_type = response[2];
        // The length might just be in response[3], not a 16-bit value
        uint8_t length = response[3];
        
        printf("Response type: 0x%02x, Length: %d\n", response_type, length);
        
        // Check for valid line response
        if (response_type == 0x31 && length >= 6 && read_result >= (6 + length)) {
            // For line features, data starts at response[6]
            // Line vector format: x0, y0, x1, y1, index, flags
            uint8_t x0 = response[6];
            uint8_t y0 = response[7]; 
            uint8_t x1 = response[8];
            uint8_t y1 = response[9];
            
            printf("Line vector: (%d,%d) to (%d,%d)\n", x0, y0, x1, y1);
            
            // Validate coordinates 
            if (x0 < 160 && x1 < 160 && y0 < 120 && y1 < 120 && x0 != x1) {
                
                int line_center_x = (x0 + x1) / 2;
                int frame_center = 79; // Center of 158-pixel wide frame
                
                // Calculate error as percentage (-100 to +100)
                int error = ((line_center_x - frame_center) * 100) / frame_center;
                
                // Clamp error to valid range
                if (error > 100) error = 100;
                if (error < -100) error = -100;
                
                printf("*** LINE DETECTED! Center: %d, Frame center: %d, Error: %d%% ***\n", 
                       line_center_x, frame_center, error);
                return error;
            } else {
                printf("Invalid coordinates: x0=%d, y0=%d, x1=%d, y1=%d\n", x0, y0, x1, y1);
            }
        } else {
            printf("Invalid response: type=0x%02x, length=%d, read_result=%d\n", 
                   response_type, length, read_result);
        }
    }
    
    // No valid line detected
    return LINE_NOT_FOUND;
}