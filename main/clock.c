#include "main_defs.h"
#include "clock.h"
#include "sync.h"
#include "onset_adc.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "hid.h"

/**
 * @{ \name midi_clock_counter relevant values
 */
#define MIDI_CLOCK_ON_BEAT 0
#define MIDI_CLOCK_LED_OFF 1
#define MIDI_CLOCK_SECOND_THIRD 4
#define MIDI_CLOCK_HALFWAY 6
#define MIDI_CLOCK_THIRD_THIRD 8
/**
 * @}
 */

extern main_runtime_vrbs bc; // main runtime data
extern SemaphoreHandle_t bc_mutex_handle; // mutex for the access to bc struct
extern QueueHandle_t onset_adc_task_queue; // queue of onset_adc_task
TaskHandle_t clock_task_handle;
QueueHandle_t clock_task_queue;

const char MIDI_MSG_TIMING_CLOCK = 248; // MIDI CLOCK MESSAGE byte value
const char MIDI_MSG_START = 250; // MIDI START MESSAGE byte value
const char MIDI_MSG_STOP = 252; // MIDI STOP MESSAGE byte value
const uint8_t layer_of[TWO_BAR_LENGTH_IN_8TH] = {3, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0}; // layer value for every step in two bars
const uart_port_t uart_num = UART_NUM_2;  // Use ESP32 UART number 2

/**
 * @brief Function to initialize the uart
*/
static void init_uart()
{
    /*
    Set up UART module parameters
    */
    uart_config_t uart_config = {
        .baud_rate = 31250,
        .data_bits = UART_DATA_7_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_PIN_1, UART_PIN_2, 0, 0));
    const int uart_buffer_size = (256); 
    /*
    Create UART queue
    */
    QueueHandle_t uart_queue;
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size, uart_buffer_size, 10, &uart_queue, ESP_INTR_FLAG_SHARED));
}

