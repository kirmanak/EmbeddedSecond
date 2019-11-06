//
// Created by Kirill on 10/17/2019.
//

#ifndef EMBEDDEDSECOND_SOLUTION_H
#define EMBEDDEDSECOND_SOLUTION_H

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>

#define GREEN GPIO_PIN_13
#define YELLOW GPIO_PIN_14
#define RED GPIO_PIN_15
#define OFF 0

#define BUTTON GPIO_PIN_15

#define DELAY 2000
#define DEFAULT_TIMEOUT 15
#define BLINK_COUNT 10
#define BLINK_TIMEOUT 200

typedef struct state {
    uint32_t red_timeout;
    uint32_t last_switch_time;
    bool is_interrupt_on;
    uint16_t current_color;
    uint16_t prev_color;
    uint8_t to_blink;
    uint8_t mode;
    bool was_button_pressed;
} state;

bool should_set_color(const state *);

void show_next_color(state *current_state);

void check_button(state *current_state);

void check_input(UART_HandleTypeDef *uart, state *current_state, unsigned char *);

void send_prompt(UART_HandleTypeDef *uart);

void send_msg(uint8_t *buf, UART_HandleTypeDef *const uart);

#endif //EMBEDDEDSECOND_SOLUTION_H
