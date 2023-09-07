/**
 * @file sync.h
 * @brief SYNC module evaluates the need of a sincronization of the midi clock.
 * For every onset, the sync module calculates the distance from the expected and evaluates the
 * need for a sincronization. If this is the case, the module sends a message to the clock module
 * asking to set delta_tau_sync value.
 * The module starts its job when notified by the onset_adc module.
 */

#ifndef BC_SYNC_H
#define BC_SYNC_H

#include "main_defs.h"

/**
 * @brief Headroom value suggested for the B-Keeper algorithm
 */
#define HEADROOM_VALUE_SYNC 0.1

/**
 * @brief Notify codes for sync_task.
 */
typedef enum
{
    SYNC_START_EVALUATION_NOTIFY,
    SYNC_RESET_PARAMETERS,
} sync_task_notify_code;

/**
 * @brief Handle of the sync_task.
 */
extern TaskHandle_t sync_task_handle;

/**
 * @brief Init function for the sync module
 * The function sets parameters and starts sync_task
 */
void sync_init();

#endif