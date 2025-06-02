#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <cmath> // For std::abs
#include "config.h"
#include "pixy2.h"

// Global variables
uint32_t last_line_time = 0;
bool searching = false;
int search_direction = 1;  // 1 for right, -1 for left

// Motor diagnostic functions
void print_motor_diagnostics() {
    printf("\n=== MOTOR CONFIGURATION DIAGNOSTICS ===\n");
    printf("Motor A Phase Pin (GP%d): %s\n", MOTOR_A_PHASE, 
           gpio_get_dir(MOTOR_A_PHASE) == GPIO_OUT ? "OUTPUT" : "INPUT");
    printf("Motor B Phase Pin (GP%d): %s\n", MOTOR_B_PHASE, 
           gpio_get_dir(MOTOR_B_PHASE) == GPIO_OUT ? "OUTPUT" : "INPUT");
    
    printf("Motor A Enable Pin (GP%d): PWM Function\n", MOTOR_A_ENABLE);
    printf("Motor B Enable Pin (GP%d): PWM Function\n", MOTOR_B_ENABLE);
    
    uint slice_a = pwm_gpio_to_slice_num(MOTOR_A_ENABLE);
    uint slice_b = pwm_gpio_to_slice_num(MOTOR_B_ENABLE);
    printf("PWM Slice A: %d, Slice B: %d\n", slice_a, slice_b);
    
    printf("PWM Wrap Value: %d\n", PWM_WRAP_VALUE);
    printf("PWM Clock Divider: %.1f\n", PWM_CLOCK_DIVIDER);
    printf("Expected PWM Frequency: %d Hz\n", PWM_FREQUENCY_HZ);
    printf("========================================\n\n");
}

void print_drv8835_mode_info() {
    printf("\n=== DRV8835 MODE CONFIGURATION ===\n");
    printf("Current code assumes PH/EN mode:\n");
    printf("- PHASE pin (GP%d & GP%d): Controls direction\n", MOTOR_A_PHASE, MOTOR_B_PHASE);
    printf("- ENABLE pin (GP%d & GP%d): Controls speed via PWM\n", MOTOR_A_ENABLE, MOTOR_B_ENABLE);
    printf("\nDRV8835 MODE pin must be connected to VCC for PH/EN mode!\n");
    printf("If MODE pin is connected to GND, you're in IN/IN mode.\n");
    printf("===================================\n\n");
}

// Enhanced motor control functions using PH/EN mode
void set_motors(int left_speed, int right_speed) {
    // Clamp speeds to the range [-MAX_SPEED, MAX_SPEED]
    if (left_speed > MAX_SPEED) left_speed = MAX_SPEED;
    if (left_speed < -MAX_SPEED) left_speed = -MAX_SPEED;
    if (right_speed > MAX_SPEED) right_speed = MAX_SPEED;
    if (right_speed < -MAX_SPEED) right_speed = -MAX_SPEED;
    
    printf("Setting motors (PH/EN): L_speed=%d, R_speed=%d\n", left_speed, right_speed);

    // Calculate PWM duty cycles (0 to PWM_WRAP_VALUE)
    uint16_t left_pwm = static_cast<uint16_t>((std::abs(left_speed) * PWM_WRAP_VALUE) / MAX_SPEED);
    uint16_t right_pwm = static_cast<uint16_t>((std::abs(right_speed) * PWM_WRAP_VALUE) / MAX_SPEED);

    // Motor A (Left)
    if (left_speed > 0) { // Forward
        gpio_put(MOTOR_A_PHASE, 1);
        pwm_set_gpio_level(MOTOR_A_ENABLE, left_pwm);
        printf("Motor A: FWD, PH=1, EN_PWM=%u (%.1f%%)\n", left_pwm, (float)left_pwm/PWM_WRAP_VALUE*100);
    } else if (left_speed < 0) { // Reverse
        gpio_put(MOTOR_A_PHASE, 0);
        pwm_set_gpio_level(MOTOR_A_ENABLE, left_pwm);
        printf("Motor A: REV, PH=0, EN_PWM=%u (%.1f%%)\n", left_pwm, (float)left_pwm/PWM_WRAP_VALUE*100);
    } else { // Stop
        gpio_put(MOTOR_A_PHASE, 0);
        pwm_set_gpio_level(MOTOR_A_ENABLE, 0);
        printf("Motor A: STOP, EN_PWM=0\n");
    }
    
    // Motor B (Right)
    if (right_speed > 0) { // Forward
        gpio_put(MOTOR_B_PHASE, 1);
        pwm_set_gpio_level(MOTOR_B_ENABLE, right_pwm);
        printf("Motor B: FWD, PH=1, EN_PWM=%u (%.1f%%)\n", right_pwm, (float)right_pwm/PWM_WRAP_VALUE*100);
    } else if (right_speed < 0) { // Reverse
        gpio_put(MOTOR_B_PHASE, 0);
        pwm_set_gpio_level(MOTOR_B_ENABLE, right_pwm);
        printf("Motor B: REV, PH=0, EN_PWM=%u (%.1f%%)\n", right_pwm, (float)right_pwm/PWM_WRAP_VALUE*100);
    } else { // Stop
        gpio_put(MOTOR_B_PHASE, 0);
        pwm_set_gpio_level(MOTOR_B_ENABLE, 0);
        printf("Motor B: STOP, EN_PWM=0\n");
    }
}

