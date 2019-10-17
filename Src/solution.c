//
// Created by Kirill on 10/17/2019.
//

#include "solution.h"

void switch_off(uint16_t color) {
    HAL_GPIO_WritePin(GPIOD, color, GPIO_PIN_RESET);
}

void switch_on(uint16_t color) {
    HAL_GPIO_WritePin(GPIOD, color, GPIO_PIN_SET);
}

uint8_t is_button_pressed() {
    return HAL_GPIO_ReadPin(GPIOC, BUTTON) == GPIO_PIN_RESET;
}

void check_button(struct state *current_state) {
    if (current_state->mode == 1) {
        if (is_button_pressed())
            current_state->was_button_pressed = 1;
    }
}

void question(UART_HandleTypeDef *uart) {
    HAL_UART_Transmit(uart, MSG_REPLACE, strlen(MSG_REPLACE), DELAY);
    HAL_UART_Transmit(uart, EOL, strlen(EOL), DELAY);
}

void show_color(uint16_t color, struct state *current_state) {
    if (current_state->prev_color != OFF)
        switch_off(current_state->prev_color);

    if (color != OFF)
        switch_on(color);

    current_state->prev_color = current_state->current_color;
    current_state->current_color = color;
}

void handle_blink(struct state *current_state) {
    if (current_state->to_blink > 0) {
        show_color(current_state->current_color == GREEN ? OFF : GREEN, current_state);
        current_state->to_blink--;
    } else {
        show_color(YELLOW, current_state);
    }
}

void show_next_color(struct state *current_state) {
    switch (current_state->current_color) {
        case OFF:
            handle_blink(current_state);
            break;

        case GREEN:
            if (current_state->prev_color == RED) {
                show_color(OFF, current_state);
                current_state->to_blink = BLINK_COUNT;
            } else {
                handle_blink(current_state);
            }
            break;

        case YELLOW:
            show_color(RED, current_state);
            break;

        case RED:
            current_state->was_button_pressed = 0;
            show_color(GREEN, current_state);
            break;
    }
}

uint8_t should_set_color(const struct state *current_state) {
    uint32_t timeout;

    int current_color = current_state->current_color;
    int is_blinking = (current_color == GREEN && current_state->prev_color == OFF) || current_color == OFF;

    if (current_color == RED) {
        timeout = current_state->was_button_pressed ? current_state->red_timeout / 4 : current_state->red_timeout;
    } else {
        timeout = is_blinking ? BLINK_TIMEOUT : DEFAULT_TIMEOUT;
    }

    uint32_t current_tick = HAL_GetTick();
    uint32_t current_time = current_tick;
    uint32_t color_switch_expected = current_state->last_switch_time + timeout;

    return current_time >= color_switch_expected;
}
