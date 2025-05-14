#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

// MPU6050 address
#define MPU6050_ADDR 0x68

// config registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C

// sensor data registers:
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H   0x41
#define TEMP_OUT_L   0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48
#define WHO_AM_I     0x75

// Conversion factors
#define ACCEL_CONVERSION_FACTOR 0.000061f  // Convert to g
#define GYRO_CONVERSION_FACTOR 0.007630f   // Convert to degrees per second

// I2C instance to use
#define I2C_PORT i2c0

// Pin definitions
#define SDA_PIN 12
#define SCL_PIN 13

// Display settings
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32
#define DISPLAY_CENTER_X (DISPLAY_WIDTH / 2)
#define DISPLAY_CENTER_Y (DISPLAY_HEIGHT / 2)
#define LINE_LENGTH_SCALE 35.0f  // Scale factor for line length

// Function to initialize the I2C and MPU6050
void init_mpu6050() {
    // Wake up the MPU6050 (clear sleep bit)
    uint8_t data[2];
    data[0] = PWR_MGMT_1;
    data[1] = 0x00;
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, data, 2, false);
    
    // Configure accelerometer for ±2g range (0x00 for ACCEL_CONFIG)
    data[0] = ACCEL_CONFIG;
    data[1] = 0x00;
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, data, 2, false);
    
    // Configure gyroscope (default range)
    data[0] = GYRO_CONFIG;
    data[1] = 0x00;
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, data, 2, false);
}

// Function to read all sensor data at once for more efficiency
void read_mpu6050_data(float *accel, float *gyro, float *temp) {
    uint8_t buffer[14];
    uint8_t reg = ACCEL_XOUT_H;
    
    // Request 14 bytes starting from ACCEL_XOUT_H (0x3B)
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, buffer, 14, false);
    
    // Combine high and low bytes and convert to physical units
    int16_t raw_accel_x = (int16_t)(buffer[0] << 8 | buffer[1]);
    int16_t raw_accel_y = (int16_t)(buffer[2] << 8 | buffer[3]);
    int16_t raw_accel_z = (int16_t)(buffer[4] << 8 | buffer[5]);
    
    int16_t raw_temp = (int16_t)(buffer[6] << 8 | buffer[7]);
    
    int16_t raw_gyro_x = (int16_t)(buffer[8] << 8 | buffer[9]);
    int16_t raw_gyro_y = (int16_t)(buffer[10] << 8 | buffer[11]);
    int16_t raw_gyro_z = (int16_t)(buffer[12] << 8 | buffer[13]);
    
    // Convert to physical units
    accel[0] = raw_accel_x * ACCEL_CONVERSION_FACTOR;  // X acceleration in g
    accel[1] = raw_accel_y * ACCEL_CONVERSION_FACTOR;  // Y acceleration in g
    accel[2] = raw_accel_z * ACCEL_CONVERSION_FACTOR;  // Z acceleration in g
    
    gyro[0] = raw_gyro_x * GYRO_CONVERSION_FACTOR;   // X rotation in deg/s
    gyro[1] = raw_gyro_y * GYRO_CONVERSION_FACTOR;   // Y rotation in deg/s
    gyro[2] = raw_gyro_z * GYRO_CONVERSION_FACTOR;   // Z rotation in deg/s
    
    *temp = raw_temp / 340.0f + 36.53f;  // Temperature in degrees C
}

// Function to draw a line using Bresenham's algorithm
void draw_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2;
    
    while (1) {
        ssd1306_drawPixel(x0, y0, 1);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            if (x0 == x1) break;
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            if (y0 == y1) break;
            err += dx;
            y0 += sy;
        }
    }
}

// Function to verify MPU6050 connection
bool check_mpu6050() {
    uint8_t who_am_i_reg = WHO_AM_I;
    uint8_t data;
    
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &who_am_i_reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, &data, 1, false);
    
    return (data == MPU6050_ADDR);
}

