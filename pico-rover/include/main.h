/**
 * @file main.h
 * @author Wesley Fletcher (wkfletcher@knights.ucf.edu)
 * @brief Function prototypes for the "main" program to be loaded onto the Pico
 * @version 0.1
 * @date 2022-03-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef MAIN_H
#define MAIN_H

// general includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// hardware includes
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

// function prototypes
int handle_input(char *in);

// callback functions
void on_UART_GPS_rx();
void on_UART_LORA_rx();

#endif



