#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/uart.h"

// define UART connection
#define UART_ID     uart0
#define BAUD_RATE   115200
#define DATA_BITS   8
#define STOP_BITS   1
#define PARITY      UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

void init_lora() {
    uart_puts(UART_ID, "AT+NETWORKID=5\r\n");
    uart_puts(UART_ID, "AT+ADDRESS=102\r\n");
}
// uart tx using core 1
void send() {
    while(1) {
        while(uart_is_writable(UART_ID)) {
            uart_puts(UART_ID, "AT+SEND=101,5,hello\r\n");
        }
    sleep_ms(5000);
    }
}
// uart rx using core 0
int main() {
    stdio_init_all();

    // Set up our UART with the baudrate defined in NEO-6 datasheet
    uart_init(UART_ID, BAUD_RATE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // config LoRa
    init_lora();
    // send on core 1
    multicore_launch_core1(send);
    // uart rx
    while (1) {
        while (uart_is_readable(UART_ID)) {
            uint8_t ch = uart_getc(UART_ID);        
            printf("%c", ch);
        }
    }
}