int main() {
    // Initialize stdio with USB
    stdio_init_all();

    // Wait for USB connection
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("USB connected. Starting program...\n");

    // Initialize I2C for both MPU6050 and SSD1306
    i2c_init(I2C_PORT, 400 * 1000);
    
    // Set up I2C pins
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    
    // Initialize MPU6050
    init_mpu6050();
    
    // Verify MPU6050 connection
    if (!check_mpu6050()) {
        printf("ERROR: MPU6050 not found!\n");
        while (1) {
            sleep_ms(500);
        }
    }
    
    printf("MPU6050 initialized successfully.\n");
    
    // Initialize OLED display
    ssd1306_setup();
    printf("SSD1306 initialized.\n");
    
    // Variables to store sensor data
    float accel[3], gyro[3], temp;
    
    // Variables for line drawing
    int line_end_x, line_end_y;
    
    // Print a startup message
    ssd1306_clear();
    ssd1306_update();
    sleep_ms(2000);
    
    // Frame rate tracking
    uint32_t frame_count = 0;
    uint32_t t_start = to_ms_since_boot(get_absolute_time());
    uint32_t t_last_fps = t_start;
    float fps = 0.0f;
    
    // Main loop
    while (1) {
        // Read IMU data
        read_mpu6050_data(accel, gyro, &temp);
        
        // Clear the display
        ssd1306_clear();
        
        // Draw a crosshair at the center (optional)
        ssd1306_drawPixel(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, 1);
        ssd1306_drawPixel(DISPLAY_CENTER_X-1, DISPLAY_CENTER_Y, 1);
        ssd1306_drawPixel(DISPLAY_CENTER_X+1, DISPLAY_CENTER_Y, 1);
        ssd1306_drawPixel(DISPLAY_CENTER_X, DISPLAY_CENTER_Y-1, 1);
        ssd1306_drawPixel(DISPLAY_CENTER_X, DISPLAY_CENTER_Y+1, 1);
        
        // Calculate line end point
        // Note: The X and Y mapping might need to be adjusted depending on the orientation
        // of your IMU and display. We may need to invert or swap X,Y depending on your setup.
        // The negative sign for Y is because positive Y on the display is downward.
        float magnitude = sqrtf(accel[0]*accel[0] + accel[1]*accel[1]);
        
        // Only draw the line if there's significant acceleration
        if (magnitude > 0.05f) {
            // Normalize and scale
            float scale = (magnitude > 1.0f) ? 1.0f : magnitude;
            scale *= LINE_LENGTH_SCALE;
            
            // Map accelerometer X and Y to display coordinates
            // Note: We may need to adjust these mappings based on your hardware orientation
            line_end_x = DISPLAY_CENTER_X - (int)(accel[0] * scale);
            line_end_y = DISPLAY_CENTER_Y + (int)(accel[1] * scale);
            
            // Ensure line_end coordinates are within display bounds
            if (line_end_x < 0) line_end_x = 0;
            if (line_end_x >= DISPLAY_WIDTH) line_end_x = DISPLAY_WIDTH - 1;
            if (line_end_y < 0) line_end_y = 0;
            if (line_end_y >= DISPLAY_HEIGHT) line_end_y = DISPLAY_HEIGHT - 1;
            
            // Draw the line
            draw_line(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, line_end_x, line_end_y);
        }
        
        // Display acceleration values (optional)
        char accel_msg[32];
        sprintf(accel_msg, "X:%.2f Y:%.2f", accel[0], accel[1]);
        
        // Calculate FPS
        frame_count++;
        uint32_t t_now = to_ms_since_boot(get_absolute_time());
        if (t_now - t_last_fps >= 1000) {
            fps = frame_count * 1000.0f / (t_now - t_last_fps);
            t_last_fps = t_now;
            frame_count = 0;
        }
        
        // Display FPS on bottom of screen
        char fps_msg[16];
        sprintf(fps_msg, "FPS:%.1f", fps);
        // ssd1306_drawMessage(0, 24, fps_msg);
        
        // Update the display
        ssd1306_update();
        
        // Print data to USB (optional - can be removed for better performance)
        printf("Accel: X=%.3f g, Y=%.3f g, Z=%.3f g | FPS: %.1f\n", 
               accel[0], accel[1], accel[2], fps);
        
        // Aim for higher refresh rate - remove delay for maximum speed
        // sleep_ms(5);  // Uncomment if you want to limit the update rate
    }
    
    return 0;
}