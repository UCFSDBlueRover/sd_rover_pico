/**
 * @file main.c
 * @author Wesley Fletcher (wkfletcher@knights.ucf.edu)
 * @brief main entrypoint to the peripheral MCU program
 * @version 0.1
 * @date 2022-04-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "main.h"
#include "../include/definitions.h"
#include "../include/comms.h"
#include "../include/motors.h"
#include "../include/config.h"

/**
 * @brief RX interrupt for GPS over UART, blocks until message is terminated. Prints received data, sending it across the serial connection.
 * 
 */
void on_UART_GPS_rx()
{
    char buffer[83];    // max size of NMEA sentence is 82 bytes (according to NMEA-0183) + 1 for termination (\0)
    int idx = 0;
    
    while (uart_is_readable(UART_ID_GPS)) // TODO: replace with timeout version?
    {
        uint8_t ch = uart_getc(UART_ID_GPS); 
        while (ch != ENDSTDIN)
        {
            buffer[idx++] = ch;

            // if the string ends or we run out of space, we're done with this string
            if (ch == CR || ch == NL || idx == (sizeof(buffer)-1))
            {
                buffer[idx] = 0; // terminate the string
                idx = 0;    // reset index
                
                break;
            }
            ch = uart_getc(UART_ID_GPS);
        }
        // don't send empty buffers
        if (strlen(buffer) > 1)
        {
            printf("$GPS %s\n", buffer);
        }
    }
}

/**
 * @brief RX interrupt for LORA over UART, blocks until message is terminated. Not actually used.
 * 
 */
void on_UART_LORA_rx()
{
    return;
}

/**
 * @brief   process a given string, dispatch based on contents
 * 
 * @param in the input string
 * @return status of input handling (EXIT_SUCCESS/EXIT_FAILURE)
 */
