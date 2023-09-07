/**
 * @file tempo.h
 * @brief Tempo module evaluates the need of a change in the bpm of the midi clock.
 * For every onset, the sync module calculates the IOI with the preceding onsets of the two bars
 * and calculates the accuracy. If the highest accuracy found is higher than the threshold
 * the module sends a message to the clock module asking to set delta_tau_tempo value.
 * The module starts its job when notified by the Sync module.
 */

#ifndef BC_TEMPO_H
#define BC_TEMPO_H

/**
 * @brief Handle of the tempo_task.
 */
extern TaskHandle_t tempo_task_handle;

/**
 * @brief Notify codes for tempo_task.
 */
typedef enum
{
    TEMPO_START_EVALUATION_NOTIFY,
    TEMPO_RESET_PARAMETERS,
} tempo_task_notify_code;

/**
 * @brief Init function for the tempo module
 * The function will set parameters and starts tempo_task
 */
void tempo_init();

#endif