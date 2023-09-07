/**
 * @file tap.h
 * @brief Tap module let the user insert the initial bpm by pressing on the tap button on time.
 * Every time the user presses the button, an interrupt routine send a message to the queue 
 * of the main task with information about the timing and the task keeps track of how many hits has been received.
 * If there are 4 hits, the task calculates the bpm average and starts the clock module.
 * If there are no hits before the timeout, the tap ask to switch to sleep mode.
 */

#ifndef BC_TAP_H
#define BC_TAP_H

#define TAP_TASK_QUEUE_RESET_COUNTER 0

/**
 * @brief Queue to send msgs to the tap module.
 *
 * Queue to send msgs to the tap module. If the message is TAP_TASK_QUEUE_RESET_COUNTER, the task
 * resets its internal counter. Otherwise it will interpret it as an absolute value of time for an hit.
 */
extern QueueHandle_t tap_task_queue;

/**
 * @brief Handle of tap_task
 *
 * Handle of tap_task used to resume the task when needed
 */
extern TaskHandle_t tap_task_handle;

/**
 * @brief Init function to be called from the main.
 */
void tap_init();

#endif