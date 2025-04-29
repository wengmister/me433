/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "pico/multicore.h"
 #include "hardware/adc.h"
 
 #define FLAG_VALUE 123
 
 void core1_entry() {
     // —— handshake ——
     multicore_fifo_push_blocking(FLAG_VALUE);
     uint32_t hand = multicore_fifo_pop_blocking();
     if (hand != FLAG_VALUE) {
         printf("Core1 handshake FAILED: got %u\n", hand);
     } else {
         printf("Core1 ready\n");
     }
 
     // Initialize LED on GP15
     const uint LED_PIN = 15;
     gpio_init(LED_PIN);
     gpio_set_dir(LED_PIN, GPIO_OUT);
     gpio_put(LED_PIN, 0);
 
     // Initialize ADC0 on A0 (GPIO26)
     adc_init();
     adc_gpio_init(26);
 
     while (1) {
         uint32_t cmd = multicore_fifo_pop_blocking();
         switch (cmd) {
             case 0: {
                 // read A0
                 adc_select_input(0);
                 uint16_t raw = adc_read();
                 // convert to millivolts (0–4095 → 0–3300 mV)
                 uint32_t mV = (uint32_t)raw * 3300 / 4095;
                 multicore_fifo_push_blocking(mV);
                 break;
             }
             case 1:
                 gpio_put(LED_PIN, 1);
                 multicore_fifo_push_blocking(FLAG_VALUE);
                 break;
             case 2:
                 gpio_put(LED_PIN, 0);
                 multicore_fifo_push_blocking(FLAG_VALUE);
                 break;
             default:
                 // unknown command
                 multicore_fifo_push_blocking(FLAG_VALUE);
                 break;
         }
     }
 }
 
 int main() {
     stdio_init_all();
     sleep_ms(100);             // allow USB CDC to enumerate
     printf("Core0: Hello, multicore!\n");
 
     // launch Core1
     multicore_launch_core1(core1_entry);
 
     // —— handshake ——
     uint32_t hand = multicore_fifo_pop_blocking();
     if (hand != FLAG_VALUE) {
         printf("Core0 handshake FAILED: got %u\n", hand);
     } else {
         multicore_fifo_push_blocking(FLAG_VALUE);
         printf("Core0 ready\n");
     }
 
     while (1) {
         int cmd;
         printf("\nEnter command (0=read A0, 1=LED on, 2=LED off): ");
         if (scanf("%d", &cmd) != 1) {
             // flush bad input
             int c;
             while ((c = getchar()) != '\n' && c != EOF);
             continue;
         }
 
         multicore_fifo_push_blocking((uint32_t)cmd);
 
         if (cmd == 0) {
             uint32_t mV = multicore_fifo_pop_blocking();
             printf("Voltage on A0: %.3f V\n", mV / 1000.0f);
         } else if (cmd == 1) {
             multicore_fifo_pop_blocking();
             printf("LED turned ON\n");
         } else if (cmd == 2) {
             multicore_fifo_pop_blocking();
             printf("LED turned OFF\n");
         } else {
             multicore_fifo_pop_blocking();
             printf("Unknown command\n");
         }
     }
 }
 