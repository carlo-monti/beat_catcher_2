#include "main_defs.h"
#include "tap.h"
#include "clock.h"
#include "sync.h"
#include "driver/gpio.h"
#include "tempo.h"
#include "hid.h"
#include "mode_switch.h"

#define TAP_HIT_WAIT_TIME_US 4000000

extern QueueHandle_t clock_task_queue;
extern TaskHandle_t clock_task_handle;
extern TaskHandle_t sync_task_handle;
extern TaskHandle_t tempo_task_handle;
extern main_runtime_vrbs bc;
extern volatile main_mode mode;

QueueHandle_t tap_task_queue = NULL;
TaskHandle_t tap_task_handle = NULL;

clock_task_queue_entry clock_tx_buffer;

static void IRAM_ATTR tap_tempo_isr_handler(void *args)
{
    static uint64_t previous_switch_debounce = 0;
    uint64_t current_time_us = esp_timer_get_time();
    if (current_time_us > previous_switch_debounce + TAP_DEBOUNCE_TIME_US)
    {
        if (mode == MODE_TAP)
        {
            uint64_t current_onset = US_TO_MS(current_time_us);
            xQueueSendFromISR(tap_task_queue, &current_onset, (TickType_t)0);
        }
        else if (mode == MODE_PLAY)
        {
            xTaskNotify(mode_switch_task_handle, MODE_SWITCH_TO_TAP, eSetValueWithOverwrite);
            xQueueReset(tap_task_queue);
            uint64_t tap_task_msg = TAP_TASK_QUEUE_RESET_COUNTER;
            xQueueSendFromISR(tap_task_queue, &tap_task_msg,(TickType_t)0);
            uint64_t current_onset = US_TO_MS(current_time_us);
            xQueueSendFromISR(tap_task_queue, &current_onset, (TickType_t)0);
        }
        else
        {
            return;
        }
        previous_switch_debounce = current_time_us;
    }
}

static void tap_task(void *arg)
{
    ESP_LOGI("tap_task", "Started tap task");
    uint64_t tap_tempo_onsets[4] = {0};
    uint64_t tap_task_queue_result = 0;
    tap_task_queue = xQueueCreate(2, sizeof(tap_tempo_onsets[0]));
    uint8_t counter = 0;
    uint64_t time_of_last_hit = 0;
    while (1)
    {
        if (xQueueReceive(tap_task_queue, &tap_task_queue_result, 0))
        {
            if (tap_task_queue_result == TAP_TASK_QUEUE_RESET_COUNTER)
            {
                counter = 0;
            }
            else
            {
                tap_tempo_onsets[counter] = tap_task_queue_result;
                time_of_last_hit = esp_timer_get_time();
                counter++;
                int msg_to_hid = HID_TAP_0 + counter;
                xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
                ESP_LOGI("tap_task", "hit");
            }
        }
        if (counter == 4)
        {
            xTaskNotify(mode_switch_task_handle, MODE_SWITCH_TO_PLAY, eSetValueWithOverwrite);
            //mode = MODE_PLAY;
            uint64_t tap_period = 0;
            for (int i = 0; i < 3; i++)
            {
                tap_period += tap_tempo_onsets[i + 1] - tap_tempo_onsets[i];
            }
            tap_period = tap_period / 3; // 4th note
            bc.tau = tap_period / 2;     // 8th note
            bc.expected_beat = tap_tempo_onsets[3] + tap_period;
            ESP_LOGI("tap_task", "tau: %lld", bc.tau);
            xTaskNotify(sync_task_handle, SYNC_RESET_PARAMETERS, eSetValueWithOverwrite);
            xTaskNotify(tempo_task_handle, TEMPO_RESET_PARAMETERS, eSetValueWithOverwrite);
            xQueueReset(clock_task_queue);
            clock_tx_buffer.type = CLOCK_QUEUE_START;
            clock_tx_buffer.value = bc.expected_beat,
            xQueueSend(clock_task_queue, &clock_tx_buffer, (TickType_t)0);
            vTaskResume(clock_task_handle); /// xxx ?
            vTaskSuspend(NULL);
            counter = 0;
        }
        else
        {
            if (counter > 0 && esp_timer_get_time() > time_of_last_hit + TAP_HIT_WAIT_TIME_US)
            {
                ESP_LOGI("tap_task", "RESET TAP");
                counter = 0;
                int msg_to_hid = HID_TAP_0;
                xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
            }
            vTaskDelay(1);
        }
    }
}

void tap_init()
{
    gpio_reset_pin(TAP_TEMPO_PIN);
    gpio_set_direction(TAP_TEMPO_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(TAP_TEMPO_PIN, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(TAP_TEMPO_PIN, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(TAP_TEMPO_PIN, tap_tempo_isr_handler, NULL);
    xTaskCreate(tap_task, "Tap_Task", 4096, NULL, TAP_TASK_PRIORITY, &tap_task_handle);
};