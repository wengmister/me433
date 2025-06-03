#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "config.h"
#include "pixy2.h"

// Global variables
uint32_t last_line_time = 0;
bool searching = false;
int search_direction = 1;  // 1 for right, -1 for left

// Motor control functions for PH/EN mode (MODE=HIGH)
void set_motors(int left_speed, int right_speed) {
    // Clamp speeds to valid range
    if (left_speed > MAX_SPEED) left_speed = MAX_SPEED;
    if (left_speed < -MAX_SPEED) left_speed = -MAX_SPEED;
    if (right_speed > MAX_SPEED) right_speed = MAX_SPEED;
    if (right_speed < -MAX_SPEED) right_speed = -MAX_SPEED;
    
    printf("Setting motors: L=%d, R=%d\n", left_speed, right_speed);
    
    // Motor A (Left) - PH/EN mode
    if (left_speed >= 0) {
        // Forward: PH=HIGH, EN=PWM
        gpio_put(MOTOR_A_PH, 1);
        pwm_set_gpio_level(MOTOR_A_EN, left_speed);
        printf("Motor A: Forward PH=1, EN=%d\n", left_speed);
    } else {
        // Reverse: PH=LOW, EN=PWM
        gpio_put(MOTOR_A_PH, 0);
        pwm_set_gpio_level(MOTOR_A_EN, -left_speed);
        printf("Motor A: Reverse PH=0, EN=%d\n", -left_speed);
    }
    
    // Motor B (Right) - PH/EN mode
    if (right_speed >= 0) {
        // Forward: PH=HIGH, EN=PWM
        gpio_put(MOTOR_B_PH, 1);
        pwm_set_gpio_level(MOTOR_B_EN, right_speed);
        printf("Motor B: Forward PH=1, EN=%d\n", right_speed);
    } else {
        // Reverse: PH=LOW, EN=PWM
        gpio_put(MOTOR_B_PH, 0);
        pwm_set_gpio_level(MOTOR_B_EN, -right_speed);
        printf("Motor B: Reverse PH=0, EN=%d\n", -right_speed);
    }
}

void stop_motors() {
    // Stop: Set EN pins to 0 (PH doesn't matter when stopped)
    pwm_set_gpio_level(MOTOR_A_EN, 0);
    pwm_set_gpio_level(MOTOR_B_EN, 0);
    printf("Motors stopped\n");
}

void search_for_line() {
    // Spin in place to search for line
    int search_speed = BASE_SPEED / 2;
    printf("Searching for line: search_speed=%d, direction=%d\n", search_speed, search_direction);
    set_motors(-search_speed * search_direction, search_speed * search_direction);
    
    // Change search direction periodically
    static uint32_t last_direction_change = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_direction_change > 1000) {
        search_direction *= -1;
        last_direction_change = current_time;
        printf("Changing search direction to %d\n", search_direction);
    }
}

void follow_line(int error) {
    // Simple differential steering
    // error: negative = line left, positive = line right
    
    int base_speed = BASE_SPEED;
    int turn_adjustment = error * KP; // Don't multiply by KP yet, keep it simple
    
    // Calculate motor speeds
    int left_speed = base_speed - turn_adjustment;   // If error<0 (line left), left_speed increases
    int right_speed = base_speed + turn_adjustment;  // If error<0 (line left), right_speed decreases
    
    // Clamp speeds to valid range
    if (left_speed > MAX_SPEED) left_speed = MAX_SPEED;
    if (left_speed < -MAX_SPEED) left_speed = -MAX_SPEED;
    if (right_speed > MAX_SPEED) right_speed = MAX_SPEED;
    if (right_speed < -MAX_SPEED) right_speed = -MAX_SPEED;
    
    printf("→ Motors: L=%d R=%d\n", left_speed, right_speed);
    
    set_motors(left_speed, right_speed);
    searching = false;
}

bool init_hardware() {
    // Configure PH pins as digital outputs for direction control
    gpio_init(MOTOR_A_PH);
    gpio_init(MOTOR_B_PH);
    gpio_set_dir(MOTOR_A_PH, GPIO_OUT);
    gpio_set_dir(MOTOR_B_PH, GPIO_OUT);
    
    // Configure EN pins as PWM for speed control
    gpio_set_function(MOTOR_A_EN, GPIO_FUNC_PWM);
    gpio_set_function(MOTOR_B_EN, GPIO_FUNC_PWM);
    
    // Set PWM wrap value (0-255 range) for EN pins
    pwm_set_wrap(pwm_gpio_to_slice_num(MOTOR_A_EN), 255);
    pwm_set_wrap(pwm_gpio_to_slice_num(MOTOR_B_EN), 255);
    
    // Enable PWM on EN pins
    pwm_set_enabled(pwm_gpio_to_slice_num(MOTOR_A_EN), true);
    pwm_set_enabled(pwm_gpio_to_slice_num(MOTOR_B_EN), true);
    
    // Start with motors stopped
    stop_motors();
    
    printf("Motor control initialized (PH/EN mode - MODE=HIGH)\n");
    return true;
}

int main() {
    stdio_init_all();
    
    printf("\n");
    printf("========================================\n");
    printf("        LINE FOLLOWER BOT v1.0         \n");
    printf("========================================\n");
    
    // Initialize hardware
    if (!init_hardware()) {
        printf("ERROR: Hardware initialization failed!\n");
        return -1;
    }
    
    // Initialize Pixy2
    if (!pixy2_init()) {
        printf("ERROR: Pixy2 initialization failed!\n");
        return -1;
    }
    
    printf("✓ Initialization complete\n");
    printf("Starting line following in 2 seconds...\n");
    printf("========================================\n\n");
    sleep_ms(2000);
    
    // Main control loop
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        // Get line position from Pixy2
        int line_error = pixy2_get_line_error();
        
        if (line_error == LINE_NOT_FOUND) {
            // No line detected
            if (!searching) {
                searching = true;
                last_line_time = current_time;
                printf("🔍 SEARCHING for line...\n");
            }
            
            // Check if we've been searching too long
            if (current_time - last_line_time > SEARCH_TIMEOUT_MS) {
                printf("⏰ Search timeout - stopping\n");
                stop_motors();
                sleep_ms(1000);
                last_line_time = current_time;
            } else {
                // Simple search pattern
                set_motors(-60, 60); // Spin to search
            }
        } else {
            // Line detected
            printf("📍 LINE: Error=%d ", line_error);
            last_line_time = current_time;
            follow_line(line_error);
        }
        
        sleep_ms(MAIN_LOOP_DELAY_MS);
    }
    
    return 0;
}