//
// Created by Kirill on 10/17/2019.
//

#ifndef EMBEDDEDSECOND_SOLUTION_H
#define EMBEDDEDSECOND_SOLUTION_H

#include <stdint.h>
#include<string.h>
#include <stm32f4xx_hal_conf.h>

#define GREEN GPIO_PIN_13
#define YELLOW GPIO_PIN_14
#define RED GPIO_PIN_15
#define OFF 0

#define BUTTON GPIO_PIN_15

#define DELAY 1000
#define DEFAULT_TIMEOUT 3000
#define BLINK_COUNT 10
#define BLINK_TIMEOUT 100

uint8_t MSG_REPLACE[] = "Please, enter new combination";
uint8_t EOL[] = "\n\r";

struct state {
    uint32_t red_timeout;
    uint32_t last_switch_time;
    uint8_t is_interrupt_on;
    uint16_t current_color;
    uint16_t prev_color;
    uint8_t to_blink;
    uint8_t mode;
    uint8_t was_button_pressed;
};

uint8_t should_set_color(const struct state *);

void show_next_color(struct state *current_state);

void check_button(struct state *current_state);

void check_input(UART_HandleTypeDef *uart, struct state *current_state);

#endif //EMBEDDEDSECOND_SOLUTION_H
