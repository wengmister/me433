#ifndef CONFIG_H
#define CONFIG_H

// Motor pins (DRV8835 PH/EN mode)
#define MOTOR_A_PHASE 16    // GP16 - Motor A Phase
#define MOTOR_A_ENABLE 17   // GP17 - Motor A Enable (PWM)
#define MOTOR_B_PHASE 18    // GP18 - Motor B Phase
#define MOTOR_B_ENABLE 19   // GP19 - Motor B Enable (PWM)

// I2C pins for Pixy2
#define I2C_SDA_PIN 20    // GP20
#define I2C_SCL_PIN 21    // GP21

// Control constants
#define BASE_SPEED 80      // Reduced base speed for better control
#define MAX_SPEED 120      // Reduced max speed to prevent extreme differences
#define MIN_SPEED 30       // Lower minimum speed
#define KP 0.3             // Much lower gain for smoother control

// PWM Configuration
#define PWM_WRAP_VALUE 1000       // Higher resolution PWM (1000 steps)
#define PWM_CLOCK_DIVIDER 125.0f  // Lower frequency: 125MHz / (125 * 1000) = 1 kHz
#define PWM_FREQUENCY_HZ 1000     // Target PWM frequency

// Pixy2 constants
#define PIXY2_I2C_ADDRESS 0x54
#define LINE_NOT_FOUND 999
#define LINE_FOLLOWING_CMD 0x30

// Timing
#define MAIN_LOOP_DELAY_MS 20
#define SEARCH_TIMEOUT_MS 2000

// Motor test and diagnostic settings
#define MOTOR_TEST_ENABLED 0      // Set to 0 to disable motor tests for normal operation
#define MOTOR_TEST_SPEED 100      // Test speed for motor diagnostics
#define MOTOR_TEST_DURATION_MS 2000  // Duration for each motor test

#endif