#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"

// Motor pins (DRV8835 PH/EN mode - MODE pin HIGH)
#define MOTOR_A_PH 16     // GP16 - Motor A Phase (direction)
#define MOTOR_A_EN 17     // GP17 - Motor A Enable (speed PWM)
#define MOTOR_B_PH 18     // GP18 - Motor B Phase (direction)
#define MOTOR_B_EN 19     // GP19 - Motor B Enable (speed PWM)

// Global variables
int motor_a_duty = 0;  // -100 to +100
int motor_b_duty = 0;  // -100 to +100

void init_motors() {
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
    
    printf("✓ Motors initialized (PH/EN mode)\n");
}

void set_motor_a(int duty_percent) {
    // Convert percentage (-100 to +100) to PWM value (0-255)
    int pwm_value = (abs(duty_percent) * 255) / 100;
    if (pwm_value > 255) pwm_value = 255;
    
    if (duty_percent >= 0) {
        // Forward: PH=LOW, EN=PWM
        gpio_put(MOTOR_A_PH, 0);
        pwm_set_gpio_level(MOTOR_A_EN, pwm_value);
    } else {
        // Reverse: PH=HIGH, EN=PWM
        gpio_put(MOTOR_A_PH, 1);
        pwm_set_gpio_level(MOTOR_A_EN, pwm_value);
    }
}

void set_motor_b(int duty_percent) {
    // Convert percentage (-100 to +100) to PWM value (0-255)
    int pwm_value = (abs(duty_percent) * 255) / 100;
    if (pwm_value > 255) pwm_value = 255;
    
    if (duty_percent >= 0) {
        // Forward: PH=LOW, EN=PWM
        gpio_put(MOTOR_B_PH, 0);
        pwm_set_gpio_level(MOTOR_B_EN, pwm_value);
    } else {
        // Reverse: PH=HIGH, EN=PWM
        gpio_put(MOTOR_B_PH, 1);
        pwm_set_gpio_level(MOTOR_B_EN, pwm_value);
    }
}

void display_status() {
    printf("\n");
    printf("==========================================\n");
    printf("         MOTOR DUTY CYCLE TEST            \n");
    printf("==========================================\n");
    printf("Motor A: %+4d%%   Motor B: %+4d%%\n", motor_a_duty, motor_b_duty);
    printf("PWM A:   %3d      PWM B:   %3d\n", 
           (abs(motor_a_duty) * 255) / 100, 
           (abs(motor_b_duty) * 255) / 100);
    printf("Dir A:   %s      Dir B:   %s\n",
           motor_a_duty >= 0 ? "FWD" : "REV",
           motor_b_duty >= 0 ? "FWD" : "REV");
    printf("==========================================\n");
    printf("Controls:\n");
    printf("  + / - : Motor A duty cycle ±1%%\n");
    printf("  { / } : Motor B duty cycle ±1%%\n");
    printf("  s     : Stop both motors\n");
    printf("  q     : Quit\n");
    printf("==========================================\n");
    printf("Enter command: ");
}

int main() {
    stdio_init_all();
    
    printf("\n");
    printf("Motor Duty Cycle Test Program\n");
    
    // Initialize motors
    init_motors();
    
    // Initial display
    display_status();
    
    // Main loop
    while (true) {
        int ch = getchar_timeout_us(100000); // 100ms timeout
        
        if (ch != PICO_ERROR_TIMEOUT) {
            bool update_display = true;
            
            switch (ch) {
                case '+':
                    motor_a_duty++;
                    if (motor_a_duty > 100) motor_a_duty = 100;
                    set_motor_a(motor_a_duty);
                    break;
                    
                case '-':
                    motor_a_duty--;
                    if (motor_a_duty < -100) motor_a_duty = -100;
                    set_motor_a(motor_a_duty);
                    break;
                    
                case '{':
                    motor_b_duty++;
                    if (motor_b_duty > 100) motor_b_duty = 100;
                    set_motor_b(motor_b_duty);
                    break;
                    
                case '}':
                    motor_b_duty--;
                    if (motor_b_duty < -100) motor_b_duty = -100;
                    set_motor_b(motor_b_duty);
                    break;
                    
                case 's':
                case 'S':
                    motor_a_duty = 0;
                    motor_b_duty = 0;
                    set_motor_a(0);
                    set_motor_b(0);
                    printf("🛑 Motors stopped\n");
                    break;
                    
                case 'q':
                case 'Q':
                    printf("🚪 Exiting program...\n");
                    set_motor_a(0);
                    set_motor_b(0);
                    return 0;
                    
                default:
                    update_display = false;
                    break;
            }
            
            if (update_display) {
                display_status();
            }
        }
        
        sleep_ms(10); // Small delay to prevent overwhelming the system
    }
    
    return 0;
}