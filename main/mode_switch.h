/**
 * @file mode_switch.h
 * @brief MODE_SWITCH module handles the changing of the main mode of Beat Catcher.
 * There are four main modes of operation:
 * MODE_TAP
 * MODE_PLAY
 * MODE_SETTINGS
 * MODE_SLEEP
 * The mode module has a main task that is exposed and can be notified using a value
 * from mode_switch_task_notify_value. In this way you can ask the task to change the mode.
 */

#ifndef BC_MODE_SWITCH_H
#define BC_MODE_SWITCH_H

#include "main_defs.h"

/**
 * @brief Notify values to ask mode change.
 *
 * When notify task, use one of this enum entry to ask the task to switch to that particular mode.
 */
typedef enum
{
    MODE_SWITCH_TO_PLAY,
    MODE_SWITCH_TO_SETTINGS,
    MODE_SWITCH_TO_TAP,
    MODE_SWITCH_TO_SLEEP,
} mode_switch_task_notify_value;

/**
 * @brief Handle of mode_switch_task to receive notifications
 *
 * Handle of mode_switch_task to receive notifications
 */
extern TaskHandle_t mode_switch_task_handle;

/**
 * @brief Init function for the mode switch module
 */
void mode_switch_init();

#endif