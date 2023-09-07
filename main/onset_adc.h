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
 */

#ifndef BC_ONSET_ADC_H
#define BC_ONSET_ADC_H
#include "main_defs.h"

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

#endif