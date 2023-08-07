/**
 * @file mode_swi.h
 * @brief File containing example of doxygen usage for quick reference.
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
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

/* TASKS PRIORITIES */
#define TAP_TASK_PRIORITY 10
#define CLOCK_TASK_PRIORITY 12
#define TEMPO_TASK_PRIORITY 10
#define SYNC_TASK_PRIORITY 10
#define ONSET_ADC_TASK_PRIORITY 10
#define HID_TASK_PRIORITY 10
#define ONSET_GARBAGE_TASK_PRIORITY 10

/* LEDS */
#if CONFIG_IDF_TARGET_ESP32
#define KICK_LED_PIN GPIO_NUM_14
#define SNARE_LED_PIN GPIO_NUM_33
#define FIRST_LED_PIN GPIO_NUM_17
#define SECOND_LED_PIN GPIO_NUM_16
#define THIRD_LED_PIN GPIO_NUM_4
#define FOURTH_LED_PIN GPIO_NUM_2
#else
#define KICK_LED_PIN GPIO_NUM_8
#define SNARE_LED_PIN GPIO_NUM_15
#define FIRST_LED_PIN GPIO_NUM_37
#define SECOND_LED_PIN GPIO_NUM_36
#define THIRD_LED_PIN GPIO_NUM_35
#define FOURTH_LED_PIN GPIO_NUM_45
#endif
#define KICK_LED_PIN_BLINK_DURATION 30 /**< Some documentation for the member BoxStruct#a. */
#define SNARE_LED_PIN_BLINK_DURATION 20 /**< Some documentation for the member BoxStruct#a. */
#define FIRST_LED_PIN_BLINK_DURATION 20 /**< Some documentation for the member BoxStruct#a. */
#define SECOND_LED_PIN_BLINK_DURATION 20 /**< Some documentation for the member BoxStruct#a. */
#define THIRD_LED_PIN_BLINK_DURATION 20 /**< Some documentation for the member BoxStruct#a. */
#define FOURTH_LED_PIN_BLINK_DURATION 20 /**< Some documentation for the member BoxStruct#a. */

#define TWO_BAR_LENGTH_IN_8TH 16

#define MODE_SWITCH_DEBOUNCE_TIME_US 600000
#define TAP_DEBOUNCE_TIME_US 200000 // almost 300 bpm

/* UART */
#if CONFIG_IDF_TARGET_ESP32
#define UART_PIN_1 GPIO_NUM_26
#define UART_PIN_2 GPIO_NUM_27
#else
#define UART_PIN_1 GPIO_NUM_17
#define UART_PIN_2 GPIO_NUM_18
#endif

#if CONFIG_IDF_TARGET_ESP32
#define TAP_TEMPO_PIN GPIO_NUM_25
#define MENU_SWITCH_PIN GPIO_NUM_12
#else
#define TAP_TEMPO_PIN GPIO_NUM_16
#define MENU_SWITCH_PIN GPIO_NUM_3
#endif

#define ONSET_GARBAGE_NOTIFY 0

#define ONSET_BUFFER_SIZE 300
#define TWO_BAR_LENGTH_IN_8TH 16
#define TAP_TASK_QUEUE_RESET_COUNTER 0

#define GET_CURRENT_TIME_MS() ((esp_timer_get_time() + 500) / 1000)
#define US_TO_MS(time_us) ((time_us + 500) / 1000)      

typedef enum
{
    MODE_PLAY,
    MODE_SETTINGS,
    MODE_TAP,
} main_mode;

typedef struct
{
    uint64_t tau; // 8th time in ms (bpm120)
    uint8_t bar_position;
    uint8_t layer;
    uint64_t expected_beat;
    bool there_is_an_onset;
    bool there_is_an_onset_out_of_phase;
    uint32_t most_recent_onset_index;
    int last_relevant_onset_index_for_sync;
    uint32_t last_relevant_onset_index_for_tempo;
} main_runtime_vrbs;

#endif