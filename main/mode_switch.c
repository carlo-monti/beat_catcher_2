#include "mode_switch.h"
#include "hid.h"
#include "clock.h"
#include "driver/gpio.h"
#include "tap.h"
#include "onset_adc.h"

extern QueueHandle_t clock_task_queue;
extern QueueHandle_t tap_task_queue;
extern TaskHandle_t tap_task_handle;
extern main_mode mode;

TaskHandle_t mode_switch_task_handle = NULL;

/*
Handler of the mode button ISR
*/
static void IRAM_ATTR mode_switch_isr_handler(void *args)
{
    /*
    Performs debouncing of the button
    */
    static uint64_t previous_switch_debounce = 0;
    uint64_t current_time_us = esp_timer_get_time();
    if (current_time_us > previous_switch_debounce + MODE_SWITCH_DEBOUNCE_TIME_US)
    {
        /*
        Perform action depending on the current mode the system is in
        */
        switch (mode)
        {
        case MODE_TAP:
        case MODE_SLEEP:
        case MODE_PLAY:
            /*
            Notify the main task to switch to SETTINGS mode
            */
            xTaskNotifyFromISR(mode_switch_task_handle, MODE_SWITCH_TO_SETTINGS, eSetValueWithOverwrite,0);
            break;
        case MODE_SETTINGS:
            /*
            Notify the main task to switch to TAP mode
            */
            xTaskNotifyFromISR(mode_switch_task_handle, MODE_SWITCH_TO_TAP, eSetValueWithOverwrite,0);
            break;
        default:
            ESP_LOGE("ERROR", "INVALID MODE SET");
            break;
        }
        previous_switch_debounce = current_time_us;
    }
}

/*
Function to put the system to sleep
*/
static void go_to_sleep(){
    /*
    Disable interrupts on wakeup pins
    */
    gpio_intr_disable(TAP_TEMPO_PIN);
    gpio_intr_disable(MODE_SWITCH_PIN);
    /*
    Set up the wakeup methods
    */
    gpio_wakeup_enable(MODE_SWITCH_PIN,GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(TAP_TEMPO_PIN,GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    /*
    Go to sleep
    */
    esp_light_sleep_start();
    /*
    Wake up from here (Re-enable the interrupts)
    */
    gpio_intr_enable(TAP_TEMPO_PIN);
    gpio_intr_enable(MODE_SWITCH_PIN);
};

/*
Main task for the mode switch module
*/
static void mode_switch_task(void *arg)
{
    while (1)
    {
        /*
        Block and wait for a notify
        */
        uint32_t notify_code = 0;
        xTaskNotifyWait(0, 0, &notify_code, portMAX_DELAY);
        clock_task_queue_entry clock_tx_buffer;
        int msg_to_hid;
        switch (notify_code)
        {
            /*
            Perform action requested by the notify code
            */  
            case MODE_SWITCH_TO_TAP:
                /*
                Ask onset to stop display gain (it can be currently set to on by hid)
                */
                int onset_adc_queue_value = ONSET_ADC_STOP_DISPLAY_GAIN;
                xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, pdMS_TO_TICKS(100));
                /*
                Change main mode
                */
                mode = MODE_TAP;
                /*
                Reset clock queue
                */
                xQueueReset(clock_task_queue);
                /*
                Ask clock to stop
                */
                clock_tx_buffer.type = CLOCK_QUEUE_STOP;
                clock_tx_buffer.value = 0;
                xQueueSend(clock_task_queue, &clock_tx_buffer, (TickType_t)0);
                /*
                Ask hid to switch to tap mode
                */
                msg_to_hid = HID_TAP_MODE_SELECT;
                xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
                /*
                Ask Tap to reset its counter
                */
                uint64_t tap_task_msg = TAP_TASK_QUEUE_RESET_COUNTER;
                xQueueSend(tap_task_queue, &tap_task_msg,(TickType_t)0);
                /*
                Resume Tap task
                */
                vTaskResume(tap_task_handle);
                break;
            case MODE_SWITCH_TO_PLAY:
                /*
                Change main mode
                */
                mode = MODE_PLAY;
                break;
            case MODE_SWITCH_TO_SETTINGS:
                /*
                Change main mode
                */
                mode = MODE_SETTINGS;
                /*
                Ask clock to stop
                */
                clock_tx_buffer.type = CLOCK_QUEUE_STOP;
                clock_tx_buffer.value = 0;
                xQueueSend(clock_task_queue, &clock_tx_buffer, (TickType_t)0);
                /*
                Ask Hid to switch to settings mode
                */
                msg_to_hid = HID_SETTINGS_MODE_SELECT;
                xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
                break;
            case MODE_SWITCH_TO_SLEEP:
                /*
                Ask hid to turn off display
                */
                msg_to_hid = HID_ENTER_SLEEP_MODE;
                xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
                /*
                Wait for the tasks to do their action
                */
                vTaskDelay(pdMS_TO_TICKS(200));
                /*
                Change main mode
                */
                mode = MODE_SLEEP;
                /*
                Go to sleep
                */
                go_to_sleep();
                break;
            default:
                ESP_LOGE("MODE_SWITCH","Invalid notification code");
                break;
        }
        vTaskDelay(1);
    }
}

/*
Init function
*/
void mode_switch_init(){
    /*
    Set up GPIO pins
    */
    gpio_reset_pin(MODE_SWITCH_PIN);
    gpio_set_direction(MODE_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(MODE_SWITCH_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(MODE_SWITCH_PIN, GPIO_INTR_NEGEDGE);
    /*
    Attach interrupt to menu switch button
    */  
    gpio_isr_handler_add(MODE_SWITCH_PIN, mode_switch_isr_handler, NULL);
    /*
    Create mode_switch_task
    */
    xTaskCreate(mode_switch_task, "Mode_Switch_Task", MODE_SWITCH_STACK_SIZE, NULL, MODE_SWITCH_TASK_PRIORITY, &mode_switch_task_handle);
}