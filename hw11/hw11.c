#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED     = 1000,
  BLINK_SUSPENDED   = 2500,
};

// GPIO button pin assignments
#define BUTTON_UP       2
#define BUTTON_LEFT     3
#define BUTTON_DOWN     4
#define BUTTON_RIGHT    5
#define BUTTON_TOGGLE   9   // Toggle mode button

// LED to indicate mode (circle/manual)
#define LED_MODE_PIN    15

// Speed levels based on button hold time (in milliseconds)
#define SPEED_LEVEL_1_TIME 0     // Initial speed
#define SPEED_LEVEL_2_TIME 500   // After 0.5 seconds
#define SPEED_LEVEL_3_TIME 1500  // After 1.5 seconds
#define SPEED_LEVEL_4_TIME 3000  // After 3 seconds

// Mouse movement delta values for each speed level
#define SPEED_LEVEL_1 3
#define SPEED_LEVEL_2 7
#define SPEED_LEVEL_3 15
#define SPEED_LEVEL_4 30

// Circle parameters
#define CIRCLE_RADIUS   5     // pixels
#define ANGLE_INC       0.1f  // radians per report

// Button state tracking
typedef struct {
    bool is_pressed;
    uint32_t press_time;
} button_state_t;

static button_state_t button_states[4]; // Up, Left, Down, Right
static bool last_toggle_state = false;
static bool circle_mode      = false;

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

void led_blinking_task(void);
void hid_task(void);
void button_init(void);
int8_t get_speed_delta(uint32_t hold_time);

/*------------- MAIN -------------*/
int main(void)
{
  // Initialize board
  board_init();
  
  // Initialize GPIO for buttons and mode LED
  button_init();

  // Init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();
    hid_task();
  }
}

// Initialize GPIO pins for buttons and LED
void button_init(void) {
    // Initialize all button pins as inputs with pull-ups
    for (int i = BUTTON_UP; i <= BUTTON_RIGHT; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_IN);
        gpio_pull_up(i);
    }
    // Mode toggle button
    gpio_init(BUTTON_TOGGLE);
    gpio_set_dir(BUTTON_TOGGLE, GPIO_IN);
    gpio_pull_up(BUTTON_TOGGLE);

    // Mode LED
    gpio_init(LED_MODE_PIN);
    gpio_set_dir(LED_MODE_PIN, GPIO_OUT);
    gpio_put(LED_MODE_PIN, 0);
    
    // Initialize button states
    for (int i = 0; i < 4; i++) {
        button_states[i].is_pressed = false;
        button_states[i].press_time  = 0;
    }
    last_toggle_state = false;
    circle_mode      = false;
}

// Calculate movement speed based on hold time
int8_t get_speed_delta(uint32_t hold_time) {
    if (hold_time >= SPEED_LEVEL_4_TIME) {
        return SPEED_LEVEL_4;
    } else if (hold_time >= SPEED_LEVEL_3_TIME) {
        return SPEED_LEVEL_3;
    } else if (hold_time >= SPEED_LEVEL_2_TIME) {
        return SPEED_LEVEL_2;
    } else {
        return SPEED_LEVEL_1;
    }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms   = 0;

  if (board_millis() - start_ms < interval_ms) return;
  start_ms += interval_ms;

  if (!tud_hid_ready()) return;

  // Handle suspend remote wakeup
  if (tud_suspended()) {
    for (int i = BUTTON_UP; i <= BUTTON_RIGHT; i++) {
      if (!gpio_get(i)) {
        tud_remote_wakeup();
        break;
      }
    }
    return;
  }

  // Check mode toggle button (edge detect)
  bool toggle_now = !gpio_get(BUTTON_TOGGLE);
  if (toggle_now && !last_toggle_state) {
    circle_mode = !circle_mode;
    gpio_put(LED_MODE_PIN, circle_mode);
  }
  last_toggle_state = toggle_now;

  // Circle mode: move mouse in a circle
  if (circle_mode) {
    static float angle = 0.0f;
    angle += ANGLE_INC;
    if (angle >= 2 * M_PI) angle -= 2 * M_PI;
    int8_t x = (int8_t)(CIRCLE_RADIUS * cosf(angle));
    int8_t y = (int8_t)(CIRCLE_RADIUS * sinf(angle));
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, x, y, 0, 0);
    return;
  }

  // Manual mode: read buttons for directional control
  int8_t x = 0;
  int8_t y = 0;
  uint32_t now = board_millis();

  // UP
  bool up = !gpio_get(BUTTON_UP);
  if (up) {
    if (!button_states[0].is_pressed) {
      button_states[0].is_pressed  = true;
      button_states[0].press_time = now;
    }
    y -= get_speed_delta(now - button_states[0].press_time);
  } else {
    button_states[0].is_pressed = false;
  }
  // LEFT
  bool left = !gpio_get(BUTTON_LEFT);
  if (left) {
    if (!button_states[1].is_pressed) {
      button_states[1].is_pressed  = true;
      button_states[1].press_time = now;
    }
    x -= get_speed_delta(now - button_states[1].press_time);
  } else {
    button_states[1].is_pressed = false;
  }
  // DOWN
  bool down = !gpio_get(BUTTON_DOWN);
  if (down) {
    if (!button_states[2].is_pressed) {
      button_states[2].is_pressed  = true;
      button_states[2].press_time  = now;
    }
    y += get_speed_delta(now - button_states[2].press_time);
  } else {
    button_states[2].is_pressed = false;
  }
  // RIGHT
  bool right = !gpio_get(BUTTON_RIGHT);
  if (right) {
    if (!button_states[3].is_pressed) {
      button_states[3].is_pressed  = true;
      button_states[3].press_time  = now;
    }
    x += get_speed_delta(now - button_states[3].press_time);
  } else {
    button_states[3].is_pressed = false;
  }

  if (x != 0 || y != 0) {
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, x, y, 0, 0);
  }
}

// The rest of the callbacks (get_report, set_report, etc.) remain unchanged...

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) instance; (void) report; (void) len;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen)
{
  (void) instance; (void) report_id; (void) report_type;
  (void) buffer; (void) reqlen; return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;
  if (report_type == HID_REPORT_TYPE_OUTPUT && report_id == REPORT_ID_KEYBOARD) {
    if (bufsize < 1) return;
    uint8_t kbd_leds = buffer[0];
    if (kbd_leds & KEYBOARD_LED_CAPSLOCK) {
      blink_interval_ms = 0;
      board_led_write(true);
    } else {
      board_led_write(false);
      blink_interval_ms = BLINK_MOUNTED;
    }
}
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+

void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state    = false;

  if (!blink_interval_ms) return;
  if (board_millis() - start_ms < blink_interval_ms) return;
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = !led_state;
}