/**
 * @brief Main task of the Clock module
*/
static void clock_task(void *arg)
{   
    /*
    Set up UART
    */
    init_uart();
    /*
    Create task queue
    */
    clock_task_queue = xQueueCreate(5, sizeof(clock_task_queue_entry));
    if (clock_task_queue == 0)
    {
        ESP_LOGE("ERROR", "Failed to create queue= %p\n", clock_task_queue);
    }
    /*
    Create variables for the task
    */
    static uint8_t midi_tick_counter = 0;
    static int delta_tau_latency[TWO_BAR_LENGTH_IN_8TH] = {0}; // delta for compensating tempo change latency
    TickType_t last_clock_tick_val;
    bool is_on = false; // Indicates if the sequence of MIDI CLOCKs is on
    uint64_t delta_tau_sync = 0; // delta for the sync process
    uint64_t midi_clock_period = 0; // period of the MIDI CLOCK signal (also period of clock_task repetition)
    uint64_t time_to_next_8th = 0; // distance of the next 8th note
    uint16_t tempo_latency_amount; // amount of the latency compensation
    set_menu_item_pointer_to_vrb(MENU_INDEX_TEMPO_LATENCY_AMOUNT, &tempo_latency_amount);  // add tempo_latency_amount vrb to menu

    while (1)
    {
        /*
        Check for messages waiting in queue
        */
        clock_task_queue_entry rx_buffer;
        if (xQueueReceive(clock_task_queue, &rx_buffer, 0))
        {
            /*
            If there is a message do what it asks:
            */
            int onset_adc_queue_value; // prepare message to be sent to onset_adc_task
            switch (rx_buffer.type)
            {
            case CLOCK_QUEUE_SET_DELTA_TAU_SYNC:
                /*
                Sync module asked to update sync value
                */
                delta_tau_sync = rx_buffer.value;
                break;
            case CLOCK_QUEUE_SET_DELTA_TAU_TEMPO:
                /*
                Tempo module asked to update tempo value (takes mutex on bc!)
                */
                xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
                int64_t delta_tau_tempo = rx_buffer.value; // Get delta tau tempo value
                if (delta_tau_tempo > bc.tau-0.8) // this avoids having a too much lower value and going back in time!
                {
                    bc.tau += delta_tau_tempo;  // update the current bpm
                }
                else
                {
                    ESP_LOGE("CLOCK", "delta_tau_tempo too low!");
                }
                /*
                Distribute delta_tau_tempo value for compensating latency
                */
                for (int j = 1; j <= 8; j++)
                {
                    delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH] += delta_tau_tempo * (float)(1/tempo_latency_amount);
                    if (delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH] < (long)(bc.tau-0.8)) // this avoids having a too much lower value and going back in time!
                    {
                        ESP_LOGE("CLOCK", "delta_tau_tempo latency too low!");
                        delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH] = (long)(bc.tau-0.8);
                    }
                }
                xSemaphoreGive(bc_mutex_handle);
                break;
            case CLOCK_QUEUE_STOP:
                /*
                Someone asked to stop sequence
                */
                uart_write_bytes(uart_num, &MIDI_MSG_STOP, 1); // send stop message
                /*
                Ask onset_adc to stop logging onset 
                */
                onset_adc_queue_value = ONSET_ADC_DISALLOW_ONSET;
                xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, 0);
                /*
                Stop sequence (stops repeating this task)
                */
                is_on = false;
                /*
                Turn off all leds
                */
                gpio_set_level(FIRST_LED_PIN, 0);
                gpio_set_level(SECOND_LED_PIN, 0);
                gpio_set_level(THIRD_LED_PIN, 0);
                gpio_set_level(FOURTH_LED_PIN, 0);
                /*
                Clear the queue
                */
                xQueueReset(clock_task_queue);
                /*
                Suspend itself
                */
                vTaskSuspend(NULL);
                break;
            case CLOCK_QUEUE_START:
                /*
                Tap asked to start the sequence
                */
                is_on = true;
                /*
                Reset parameters
                */
                midi_tick_counter = 0;
                delta_tau_sync = 0;
                midi_clock_period = 0;
                memset(delta_tau_latency, 0, sizeof(delta_tau_latency));
                time_to_next_8th = 0;
                xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
                bc.bar_position = 0;
                bc.layer = 3;
                bc.most_recent_onset_index = 0;
                bc.last_relevant_onset_index_for_sync = 0;
                bc.last_relevant_onset_index_for_tempo = 0;
                bc.there_is_an_onset = false;
                xSemaphoreGive(bc_mutex_handle);
                /*
                Ask onset_adc to start logging onsets
                */
                onset_adc_queue_value = ONSET_ADC_ALLOW_ONSET;
                xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, 1);
                /*
                Calculate the time at which the first MIDI CLOCK has to be sent
                */                
                uint64_t wait_time_until_first_clock = rx_buffer.value - GET_CURRENT_TIME_MS();
                last_clock_tick_val = xTaskGetTickCount();
                xQueueReset(clock_task_queue);
                /*
                Send MIDI START MESSAGE
                */
                uart_write_bytes(uart_num, &MIDI_MSG_START, 1);
                /*
                Delays itself until it is time to send first MIDI CLOCK
                */
                xTaskDelayUntil(&last_clock_tick_val, pdMS_TO_TICKS(wait_time_until_first_clock));
                break;
            default:
                ESP_LOGE("CLOCK", "INVALID queue value");
                break;
            }
        }
        if (is_on)
        {                
            /*
            Send MIDI CLOCK message
            */
            uart_write_bytes(uart_num, &MIDI_MSG_TIMING_CLOCK, 1);
            /*
            Do other task depending on the index midi_tick_counter
            */
            switch (midi_tick_counter)
            {
            case MIDI_CLOCK_ON_BEAT:
                /*
                Index 0/12 (first of the 8th note)
                */
                time_to_next_8th = bc.tau + delta_tau_latency[bc.bar_position]; // calculate time distance from now to the next step
                delta_tau_latency[bc.bar_position] = 0; // reset delta_tau_latency for current position
                midi_clock_period = (time_to_next_8th + 6) / 12; // calculate period for MIDI Clock
                /*
                Turn on led depending on bar position
                */
                switch (bc.bar_position % 8)
                {
                case 0:
                    gpio_set_level(FIRST_LED_PIN, 1);
                    break;
                case 2:
                    gpio_set_level(SECOND_LED_PIN, 1);
                    break;
                case 4:
                    gpio_set_level(THIRD_LED_PIN, 1);
                    break;
                case 6:
                    gpio_set_level(FOURTH_LED_PIN, 1);
                    break;
                default:
                    break;
                }
                break;
            case MIDI_CLOCK_LED_OFF:
                /*
                Turn off leds
                */
                gpio_set_level(FIRST_LED_PIN, 0);
                gpio_set_level(SECOND_LED_PIN, 0);
                gpio_set_level(THIRD_LED_PIN, 0);
                gpio_set_level(FOURTH_LED_PIN, 0);
                break;
            case MIDI_CLOCK_SECOND_THIRD:
                /*
                Ask onset_adc_task to stop logging onsets (notch of 16th) and start sync evaluation
                */
                int onset_adc_queue_value = ONSET_ADC_DISALLOW_ONSET_AND_START_SYNC;
                xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, 1);
                break;
            case MIDI_CLOCK_HALFWAY:
                /*
                Index 6/12 (Halfway between two 8th notes)
                */
                xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
                /*
                Calculate the position of next 8th note keeping into account
                the delta_tau_sync_factor
                */
                time_to_next_8th = ((bc.tau + 1) / 2) + delta_tau_sync;
                delta_tau_sync = 0;
                midi_clock_period = (time_to_next_8th + 3) / 6;
                bc.expected_beat = GET_CURRENT_TIME_MS() + time_to_next_8th;     // set next clock as expected_beat
                bc.bar_position = (bc.bar_position + 1) % TWO_BAR_LENGTH_IN_8TH; // set new bar position number
                bc.layer = layer_of[bc.bar_position];
                xSemaphoreGive(bc_mutex_handle);
                break;
            case MIDI_CLOCK_THIRD_THIRD:
                /*
                Ask onset_adc_task to resume logging onsets
                */
                onset_adc_queue_value = ONSET_ADC_ALLOW_ONSET;
                xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, 1);
                break;
            default:
                break;
            }
            /*
            Delays itself until it is time to send the next MIDI CLOCK
            */
            xTaskDelayUntil(&last_clock_tick_val, pdMS_TO_TICKS(midi_clock_period));
            midi_tick_counter = (midi_tick_counter + 1) % 12;
        }
        else
        {               
            /*
            This is executed only one time at the start of the system
            */
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

/**
 * @brief Initialization of clock
 * Sets GPIO and creates clock_task
*/
void clock_init()
{
    /*
    Set up GPIO pins for the led 1-4
    */
    gpio_reset_pin(FIRST_LED_PIN);
    gpio_set_direction(FIRST_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(SECOND_LED_PIN);
    gpio_set_direction(SECOND_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(THIRD_LED_PIN);
    gpio_set_direction(THIRD_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(FOURTH_LED_PIN);
    gpio_set_direction(FOURTH_LED_PIN, GPIO_MODE_OUTPUT);
    /*
    Create clock_task
    */
    xTaskCreate(clock_task, "Clock_Task", CLOCK_TASK_STACK_SIZE, NULL, CLOCK_TASK_PRIORITY, &clock_task_handle);
}