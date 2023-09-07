/**
 * @file clock.h
 * @brief Clock module keeps up the time and sends MIDI_CLOCK messages.
 * The clock module keeps up with the time of the song and sends MIDI_CLOCK messages via UART.
 * It can be controlled by messaging its queue using the data structures below.
 * The clock_task has a while loop that keeps repeating with this scheme:
 * - Check message in queue
 * - If there are messages, do what the messages asks to
 * - If the sequence is on than send MIDI CLOCK message and do other actions depending on index
 * - Delay itself until it's time to send the next MIDI CLOCK
 */
#ifndef BC_CLOCK_H
#define BC_CLOCK_H

/**
 * @{ \name GPIO pins for Led 1-4
 */
#if CONFIG_IDF_TARGET_ESP32
#define FIRST_LED_PIN GPIO_NUM_17
#define SECOND_LED_PIN GPIO_NUM_16
#define THIRD_LED_PIN GPIO_NUM_4
#define FOURTH_LED_PIN GPIO_NUM_2
#else
#define FIRST_LED_PIN GPIO_NUM_37
#define SECOND_LED_PIN GPIO_NUM_36
#define THIRD_LED_PIN GPIO_NUM_35
#define FOURTH_LED_PIN GPIO_NUM_45
#endif
/**
 * @}
 */

/**
 * @{ \name GPIO pins for UART
 */
#if CONFIG_IDF_TARGET_ESP32
#define UART_PIN_1 GPIO_NUM_26
#define UART_PIN_2 GPIO_NUM_27
#else
#define UART_PIN_1 GPIO_NUM_17
#define UART_PIN_2 GPIO_NUM_18
#endif
/**
 * @}
 */

/**
 * @brief Handle for the clock_task queue.
 * Defined inside clock.c
 */
extern QueueHandle_t clock_task_queue;
/**
 * @brief Handle for the clock_task.
 * Defined inside clock.c
 */
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
 * This function will initialize the GPIO pins, the UART and creates the clock_task
 */
void clock_init();

#endif