/*! \mainpage
 *
 * \section intro_sec About
 *
 * Beat Catcher 2.0 is a beat-tracking device for synchronizing electronic systems (drum machines, sequencer, arpeggiator, ...) with an acoustic drum played by a human performer. Using one piezoelectric sensor on the Kick and another on the Snare, Beat Catcher 2.0 detects the tempo currently held by the musician and sends out a Midi Clock signal that is in sync with it.
 * Beat Catcher 2.0 is an implementation of the B-Keeper beat-tracking algorithm (Robertson, Andrew, and Mark D. Plumbley. “Synchronizing Sequencing Software to a Live Drummer.” Computer Music Journal, vol. 37, no. 2, 2013). An updated version of B-Keeper algorithm is now included in Ableton Live 11, a de facto standard for electro/acoustic performances. Beat Catcher 2.0 represents an alternative method that is cheap and portable: a DAWless way of doing syncronization.
 * The whole project is based on an ESP32 MCU and uses other peripherals such as an OLED Display SSD1306, an Encoder KY-040 and various electronic components (piezo sensors, leds...).
 *
 * \section Code
 * The code has been developed with the ESP-IDF framework and can be found at: <a href="https://github.com/carlo-monti/beat_catcher_2">https://github.com/carlo-monti/beat_catcher_2</a> 
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
    hid_init();
    mode_switch_init();
    onset_adc_init();
    sync_init();
    tempo_init();
    tap_init();
    clock_init();
    /*
    Refresh the menu values after all the modules setted their pointer
    */
    hid_set_up_values();
    /*
    Delete this task
    */
    vTaskDelete(NULL);
}
