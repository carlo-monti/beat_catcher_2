/**
 * @file mode_swi.h
 * @brief File containing example of doxygen usage for quick reference.
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 */

#ifndef BC_SYNC_H
#define BC_SYNC_H

#define HEADROOM_VALUE 0.1

#include "main_defs.h"

typedef enum
{
    SYNC_START_EVALUATION_NOTIFY,
    SYNC_RESET_PARAMETERS,
} sync_task_notify_code;

extern TaskHandle_t sync_task_handle;

void sync_init();

#endif