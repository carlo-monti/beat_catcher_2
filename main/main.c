/*! \mainpage
 *
 * \section intro_sec Introduction
 *
 * Beat Catcher 2.0 is the new implementation of Beat Catcher 1.0, a tempo tracking system. It is based
 * on the algor
 *
 * \section install_sec Installation
 *
 * \subsection step1 Step 1: Opening the box
 *
 * etc...
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

UBaseType_t clockHighWaterMark;
UBaseType_t hidHighWaterMark;
UBaseType_t onset_adcHighWaterMark;
UBaseType_t mode_switchHighWaterMark;
UBaseType_t syncHighWaterMark;
UBaseType_t tapHighWaterMark;
UBaseType_t tempoHighWaterMark;

onset_entry onsets[ONSET_BUFFER_SIZE];
volatile main_mode mode = MODE_TAP;
SemaphoreHandle_t bc_mutex_handle = NULL;
main_runtime_vrbs bc = {
    .tau = 250, // 8th time in ms (bpm120)
    .bar_position = 0,
    .layer = 0,
    .expected_beat = 0,
    .there_is_an_onset = false,
    .most_recent_onset_index = 0,
    .last_relevant_onset_index_for_sync = -1,
    .last_relevant_onset_index_for_tempo = 0,
};

void app_main(void)
{
    gpio_install_isr_service(0);  
    bc_mutex_handle = xSemaphoreCreateMutex();
    hid_init();
    mode_switch_init();
    onset_adc_init();
    sync_init();
    tempo_init();
    tap_init();
    clock_init();
    hid_set_up_values();
    vTaskDelete(NULL);
}
