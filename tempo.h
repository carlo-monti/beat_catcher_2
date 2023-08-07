/**
 * @file mode_swi.h
 * @brief File containing example of doxygen usage for quick reference.
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 */

#ifndef BC_TEMPO_H
#define BC_TEMPO_H

extern TaskHandle_t tempo_task_handle;

typedef enum
{
    TEMPO_START_EVALUATION_NOTIFY,
    TEMPO_RESET_PARAMETERS,
} tempo_task_notify_code;

void tempo_init();

#endif