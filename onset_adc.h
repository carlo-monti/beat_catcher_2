/**
 * @file mode_swi.h
 * @brief File containing example of doxygen usage for quick reference.
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 */

#ifndef BC_ONSET_ADC_H
#define BC_ONSET_ADC_H
#include "main_defs.h"

extern QueueHandle_t onset_adc_task_queue;

typedef enum
{
    ONSET_ADC_ALLOW_ONSET,
    ONSET_ADC_DISALLOW_ONSET,
    ONSET_ADC_DISALLOW_ONSET_AND_START_SYNC,
} onset_adc_queue_msg;

typedef struct
{
    uint64_t time;
    uint8_t type; // kick or snare
} onset_entry;

void onset_adc_init();

#endif