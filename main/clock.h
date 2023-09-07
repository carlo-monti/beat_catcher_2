/**
 * @file clock.h
 * @brief Clock module keeps up the time and sends MIDI_CLOCK messages.
 * The clock module keeps up with the time of the song and sends MIDI_CLOCK messages via UART.
 * It can be controlled by messaging its queue using the data structures below.
 */
#ifndef BC_CLOCK_H
#define BC_CLOCK_H

extern QueueHandle_t clock_task_queue;
extern TaskHandle_t clock_task_handle;

/**
 * @brief Type of message sent to the queue.
 * Every message that the queue receives has to be one of the types specified here
 */
typedef enum
{
    CLOCK_QUEUE_SET_DELTA_TAU_SYNC,/**< Asks the clock to update the delta tau value for the sync */
    CLOCK_QUEUE_SET_DELTA_TAU_TEMPO,/**< Asks the clock to update the delta tau value for the tempo */
    CLOCK_QUEUE_STOP,/**< Asks the clock to stop */
    CLOCK_QUEUE_START,/**< Asks the clock to start */
} clock_task_queue_entry_type;

/**
 * @brief Message for the queue.
 * This is the main structure of the message that has to be sent to the queue
 */
typedef struct
{
    clock_task_queue_entry_type type; /**< Type of message (choosen from the clock_task_queue_entry_type enum) */
    long long value; /**< Value sent (delta tau sync or tempo) */
} clock_task_queue_entry;

/**
 * @brief Init function to be called from the main.
 */
void clock_init();

#endif