void stop_motors() {
    gpio_put(MOTOR_A_PHASE, 0);
    pwm_set_gpio_level(MOTOR_A_ENABLE, 0);
    gpio_put(MOTOR_B_PHASE, 0);
    pwm_set_gpio_level(MOTOR_B_ENABLE, 0);
    printf("Motors stopped (PH/EN mode)\n");
}

// Comprehensive motor test function for PH/EN mode
void motor_test_ph_en() {
    printf("\n=== COMPREHENSIVE MOTOR TEST (PH/EN MODE) ===\n");
    
    printf("Test 1: Motor A Forward\n");
    set_motors(MOTOR_TEST_SPEED, 0);
    sleep_ms(MOTOR_TEST_DURATION_MS);
    stop_motors();
    sleep_ms(500);
    
    printf("Test 2: Motor A Reverse\n");
    set_motors(-MOTOR_TEST_SPEED, 0);
    sleep_ms(MOTOR_TEST_DURATION_MS);
    stop_motors();
    sleep_ms(500);
    
    printf("Test 3: Motor B Forward\n");
    set_motors(0, MOTOR_TEST_SPEED);
    sleep_ms(MOTOR_TEST_DURATION_MS);
    stop_motors();
    sleep_ms(500);
    
    printf("Test 4: Motor B Reverse\n");
    set_motors(0, -MOTOR_TEST_SPEED);
    sleep_ms(MOTOR_TEST_DURATION_MS);
    stop_motors();
    sleep_ms(500);
    
    printf("Test 5: Both Motors Forward\n");
    set_motors(MOTOR_TEST_SPEED, MOTOR_TEST_SPEED);
    sleep_ms(MOTOR_TEST_DURATION_MS);
    stop_motors();
    sleep_ms(500);
    
    printf("Test 6: Both Motors Reverse\n");
    set_motors(-MOTOR_TEST_SPEED, -MOTOR_TEST_SPEED);
    sleep_ms(MOTOR_TEST_DURATION_MS);
    stop_motors();
    sleep_ms(500);
    
    printf("Test 7: Turn Left (Right motor forward, Left motor reverse)\n");
    set_motors(-MOTOR_TEST_SPEED, MOTOR_TEST_SPEED);
    sleep_ms(MOTOR_TEST_DURATION_MS);
    stop_motors();
    sleep_ms(500);
    
    printf("Test 8: Turn Right (Left motor forward, Right motor reverse)\n");
    set_motors(MOTOR_TEST_SPEED, -MOTOR_TEST_SPEED);
    sleep_ms(MOTOR_TEST_DURATION_MS);
    stop_motors();
    sleep_ms(500);
    
    printf("=== MOTOR TEST COMPLETE ===\n\n");
}

// PWM ramp test for debugging
void pwm_ramp_test() {
    printf("\n=== PWM RAMP TEST ===\n");
    printf("Testing Motor A with PWM ramp...\n");
    
    gpio_put(MOTOR_A_PHASE, 1); // Forward direction
    
    // Ramp up
    for (int i = 0; i <= PWM_WRAP_VALUE; i += PWM_WRAP_VALUE/10) {
        pwm_set_gpio_level(MOTOR_A_ENABLE, i);
        printf("PWM Level: %d (%.1f%%)\n", i, (float)i/PWM_WRAP_VALUE*100);
        sleep_ms(500);
    }
    
    // Ramp down
    for (int i = PWM_WRAP_VALUE; i >= 0; i -= PWM_WRAP_VALUE/10) {
        pwm_set_gpio_level(MOTOR_A_ENABLE, i);
        printf("PWM Level: %d (%.1f%%)\n", i, (float)i/PWM_WRAP_VALUE*100);
        sleep_ms(500);
    }
    
    stop_motors();
    printf("=== PWM RAMP TEST COMPLETE ===\n\n");
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
    // Simple proportional control
    int turn_speed = error * KP;
    int left_speed = BASE_SPEED - turn_speed;
    int right_speed = BASE_SPEED + turn_speed;
    
    printf("Line following: error=%d, turn_speed=%d\n", error, turn_speed);
    printf("Before limits: left=%d, right=%d\n", left_speed, right_speed);
    
    // Clamp speeds to valid range
    if (left_speed > MAX_SPEED) left_speed = MAX_SPEED;
    if (left_speed < -MAX_SPEED) left_speed = -MAX_SPEED;
    if (right_speed > MAX_SPEED) right_speed = MAX_SPEED;
    if (right_speed < -MAX_SPEED) right_speed = -MAX_SPEED;

    // Ensure minimum speed for both motors if they are supposed to be moving
    if (left_speed != 0) {
        if (left_speed > 0 && left_speed < MIN_SPEED) left_speed = MIN_SPEED;
        if (left_speed < 0 && left_speed > -MIN_SPEED) left_speed = -MIN_SPEED;
    }
    if (right_speed != 0) {
        if (right_speed > 0 && right_speed < MIN_SPEED) right_speed = MIN_SPEED;
        if (right_speed < 0 && right_speed > -MIN_SPEED) right_speed = -MIN_SPEED;
    }
    
    printf("Final motor speeds: left=%d, right=%d\n", left_speed, right_speed);
    set_motors(left_speed, right_speed);
    searching = false;
}

