/**
 * @file onset_adc.h
 * @brief ONSET_ADC module handles the sampling of the piezo sensors and the onset detection.
 * Basically, the module samples the analog input of Kick and Snare using the continuous adc mode.
 * Given that the ADC on the ESP32 is known to be less than ideal, it performs an oversampling and an averaging.
 * After that a very simple filter is applied to detect the amplitude envelope.
 * The module, than, calculates the slope of the envelope and detects the onset.
 * Whenever an onset is detected, its absolute position in time is stored in the onsets array.
 * 
 * The onset_adc module has a queue that is used to ask the main task to start/stop logging onsets.
 * 
 * The module can be asked (by Hid) to display gain. In this case the variable display_gain is set to true
 * and the two leds will display the gain clipping (sample value > GAIN_CLIP_VALUE) and not the onset detected.
 * The two variable (display_gain and allow_onset) should be not true together.
 */

#ifndef BC_ONSET_ADC_H
#define BC_ONSET_ADC_H
#include "main_defs.h"

/**
 * @{ \name GPIO pins for Kick and Snare leds
 */
#if CONFIG_IDF_TARGET_ESP32
#define KICK_LED_PIN GPIO_NUM_14
#define SNARE_LED_PIN GPIO_NUM_12
#else
#define KICK_LED_PIN GPIO_NUM_8
#define SNARE_LED_PIN GPIO_NUM_15
#endif
#define KICK_LED_PIN_BLINK_DURATION 15
#define SNARE_LED_PIN_BLINK_DURATION 15
/**
 * @}
 */

/**
 * @brief Queue to send msgs to the onset_adc module.
 *
 * Queue to send msgs to the onset_adc module. The allowed msg value are defined in onset_adc_queue_msg enum.
 */
extern QueueHandle_t onset_adc_task_queue;

/**
 * @brief Allowed messages for the onset_adc queue.
 *
 * Allowed messages for the onset_adc queue.
 */
typedef enum
{
    ONSET_ADC_ALLOW_ONSET, /**< The module starts logging onsets */
    ONSET_ADC_DISALLOW_ONSET, /**< The module stops logging onsets */
    ONSET_ADC_DISALLOW_ONSET_AND_START_SYNC, /**< The module starts logging onsets and notifies sync to start evaluation */
    ONSET_ADC_START_DISPLAY_GAIN, /**< The leds start indicating the gain clipping (adc sample > 4096) */
    ONSET_ADC_STOP_DISPLAY_GAIN, /**< The leds stop indicating the gain clipping (adc sample > 4096) */
} onset_adc_queue_msg;

/**
 * @brief Struct of the onset log entry.
 *
 * Struct of the onset log entry.
 */
typedef struct
{
    uint64_t time; /**< Absolute time of the onset */
    uint8_t type; /**< Type of onset: 0 for kick and 1 for snare */
} onset_entry;

/**
 * @brief Init function of the onset_adc module
 *
 * Init function of the onset_adc module
 */
void onset_adc_init();

void turn_off_adc();

void turn_on_adc();

#endif