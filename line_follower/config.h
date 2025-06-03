#ifndef CONFIG_H
#define CONFIG_H

// Motor pins (DRV8835 PH/EN mode - MODE pin HIGH)
#define MOTOR_A_PH 16     // GP16 - Motor A Phase (direction)
#define MOTOR_A_EN 17     // GP17 - Motor A Enable (speed PWM)
#define MOTOR_B_PH 18     // GP18 - Motor B Phase (direction)
#define MOTOR_B_EN 19     // GP19 - Motor B Enable (speed PWM)

// I2C pins for Pixy2
#define I2C_SDA_PIN 20    // GP20
#define I2C_SCL_PIN 21    // GP21

// Control constants - stronger motor outputs
#define BASE_SPEED 100    // Increased base speed for stronger output
#define MAX_SPEED 300     // Higher max speed
#define MIN_SPEED 40      // Higher minimum to overcome friction
#define KP 5           // Increased gain for stronger steering response

// Line detection calibration
#define LINE_CENTER_OFFSET 70  // Offset to correct for camera/mounting bias

// Pixy2 constants
#define PIXY2_I2C_ADDRESS 0x54
#define LINE_NOT_FOUND 999
#define LINE_FOLLOWING_CMD 0x30

// Timing
#define MAIN_LOOP_DELAY_MS 20
#define SEARCH_TIMEOUT_MS 2000

#endif