/**
 * @file mode_swi.h
 * @brief File containing example of doxygen usage for quick reference.
 *
 * Here typically goes a more extensive explanation of what the header
 * defines. Doxygens tags are words preceeded by either a backslash @\
 * or by an at symbol @@.
 */

#ifndef BC_TAP_H
#define BC_TAP_H

extern QueueHandle_t tap_task_queue;
extern TaskHandle_t tap_task_handle;

void tap_init();

#endif