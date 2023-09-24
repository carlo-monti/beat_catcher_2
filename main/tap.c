#include "main_defs.h"
#include "tap.h"
#include "clock.h"
#include "sync.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "tempo.h"
#include "hid.h"
#include "mode_switch.h"

/**
 * @brief Timeout for going to sleep when waiting for hit
*/
#define TAP_HIT_WAIT_TIME_US 4000000

extern QueueHandle_t clock_task_queue;
extern TaskHandle_t clock_task_handle;
extern TaskHandle_t sync_task_handle;
extern TaskHandle_t tempo_task_handle;
extern main_runtime_vrbs bc;
extern volatile main_mode mode;
extern SemaphoreHandle_t bc_mutex_handle; // mutex for the access to bc struct

QueueHandle_t tap_task_queue = NULL;
TaskHandle_t tap_task_handle = NULL;

clock_task_queue_entry clock_tx_buffer;

/**
 * @brief ISR handler for the tap button
 * It gets called when the user pushes the tap button
*/
static void IRAM_ATTR tap_tempo_isr_handler(void *args)
{
    /*
    Debounce
    */
    static uint64_t previous_switch_debounce = 0;
    uint64_t current_time_us = esp_timer_get_time();
    if (current_time_us > previous_switch_debounce + TAP_DEBOUNCE_TIME_US)
    {
        uint64_t tap_task_msg;
        uint64_t current_onset;
        switch(mode){
            case MODE_TAP:
                /*
                If this is not the first hit, notify the tap_task with the hit position (in abs time)
                */
                current_onset = current_time_us;
                xQueueSendFromISR(tap_task_queue, &current_onset, (TickType_t)0);
                break;
            case MODE_SLEEP:
            case MODE_PLAY:
                /*
                This is the first tap.
                Reset the queue of tap_task, ask to reset counter and notify the hit position
                */
                xTaskNotify(mode_switch_task_handle, MODE_SWITCH_TO_TAP, eSetValueWithOverwrite);
                xQueueReset(tap_task_queue);
                tap_task_msg = TAP_TASK_QUEUE_RESET_COUNTER;
                xQueueSendFromISR(tap_task_queue, &tap_task_msg,(TickType_t)0);
                current_onset = current_time_us;
                xQueueSendFromISR(tap_task_queue, &current_onset, (TickType_t)0);
                break;
            default:
                return;
                break;
        }
        previous_switch_debounce = current_time_us;
    }
}

/**
 * @brief Main task of the Tap module
*/
static void tap_task(void *arg)
{
    /*
    Initialize parameters
    */
    uint64_t tap_tempo_onsets[4] = {0};
    uint64_t tap_task_queue_result = 0;
    uint8_t counter = 0;
    uint64_t time_of_last_hit = 0;
    /*
    Create queue
    */
    tap_task_queue = xQueueCreate(2, sizeof(tap_tempo_onsets[0]));

    while (1)
    {
        /*
        Check if there is a message on the queue
        */
        if (xQueueReceive(tap_task_queue, &tap_task_queue_result, 0))
        {
            if (tap_task_queue_result == TAP_TASK_QUEUE_RESET_COUNTER)
            {
                /*
                If the message ask to reset counter:
                */
                counter = 0;
            }
            else
            {
                /*
                If there is a hit, log it into the array and increase the counter
                */
                tap_tempo_onsets[counter] = tap_task_queue_result;
                counter++;
                /*
                Ask hid module to display the hit received
                */
                int msg_to_hid = HID_TAP_0 + counter;
                xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
            }
            /*
            Annotate the time to check for timeout
            */
            time_of_last_hit = esp_timer_get_time();
        }
        if (counter == 4)
        {
            /*
            If it is the last hit:
            Notify the mode_switch_task to go in PLAY mode
            */
            xTaskNotify(mode_switch_task_handle, MODE_SWITCH_TO_PLAY, eSetValueWithOverwrite);
            /*
            Calculate bpm
            */
            uint64_t tap_period = 0;
            for (int i = 0; i < 3; i++)
            {
                tap_period += tap_tempo_onsets[i + 1] - tap_tempo_onsets[i];
            }
            tap_period = tap_period / 3; // 4th note
            xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
            bc.tau = tap_period / 2;     // 8th note
            bc.expected_beat = tap_tempo_onsets[3] + tap_period;
            xSemaphoreGive(bc_mutex_handle);
            /*
            Notify the new bpm to sync and tempo modules
            */
            xTaskNotify(sync_task_handle, SYNC_RESET_PARAMETERS, eSetValueWithOverwrite);
            xTaskNotify(tempo_task_handle, TEMPO_RESET_PARAMETERS, eSetValueWithOverwrite);
            /*
            Reset the clock queue, ask to start sequence and resume its task
            */
            xQueueReset(clock_task_queue);
            clock_tx_buffer.type = CLOCK_QUEUE_START;
            clock_tx_buffer.value = bc.expected_beat,
            xQueueSend(clock_task_queue, &clock_tx_buffer, (TickType_t)0);
            vTaskResume(clock_task_handle);
            counter = 0;
            /*
            Suspend until next tap mode is selected
            */
            vTaskSuspend(NULL);
        }
        else
        {
            if (esp_timer_get_time() > time_of_last_hit + TAP_HIT_WAIT_TIME_US)
            {
                /*
                If there is no hit until timeout time:
                */
                if(mode == MODE_TAP){
                    counter = 0;
                    /*
                    Ask mode switch module to go to sleep
                    */
                    xTaskNotify(mode_switch_task_handle, MODE_SWITCH_TO_SLEEP, eSetValueWithOverwrite);
                    /*
                    Suspend until next tap mode is selected
                    */
                    vTaskSuspend(NULL);
                }
            }
            vTaskDelay(1);
        }
    }
}

/**
 * @brief Initialization of the Tap module
 * Sets GPIO pins, attach interrupt to tap button and creates tap_task
*/
void tap_init()
{
    /*
    Sets GPIO PIN
    */
    gpio_reset_pin(TAP_TEMPO_PIN);
    gpio_set_direction(TAP_TEMPO_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TAP_TEMPO_PIN, GPIO_PULLDOWN_ONLY);
    vTaskDelay(1);
    /*
    Attach interrupt
    */ 
    gpio_set_intr_type(TAP_TEMPO_PIN, GPIO_INTR_POSEDGE);
    gpio_isr_handler_add(TAP_TEMPO_PIN, tap_tempo_isr_handler, NULL);
    /*
    Create tap_task 
    */
    xTaskCreate(tap_task, "Tap_Task", TAP_TASK_STACK_SIZE, NULL, TAP_TASK_PRIORITY, &tap_task_handle);
};