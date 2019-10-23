//
// Created by Kirill on 10/17/2019.
//

#include "solution.h"

void switch_off(const uint16_t color) {
    HAL_GPIO_WritePin(GPIOD, color, GPIO_PIN_RESET);
}

void switch_on(const uint16_t color) {
    HAL_GPIO_WritePin(GPIOD, color, GPIO_PIN_SET);
}

bool is_button_pressed() {
    return HAL_GPIO_ReadPin(GPIOC, BUTTON) == GPIO_PIN_RESET;
}

void check_button(struct state *current_state) {
    if (current_state->mode == 1) {
        if (is_button_pressed())
            current_state->was_button_pressed = 1;
    }
}

void send_msg(uint8_t *buf, UART_HandleTypeDef *const uart) {
    HAL_UART_Transmit(uart, buf, strlen((const char *) buf), DEFAULT_TIMEOUT);
    HAL_UART_Transmit(uart, "\n\r", strlen((const char *) "\n\r"), DEFAULT_TIMEOUT);
}

char *get_color(const struct state *const current_state) {
    char *str = calloc(255, sizeof(char));
    char *text = current_state->current_color == RED ? "red" :
                 current_state->current_color == YELLOW ? "yellow" :
                 current_state->to_blink > 0 ? "blinking green" : "green";
    strcat(str, text);
    return str;
}

char *get_mode(const struct state *const current_state) {
    char *str = calloc(255, sizeof(char));
    strcat(str, "mode ");
    char number[10];
    itoa(current_state->mode, number, 10);
    strcat(str, number);
    return str;
}

char *get_timeout(const struct state *const current_state) {
    char *str = calloc(255, sizeof(char));
    strcat(str, "timeout "); // TODO size
    char number[10]; // TODO just because I can?
    itoa(current_state->last_switch_time, number, 10);
    strcat(str, number);
    return str;
}

char *get_interrupt(const struct state *const current_state) {
	char *str = calloc(255, sizeof(char));
	strcat(str, current_state->is_interrupt_on ? "I" : "P");
	return str;
}

void on_question(UART_HandleTypeDef *const uart, const struct state *const current_state) {
    char *const color = get_color(current_state);
    char *const mode = get_mode(current_state);
    char *const timeout = get_timeout(current_state);
    char *const interrupt = get_interrupt(current_state);

    char *result = calloc(1024, sizeof(char));

    strcat(result, color);
    strcat(result, "\n\r");
    strcat(result, mode);
    strcat(result, "\n\r");
    strcat(result, timeout);
    strcat(result, "\n\r");
    strcat(result, interrupt);

    // TODO: create message
    send_msg((uint8_t *) result, uart);

    free(result);
    free(color);
    free(mode);
    free(timeout);
    free(interrupt);
}

bool starts_with(const char *const str, const char *const prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool equals(const char *const str_one, const char *const str_two) {
    return strcmp(str_one, str_two) == 0;
}

void send_echo(uint8_t *const buf, const uint16_t len, UART_HandleTypeDef *const uart) {
    HAL_UART_Transmit(uart, buf, len, DEFAULT_TIMEOUT);
}

void send_error(UART_HandleTypeDef *const uart) {
    char msg[] = "Malformed input!";
    send_msg((uint8_t *) msg, uart);
}

bool on_set_mode(const uint8_t *const buf, struct state *const current_state) {
    size_t mode_index = strlen("set mode ");
    long mode = strtol((const char *) (buf + mode_index), NULL, 10);
    if (mode < 1 || mode > 2 || errno == ERANGE) {
        return false;
    }

    current_state->mode = (uint8_t) mode;
    return true;
}

bool on_set_timeout(const uint8_t *const buf, struct state *const current_state) {
    size_t timeout_index = strlen("set timeout ");
    long timeout = strtol((const char *) (buf + timeout_index), NULL, 10);
    if (timeout <= 0 || errno == ERANGE) {
        return false;
    }

    current_state->red_timeout = (uint32_t) timeout;
    return true;
}

bool on_set_interrupts(const uint8_t *const buf, struct state *const current_state) {
    if (equals((const char *) buf, "set interrupts on")) {
        current_state->is_interrupt_on = true;
    } else if (equals((const char *) buf, "set interrupts off")) {
        current_state->is_interrupt_on = false;
    } else {
        return false;
    }

    return true;
}

void on_buf(uint8_t *buf, const uint16_t len, UART_HandleTypeDef *uart, struct state *const current_state) {
    send_echo(buf, len, uart);

    if (len == 0)
        return;

    if (equals((const char *) buf, "?")) {
        on_question(uart, current_state);
    } else if (starts_with((const char *) buf, "set mode ")) {
        if (!on_set_mode(buf, current_state))
            send_error(uart);
    } else if (starts_with((const char *) buf, "set timeout ")) {
        if (!on_set_timeout(buf, current_state))
            send_error(uart);
    } else if (starts_with((const char *) buf, "set interrupts ")) {
        if (!on_set_interrupts(buf, current_state))
            send_error(uart);
    } else {
        send_error(uart);
    }

    send_prompt(uart);
    free(buf);
}

void show_color(uint16_t color, struct state *current_state) {
    current_state->prev_color = current_state->current_color;
    current_state->current_color = color;
    current_state->last_switch_time = HAL_GetTick();

    if (current_state->prev_color != OFF)
        switch_off(current_state->prev_color);

    if (color != OFF)
        switch_on(color);
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

bool should_set_color(const struct state *current_state) {
    uint32_t timeout;

    int current_color = current_state->current_color;
    int is_blinking = (current_color == GREEN && current_state->prev_color == OFF) || current_color == OFF;

    if (current_color == RED) {
        timeout = current_state->was_button_pressed ? current_state->red_timeout / 4 : current_state->red_timeout;
    } else {
        timeout = is_blinking ? BLINK_TIMEOUT : DELAY;
    }

    uint32_t current_time = HAL_GetTick();
    uint32_t color_switch_expected = current_state->last_switch_time + timeout;

    return current_time >= color_switch_expected;
}

void check_input(UART_HandleTypeDef *uart, struct state *current_state) {
    // TODO: what if echo to our msg?
    const int buf_size = 255; // TODO: just because I can
    uint8_t *buf = calloc(buf_size, sizeof(uint8_t));
    uint8_t symbol[2] = {0};
    bool is_read = false;
    do {
    	HAL_StatusTypeDef result = HAL_UART_Receive(uart, symbol, 1, DEFAULT_TIMEOUT);
    	    switch (result) {
    	        case HAL_OK:
    	        	strcat((const char *) buf, (const char *) symbol);
    	        	is_read = true;
    	            break;

    	        case HAL_BUSY:
    	        case HAL_TIMEOUT:
    	        case HAL_ERROR:
    	            // TODO: is there anything we can do?
    	        	is_read = false;
    	        	break;
    	    }

    } while (is_read);
    int len = strlen(buf);
    if (len > 0) {
     on_buf((unsigned char *) buf, strlen((const char *) buf), uart, current_state);
    }
}

void send_prompt(UART_HandleTypeDef *uart) {
	char prompt[] ="Enter command: ";
    HAL_UART_Transmit(uart, prompt, strlen((const char *) prompt), DEFAULT_TIMEOUT);
}
