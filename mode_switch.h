/**
 * @file mode_swi.h
 * @brief File containing example of doxygen usage for quick reference.
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 */

#ifndef BC_MODE_SWITCH_H
#define BC_MODE_SWITCH_H

#include "main_defs.h"

/**
 * @brief Use brief, otherwise the index won't have a brief explanation.
 *
 * Detailed explanation.
 */
typedef enum
{
    MODE_SWITCH_TO_PLAY,
    MODE_SWITCH_TO_SETTINGS,
    MODE_SWITCH_TO_TAP,
} mode_switch_task_notify_value;

/**
 * @brief Use brief, otherwise the index won't have a brief explanation.
 *
 * Detailed explanation.
 */
extern TaskHandle_t mode_switch_task_handle;

/**
 * @brief Use brief, otherwise the index won't have a brief explanation.
 *
 * Detailed explanation.
 */
void mode_switch_init();

#endif