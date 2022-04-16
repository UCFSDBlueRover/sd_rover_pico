/**
 * @file motors.c
 * @author Wesley Fletcher (wkfletcher@knights.ucf.com)
 * @brief Functions for controlling the motors via PWM and reading encoders
 * @version 0.1
 * @date 2022-03-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */

// general includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// hardware includes
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#include "motors.h"

/**
 * @brief Updates an ENC_STATE struct based on current state of encoder GPIOs;
 *        Intended to be called via timer.
 * 
 * @param encoder pointer to a ENC_STATE struct to be updated
 */
void encoder_update(ENC_STATE *encoder)
{
    int tickA, tickB;

    // read current tick states
    tickA = gpio_get(encoder->channelA);
    
    // if GPIO state has changed, update tickCount
    if (tickA != encoder->previousTick)
    {
        tickB = gpio_get(encoder->channelB);

        // forward movement
        if (tickA != tickB)
        {
            encoder->tickCount += tickA;
        }
        // backwards movement
        else
        {
            encoder->tickCount -= tickA;
        }
        encoder->previousTick = tickA;
    }
}

/**
 * @brief Timer callback that updates encoder information using encoder_update()
 * 
 * @param rt the repeating_timer_t struct that contains our encoder data (in user_data)
 * @return true continue with this timer
 * @return false stop this timer
 */
bool enc_timer_callback(repeating_timer_t *rt)
{
    // get encoders from user_data of rpeating_timer_t struct
    void **encoders = (void **) rt->user_data;
    ENC_STATE *enc1 = (ENC_STATE *) encoders[0];
    ENC_STATE *enc2 = (ENC_STATE *) encoders[1];

    // udpate each encoder
    encoder_update(enc1);
    encoder_update(enc2);

    return true;
}

/**
 * @brief Configures PWM using defs from motors.h
 * 
 * @return int 
 */
int configure_PWM()
{
    // configure PWM pins as PWM
    gpio_set_function(PWM_1_PIN, GPIO_FUNC_PWM);
    gpio_set_function(PWM_2_PIN, GPIO_FUNC_PWM);

    // configure DIR pins as GPIO
    gpio_set_function(DIR_1_PIN, GPIO_FUNC_SIO);
    gpio_set_function(DIR_2_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(DIR_1_PIN, GPIO_OUT);
    gpio_set_dir(DIR_2_PIN, GPIO_OUT);

    // set slice
    uint slice1 = pwm_gpio_to_slice_num(PWM_1_PIN);
    uint slice2 = pwm_gpio_to_slice_num(PWM_2_PIN);
    if (slice1 != slice2)
    {
        printf("$ERR PWM slice mismatch, slice1: %d, slice2: %d", slice1, slice2);
        return EXIT_FAILURE;
    }

    // set "wrap": number of cycles for each pulse
    pwm_set_wrap(slice1, PWM_WRAP);

    // start PWMs at 0 = STOP
    pwm_set_chan_level(slice1, PWM_CHAN_A, 0);     // right
    pwm_set_chan_level(slice1, PWM_CHAN_B, 0);     // left

    // set the PWM running
    pwm_set_enabled(slice1, true);

    return EXIT_SUCCESS;
}

/**
 * @brief 
 * 
 * @param left_dir true for forward, false for reverse
 * @param left_speed 0-100, "full-stop"-> "max-go"
 * @param right_dir true for forward, false for reverse
 * @param right_speed 0-100, "full-stop"-> "max-go"
 */
void set_PWM(bool left_dir, int left_speed, bool right_dir, int right_speed)
{
    // set DIR pins
    gpio_put(DIR_1_PIN, left_dir);
    gpio_put(DIR_2_PIN, right_dir);

    // set PWM pins, normalizing for the wrap cycle/100
    pwm_set_gpio_level(PWM_1_PIN, (int16_t)left_speed * (PWM_WRAP / 100));
    pwm_set_gpio_level(PWM_2_PIN, (int16_t)right_speed * (PWM_WRAP / 100));
}
