/**
 * @file main_defs.h
 * @brief This file contains definitions valid for the whole code (task stack size, task priority, macros and typedefs,...)
 */

#ifndef BC_MAIN_DEFS_H
#define BC_MAIN_DEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "esp_timer.h"

/**
 * @{ \name Task priorities
 */
#define TAP_TASK_PRIORITY 10
#define CLOCK_TASK_PRIORITY 11
#define TEMPO_TASK_PRIORITY 10
#define SYNC_TASK_PRIORITY 10
#define ONSET_ADC_TASK_PRIORITY 10
#define HID_TASK_PRIORITY 10
#define MODE_SWITCH_TASK_PRIORITY 10
/**
 * @}
 */

/**
 * @{ \name Stack size for all the tasks
 */
#define TAP_TASK_STACK_SIZE 4096
#define CLOCK_TASK_STACK_SIZE 4096
#define TEMPO_TASK_STACK_SIZE 4096
#define SYNC_TASK_STACK_SIZE 4096
#define ONSET_ADC_TASK_STACK_SIZE 4096
#define HID_TASK_STACK_SIZE 4096
#define MODE_SWITCH_STACK_SIZE 4096
/**
 * @}
 */

/**
 * @brief Number of 8th notes contained in two bars
 */
#define TWO_BAR_LENGTH_IN_8TH 16

/**
 * @brief Max number of onsets that can be stored at the same time
 */
#define ONSET_BUFFER_SIZE 300

/**
 * @brief Macros that gives the current time in ms
 */
#define GET_CURRENT_TIME_MS() ((esp_timer_get_time() + 500) / 1000)

/**
 * @brief Macros that converts us to ms
 */
#define US_TO_MS(time_us) ((time_us + 500) / 1000)      

/**
 * @brief Enum of the four main modes of the system
 */
typedef enum
{
    MODE_PLAY,
    MODE_SETTINGS,
    MODE_TAP,
    MODE_SLEEP,
} main_mode;

/**
 * @brief Struct of the main runtime global variable.
 * It contains all the parameters that can change over time
 */
typedef struct
{
    uint64_t tau; // 8th time in ms (bpm120)
    uint8_t bar_position;
    uint8_t layer;
    uint64_t expected_beat;
    bool there_is_an_onset;
    uint32_t most_recent_onset_index;
    int last_relevant_onset_index_for_sync;
    uint32_t last_relevant_onset_index_for_tempo;
} main_runtime_vrbs;

#endif