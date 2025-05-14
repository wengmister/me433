#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

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

// Function to initialize the I2C and MPU6050
void init_mpu6050() {
    // Initialize I2C with 400kHz clock speed
    i2c_init(I2C_PORT, 400 * 1000);
    
    // Set up pins
    gpio_set_function(12, GPIO_FUNC_I2C); // SDA
    gpio_set_function(13, GPIO_FUNC_I2C); // SCL
    gpio_pull_up(12);
    gpio_pull_up(13);
    
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

// Function to read a single byte from the MPU6050
uint8_t read_mpu6050_register(uint8_t reg) {
    uint8_t data;
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDR, &data, 1, false);
    return data;
}

// Function to read a 16-bit value from two consecutive registers
int16_t read_mpu6050_16bit(uint8_t reg_high, uint8_t reg_low) {
    // Combine high and low bytes
    int16_t val = (int16_t)(read_mpu6050_register(reg_high) << 8 | read_mpu6050_register(reg_low));
    return val;
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

int main() {
    // Initialize stdio with USB
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("USB connected. Starting program...\n");

    // Initialize MPU6050
    init_mpu6050();
    
    // Verify communication by reading WHO_AM_I register
    uint8_t who_am_i = read_mpu6050_register(WHO_AM_I);
    printf("WHO_AM_I = 0x%02X (should be 0x68 for MPU6050)\n", who_am_i);
    
    // If device ID doesn't match, halt
    if (who_am_i != MPU6050_ADDR) {
        printf("ERROR: MPU6050 not found!\n");
        while (1) {
            // Blink LED or similar to indicate error
            sleep_ms(500);
        }
    }
    
    // Variables to store sensor data
    float accel[3], gyro[3], temp;
    
    // Read sensor data at 100Hz
    while (1) {
        // Read all data
        read_mpu6050_data(accel, gyro, &temp);
        
        // Print data to USB
        printf("Accel: X=%.3f g, Y=%.3f g, Z=%.3f g | ", 
               accel[0], accel[1], accel[2]);
        printf("Gyro: X=%.3f deg/s, Y=%.3f deg/s, Z=%.3f deg/s | ", 
               gyro[0], gyro[1], gyro[2]);
        printf("Temp: %.2f C\n", temp);
        
        // Sleep to maintain approximately 100Hz read rate
        sleep_ms(10);
    }
    
    return 0;
}