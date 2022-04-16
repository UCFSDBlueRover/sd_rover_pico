/**
 * @file config.h
 * @author Wesley Fletcher (wkfletcher@knights.ucf.edu)
 * @brief function prototypes and definitions for config.c
 * @version 0.1
 * @date 2022-04-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef CONFIG_H
#define CONFIG_H

// hardware includes
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

// function prototypes
int configure_UART(uart_inst_t *UART_ID, uint BAUDRATE, uint TX_PIN, 
                   uint RX_PIN, uint DATA_BITS, uint STOP_BITS, uint PARITY, 
                   irq_handler_t IRQ_FUN, bool useIRQ);

#endif