
#include "mode_switch.h"
#include "hid.h"
#include "clock.h"
#include "driver/gpio.h"
#include "tap.h"

/*!
Beat catcher 2.0 - Mode Switch Module
This module handles the switching between the main modes of the system:
-MODE_PLAY
-MODE_TAP
-MODE_SETTINGS
*/

extern QueueHandle_t clock_task_queue;
extern QueueHandle_t tap_task_queue;
extern TaskHandle_t tap_task_handle;
extern main_mode mode;

TaskHandle_t mode_switch_task_handle = NULL;

static void IRAM_ATTR mode_switch_isr_handler(void *args)
{
    static uint64_t previous_switch_debounce = 0;
    uint64_t current_time_us = esp_timer_get_time();
    if (current_time_us > previous_switch_debounce + MODE_SWITCH_DEBOUNCE_TIME_US)
    {
        switch (mode)
        {
        case MODE_TAP: // -> switch to SETTINGS
        case MODE_SLEEP: // -> switch to SETTINGS
        case MODE_PLAY: // -> switch to SETTINGS
            // xxx add display change
            xTaskNotifyFromISR(mode_switch_task_handle, MODE_SWITCH_TO_SETTINGS, eSetValueWithOverwrite,0);
            break;
        case MODE_SETTINGS: // -> switch to TAP
            // xxx add display change
            xTaskNotifyFromISR(mode_switch_task_handle, MODE_SWITCH_TO_TAP, eSetValueWithOverwrite,0);
            break;
        default:
            ESP_LOGI("ERROR", "INVALID MODE SET"); // xxx togliere
            break;
        }
        previous_switch_debounce = current_time_us;
    }
}

static void go_to_sleep(){
    ESP_LOGI("tap_task", "GOING TO SLEEP");
    ESP_LOGI("tap_task", "current mode: %d",mode);
    //esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
    gpio_wakeup_enable(MENU_SWITCH_PIN,GPIO_INTR_LOW_LEVEL);
    gpio_intr_disable(TAP_TEMPO_PIN);
    gpio_intr_disable(MENU_SWITCH_PIN);
    gpio_wakeup_enable(TAP_TEMPO_PIN,GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    gpio_intr_enable(TAP_TEMPO_PIN);
    gpio_intr_enable(MENU_SWITCH_PIN);
    ESP_LOGI("tap_task", "WAKING FROM SLEEP");
    ESP_LOGI("tap_task", "WAKING current mode: %d",mode);
};

static void mode_switch_task(void *arg)
{
    while (1)
    {
        uint32_t notify_code = 0;
        xTaskNotifyWait(0, 0, &notify_code, portMAX_DELAY);
        clock_task_queue_entry clock_tx_buffer;
        int msg_to_hid;
        switch (notify_code)
        {
            case MODE_SWITCH_TO_TAP:
                mode = MODE_TAP;
                xQueueReset(clock_task_queue);
                clock_tx_buffer.type = CLOCK_QUEUE_STOP;
                clock_tx_buffer.value = 0,
                xQueueSend(clock_task_queue, &clock_tx_buffer, (TickType_t)0);
                msg_to_hid = HID_TAP_MODE_SELECT;
                xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
                //xQueueReset(tap_task_queue);
                uint64_t tap_task_msg = TAP_TASK_QUEUE_RESET_COUNTER;
                xQueueSend(tap_task_queue, &tap_task_msg,(TickType_t)0);
                vTaskResume(tap_task_handle);
                break;
            case MODE_SWITCH_TO_PLAY:
                mode = MODE_PLAY;
                break;
            case MODE_SWITCH_TO_SETTINGS:
                ESP_LOGI("MODE_SWITCH","Received ask to switch");
                mode = MODE_SETTINGS;
                clock_tx_buffer.type = CLOCK_QUEUE_STOP;
                clock_tx_buffer.value = 0;
                xQueueSend(clock_task_queue, &clock_tx_buffer, (TickType_t)0);
                msg_to_hid = HID_SETTINGS_MODE_SELECT;
                xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
                break;
            case MODE_SWITCH_TO_SLEEP:
                ESP_LOGI("MODE_SWITCH","Received ask to sleep");
                mode = MODE_SLEEP;
                clock_tx_buffer.type = CLOCK_QUEUE_STOP;//xxx inutile
                clock_tx_buffer.value = 0;
                xQueueReset(clock_task_queue);
                xQueueSend(clock_task_queue, &clock_tx_buffer, (TickType_t)0);
                msg_to_hid = HID_ENTER_SLEEP_MODE;
                xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
                vTaskDelay(pdMS_TO_TICKS(500));
                go_to_sleep();
                break;
            default:
                ESP_LOGE("MODE_SWITCH","Invalid notification code");
                break;
        }
        vTaskDelay(1);
    }
}

void mode_switch_init(){
    gpio_reset_pin(MENU_SWITCH_PIN);
    gpio_set_direction(MENU_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(MENU_SWITCH_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(MENU_SWITCH_PIN, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(MENU_SWITCH_PIN, mode_switch_isr_handler, NULL);
    xTaskCreate(mode_switch_task, "Mode_Switch_Task", MODE_SWITCH_STACK_SIZE, NULL, MODE_SWITCH_TASK_PRIORITY, &mode_switch_task_handle);
}