int handle_input(char *in)
{
    // for tokenizing input string
    char * delim = " ";
    char * token;
    int seq;

    // for $MTR commands
    bool dir1 = 0, dir2 = 0;
    int8_t pwm1 = 0, pwm2 = 0;

    // tokenize string (strtok modifies the original string)
    token = strtok(in, delim);

    // "switch" on first token == message type
    // ACK messages are used for confirmation that sent data was received
    if (strcmp(token, MSG_ACK) == 0)
    {
        // check the seq number
        token = strtok(NULL, delim);
        printf("Next token: %s", token);
        seq = atoi(token);
        printf("Got an ACK message. SEQ: %d\n", seq);
        return EXIT_SUCCESS;
    }
    // CMD messages come from the GS, are to be passed up to the SBC with printf
    else if (strcmp(token, MSG_CMD) == 0)
    {        
        token = strtok(NULL, "");
        // to avoid having to copy the string, just re-adding '$CMD' manually
        printf("$CMD %s\n", token);
        return EXIT_SUCCESS;
    }
    // MTR messages are used for PWM commands through the Pico
    else if (strcmp(token, MSG_MOTORS) == 0)
    {
        // unpack message to vars
        token = strtok(NULL, delim);
        dir1 = (strcmp(token, "0") != 0);   // DIR1
        token = strtok(NULL, delim);
        pwm1 = atoi(token);                 // PWM1
        token = strtok(NULL, delim);
        dir2 = (strcmp(token, "0") != 0);   // DIR2
        token = strtok(NULL, delim);
        pwm2 = atoi(token);                 // PWM2

        // debug
        printf("DIR1: %d\nPWM1: %d\nDIR2: %d\nPWM2: %d\n", dir1, pwm1, dir2, pwm2);
        // set PWM using vars from message
        set_PWM(dir1, pwm1, dir2, pwm2);
        return EXIT_SUCCESS;
    }
    else if (strcmp(token, MSG_REQ) == 0)
    {
        // will depend
        return EXIT_SUCCESS;
    }
    // TX messages are from the SBC, meant to be transmitted on LORA to the GS
    else if (strcmp(token, MSG_TX) == 0)
    {
        // transmit over UART
        return EXIT_SUCCESS;
    }
    // TLM messages are meant to be transmitted, need to be added to queue
    else if (strcmp(token, MSG_TLM) == 0)
    {        
        token = strtok(NULL, "");
        printf("$TLM message: %s\n", token);
        if (!queue_try_add(&transmit_queue, token)) 
        {
            printf("$ERR Failed to add $TLM message to transmit queue: %s\n", token); 
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // something went wrong
    return EXIT_FAILURE;
}

/**
 * @brief Program entrypoint.
 * 
 * @return int success/failure
 */
int main() 
{
    stdio_init_all();

    // queue_t receive_queue;
    // queue_t transmit_queue;

    // STDIN/STDOUT IO
    char ch;
    int idx = 0;
    char in_string[255];
    char out_string[255];
    char received_data[LORA_SIZE];
    char sent_data[LORA_SIZE] = "data";
    int status;

    // give everything time to turn on
    sleep_ms(2000);

    // configure UART for GPS
    status = configure_UART(UART_ID_GPS,
                            BAUD_RATE_GPS,
                            UART_TX_PIN_GPS, UART_RX_PIN_GPS,
                            DATA_BITS_GPS, STOP_BITS_GPS, PARITY_GPS,
                            on_UART_GPS_rx, 1);
    if (status)
    {
        printf("$ERR Failed to initialize UART for GPS.\n");
        // return EXIT_FAILURE;
    }

    // // configure UART for LORA
    // status = configure_UART(UART_ID_LORA,
    //                         BAUD_RATE_LORA,
    //                         UART_TX_PIN_LORA, UART_RX_PIN_LORA,
    //                         DATA_BITS_LORA, STOP_BITS_LORA, PARITY_LORA,
    //                         on_UART_LORA_rx, 0);
    // if (status)
    // {
    //     printf("$ERR Failed to initialize UART for LoRa.\n");
    //     // return EXIT_FAILURE;
    // }

    // // init inter-core queues for LoRa communication
    // queue_init(&receive_queue, LORA_SIZE, 5);
    // queue_init(&transmit_queue, LORA_SIZE, 5);
    // // Start core 1 - Do this before any interrupt configuration
    // multicore_launch_core1(comm_run); 

    // configure PWM for motor controller
    status = configure_PWM();
    if (status)
    {
        printf("$ERR Failed to configure PWM.\n");
        return EXIT_FAILURE;
    }

    // configure encoders
    // configure GPIO for encoders
    gpio_set_function(ENC_1_PIN_A, GPIO_FUNC_SIO);
    gpio_set_function(ENC_1_PIN_B, GPIO_FUNC_SIO);
    gpio_set_dir(ENC_1_PIN_A, GPIO_IN);
    gpio_set_dir(ENC_1_PIN_B, GPIO_IN);

    gpio_set_function(ENC_2_PIN_A, GPIO_FUNC_SIO);
    gpio_set_function(ENC_2_PIN_B, GPIO_FUNC_SIO);
    gpio_set_dir(ENC_2_PIN_A, GPIO_IN);
    gpio_set_dir(ENC_2_PIN_B, GPIO_IN);

    // ENC_STATE variables will be updated via timer
    ENC_STATE enc1 = {.channelA = ENC_1_PIN_A,
                      .channelB = ENC_1_PIN_B,
                      .previousTick = 0,
                      .tickCount = 0};
    ENC_STATE enc2 = {.channelA = ENC_2_PIN_A,
                      .channelB = ENC_2_PIN_B,
                      .previousTick = 0,
                      .tickCount = 0};
    void *encoders[] = {&enc1, &enc2};    // stores encoders in single place for timer callback

    // // configure a timer to update encoders at specified freq (Hz)
    int hz = 1000;
    repeating_timer_t timer;
    // negative timeout means exact delay (rather than delay between callbacks)
    if (!add_repeating_timer_us(-1000000 / hz, enc_timer_callback, &encoders, &timer)) {
        printf("Failed to add timer\n");
        return EXIT_FAILURE;
    }

    hz = 20;    // loop rate
    // spin
    while (1)
    {
        // provide encoder updates (tickCounts should be updated by timer)
        printf("$ENC 1 %d 2 %d \n", enc1.tickCount, enc2.tickCount);

        // check if LoRa data is available to process
        // if (queue_try_remove(&receive_queue, received_data)) 
        // {
        //     // everything from CORE 1 is a $CMD
        //     // printf("CORE 0 RECEIVED DATA: %s\n", received_data); 
        //     char cmd[LORA_SIZE] = "$CMD ";
        //     strcat(cmd, received_data);
        //     handle_input(cmd);
        // }
        // if (!queue_try_add(&transmit_queue, sent_data)) 
        // {
        //     printf("$ERR Failed to add data to transmit queue: %s\n", sent_data); 
        // }

        // attempt to read char from stdin
        // no timeout makes it non-blocking
        ch = getchar_timeout_us(0);
        while (ch != ENDSTDIN)
        {
            in_string[idx++] = ch;

            // if the string ends or we run out of space, we're done with this string
            if (ch == CR || ch == NL || idx == (sizeof(in_string)-1))
            {
                in_string[idx-1] = 0; // terminate the string
                idx = 0;    // reset index
                
                status = handle_input(in_string);
                if (status)
                {
                    printf("$ERR Failed to process string: %s\n", in_string);
                }
                break;
            }
            ch = getchar_timeout_us(0);
        }

        sleep_ms(1000 / hz);    // set loop rate; best effort
    }

    printf("ENDING PICO EXECUTION\n");
    return EXIT_SUCCESS;

}