bool init_hardware() {
    printf("Initializing motor hardware...\n");
    
    // Configure PHASE pins as digital outputs
    gpio_init(MOTOR_A_PHASE);
    gpio_set_dir(MOTOR_A_PHASE, GPIO_OUT);
    gpio_put(MOTOR_A_PHASE, 0); // Initialize to low
    
    gpio_init(MOTOR_B_PHASE);
    gpio_set_dir(MOTOR_B_PHASE, GPIO_OUT);
    gpio_put(MOTOR_B_PHASE, 0); // Initialize to low

    // Configure ENABLE pins for PWM
    gpio_set_function(MOTOR_A_ENABLE, GPIO_FUNC_PWM);
    gpio_set_function(MOTOR_B_ENABLE, GPIO_FUNC_PWM);

    uint slice_a_en = pwm_gpio_to_slice_num(MOTOR_A_ENABLE);
    uint slice_b_en = pwm_gpio_to_slice_num(MOTOR_B_ENABLE);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, PWM_CLOCK_DIVIDER);
    pwm_config_set_wrap(&config, PWM_WRAP_VALUE);

    // Initialize PWM for both slices
    pwm_init(slice_a_en, &config, true);
    if (slice_a_en != slice_b_en) {
        pwm_init(slice_b_en, &config, true);
    }
    
    // Ensure both PWM outputs are enabled
    pwm_set_enabled(slice_a_en, true);
    if (slice_a_en != slice_b_en) {
        pwm_set_enabled(slice_b_en, true);
    }
    
    // Start with motors stopped
    stop_motors();
    
    printf("Motor control initialized (PH/EN mode)\n");
    printf("PWM Wrap: %d, Clock Divider: %.1f\n", PWM_WRAP_VALUE, PWM_CLOCK_DIVIDER);
    printf("Calculated PWM Frequency: %.1f Hz\n", 125000000.0f / (PWM_CLOCK_DIVIDER * (PWM_WRAP_VALUE + 1)));
    
    return true;
}

int main() {
    stdio_init_all();

    // Wait for USB connection
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("USB connected. Starting program...\n");
    
    printf("Line Follower Bot Starting...\n");
    
    // Print diagnostic information
    print_drv8835_mode_info();
    
    // Initialize hardware
    if (!init_hardware()) {
        printf("Hardware initialization failed!\n");
        return -1;
    }
    
    // Print motor diagnostics
    print_motor_diagnostics();
    
    // Initialize Pixy2
    if (!pixy2_init()) {
        printf("Pixy2 initialization failed! Continuing anyway...\n");
        printf("You can still test motor control manually\n");
    }
    
    // Run motor tests if enabled
    if (MOTOR_TEST_ENABLED) {
        printf("Running motor tests...\n");
        motor_test_ph_en();
        pwm_ramp_test();
        printf("Motor tests complete. Starting line following...\n");
    }
    
    printf("===== STARTING LINE FOLLOWING MODE =====\n");
    printf("The robot will:\n");
    printf("- Follow lines when detected by Pixy2\n");
    printf("- Search by spinning when no line is found\n");
    printf("- Stop after %d ms of no line detection\n", SEARCH_TIMEOUT_MS);
    printf("==========================================\n");
    
    sleep_ms(1000);

    // Main control loop
    while (true) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        
        int line_error = pixy2_get_line_error();
        
        if (line_error == LINE_NOT_FOUND) {
            if (!searching) {
                searching = true;
                last_line_time = current_time;
                printf("Line lost, starting search\n");
            }
            
            if (current_time - last_line_time > SEARCH_TIMEOUT_MS) {
                printf("Search timeout, stopping motors\n");
                stop_motors();
                sleep_ms(1000);
                last_line_time = current_time;
            } else {
                 search_for_line();
            }
        } else {
            last_line_time = current_time;
            follow_line(line_error);
        }
        
        sleep_ms(MAIN_LOOP_DELAY_MS);
    }
    
    return 0;
}