/**
 * This is the main file. It contains the app_main function which runs first.
 * It calls all the init functions of the modules and then delets itself.
 */

#include "main_defs.h"
#include "onset_adc.h"
#include "sync.h"
#include "clock.h"
#include "tap.h"
#include "mode_switch.h"
#include "tempo.h"
#include "hid.h"
#include "driver/gpio.h"
#include "../components/ssd1306/ssd1306.h"
#include "../components/ssd1306/font8x8_basic.h"

/**
 * @brief Circular buffer for storing onset informations
 */
onset_entry onsets[ONSET_BUFFER_SIZE];

/**
 * @brief Mode the system is currently in
 */
volatile main_mode mode = MODE_TAP;

/**
 * @brief Mutex for protecting the bc variable
 */
SemaphoreHandle_t bc_mutex_handle = NULL;

/**
 * @brief Main runtime variable of the system
 */
main_runtime_vrbs bc = {
    .tau = 250, // 8th time in ms (bpm120)
    .bar_position = 0, // current bar position onto two bars (0-15)
    .layer = 0, // current layer value of the bar position
    .expected_beat = 0, // position of the next expected beat
    .there_is_an_onset = false, // indicates if there has been an onset
    .most_recent_onset_index = 0, // index of the most recent onset inside the array
    .last_relevant_onset_index_for_sync = -1, // index of the last relevant onset for sync calculation
    .last_relevant_onset_index_for_tempo = 0, // index of the last relevant onset for tempo calculation
};

/**
 * @brief This function runs only at the start of the system and it starts the modules and sets up some global parameters.
 */
void app_main(void)
{
    /*
    Install an isr service
    */
    gpio_install_isr_service(0);
    /*
    Create the mutex of bc variable
    */
    bc_mutex_handle = xSemaphoreCreateMutex();
    /*
    Start all the modules
    */
    ESP_LOGI("main.c","hid_init");
    hid_init();
    ESP_LOGI("main.c","mode_switch_init");
    mode_switch_init();
    ESP_LOGI("main.c","onset_init");
    onset_adc_init();
    sync_init();
    tempo_init();
    ESP_LOGI("main.c","tap_init");
    tap_init();
    ESP_LOGI("main.c","clock_init");
    clock_init();
    /*
    Refresh the menu values after all the modules setted their pointer
    */
    hid_set_up_values();
    /*
    Delete this task
    */
    int msg_to_hid = HID_TAP_MODE_SELECT;
    xQueueSend(hid_task_queue, &msg_to_hid, (TickType_t)0);
    vTaskDelete(NULL);
}
