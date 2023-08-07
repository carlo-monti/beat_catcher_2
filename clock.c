#include "main_defs.h"
#include "clock.h"
#include "sync.h"
#include "onset_adc.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "hid.h"

#define TEMPO_LATENCY_WIDTH 5
#define TEMPO_LATENCY_START 5

typedef enum
{
    MIDI_CLOCK_ON_BEAT = 0,
    MIDI_CLOCK_LED_OFF = 1,
    MIDI_CLOCK_SECOND_THIRD = 4,
    MIDI_CLOCK_HALFWAY = 6,
    MIDI_CLOCK_THIRD_THIRD = 8,
} midi_clock_counter_index;


extern TaskHandle_t sync_task_handle;
extern main_runtime_vrbs bc;
extern onset_entry onsets[];
extern QueueHandle_t onset_adc_task_queue;

TaskHandle_t clock_task_handle;
QueueHandle_t clock_task_queue;

const char MIDI_MSG_TIMING_CLOCK = 248;
const char MIDI_MSG_START = 250;
const char MIDI_MSG_STOP = 252;
const uint8_t layer_of[TWO_BAR_LENGTH_IN_8TH] = {3, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
const uart_port_t uart_num = UART_NUM_2;

static uint64_t last = 0; /// xxx togliere

static void init_uart(){
    uart_config_t uart_config = {
        .baud_rate = 31250,
        .data_bits = UART_DATA_7_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, UART_PIN_1, UART_PIN_2, 0, 0)); // check pin xxx
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size, uart_buffer_size, 10, &uart_queue, ESP_INTR_FLAG_SHARED));
    /* TASK QUEUE */
    clock_task_queue = xQueueCreate(5, sizeof(clock_task_queue_entry));
    if (clock_task_queue == 0)
    {
        ESP_LOGE("ERROR", "Failed to create queue= %p\n", clock_task_queue);
    }
}

static void clock_task(void *arg)
{
    init_uart();
    static uint8_t midi_tick_counter = 0;
    uint64_t delta_tau_sync = 0;
    uint64_t midi_clock_period = 0;                             // delta for the synchronizing process
    static long delta_tau_latency[TWO_BAR_LENGTH_IN_8TH] = {0}; // delta for compensate tempo change
    uint64_t time_to_next_8th = 0;
    TickType_t last_clock_tick_val;
    bool is_on = false;
    uint16_t tempo_latency_amount;
    uint16_t tempo_latency_smooth;
    set_menu_item_pointer_to_vrb(MENU_INDEX_TEMPO_LATENCY_AMOUNT, &tempo_latency_amount);
    set_menu_item_pointer_to_vrb(MENU_INDEX_TEMPO_LATENCY_SMOOTH, &tempo_latency_smooth);
    while (1)
    {
        // last_clock_tick_val = xTaskGetTickCount();
        clock_task_queue_entry rx_buffer;
        int onset_adc_queue_value;
        if (xQueueReceive(clock_task_queue, &rx_buffer, 0))
        {
            switch (rx_buffer.type)
            {
            case CLOCK_QUEUE_SET_DELTA_TAU_SYNC:
                delta_tau_sync = rx_buffer.value;
                ESP_LOGI("CLOCK", "Deltatau sync set %lld",delta_tau_sync);
                break;
            case CLOCK_QUEUE_SET_DELTA_TAU_TEMPO:
                int64_t delta_tau_tempo = rx_buffer.value; // value we receive
                ESP_LOGI("CLOCK", "Tau %ld",(long)(bc.tau * -0.8));
                ESP_LOGI("CLOCK", "Deltatempo sync set %lld",delta_tau_tempo);
                int factor = tempo_latency_amount;
                if(delta_tau_tempo > bc.tau * -0.8){
                    bc.tau += delta_tau_tempo; // this avoids having a too much lower value and going back in time!
                }else{
                    ESP_LOGI("CLOCK", "delta_tau_tempo too low!");
                }
                for (int j = 1; j <= tempo_latency_smooth; j++)
                {
                    delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH] += delta_tau_tempo * factor;
                    ESP_LOGI("CLOCK", "delta_tau_latency[%d]: %ld", j, delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH]);
                    if(delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH] < (long)(bc.tau * -0.8)){
                        // this avoids having a too much lower value and going back in time!
                        ESP_LOGI("CLOCK", "delta_tau_tempo latency too low!");
                        delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH] = (long)(bc.tau * -0.8);
                    }
                    factor--;
                }
                break;
            case CLOCK_QUEUE_STOP:
                uart_write_bytes(uart_num, &MIDI_MSG_STOP, 1);
                ESP_LOGI("CLOCK", "MIDI STOP sent");
                onset_adc_queue_value = ONSET_ADC_DISALLOW_ONSET;
                xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, 0);
                is_on = false;
                gpio_set_level(FIRST_LED_PIN, 0);
                gpio_set_level(SECOND_LED_PIN, 0);
                gpio_set_level(THIRD_LED_PIN, 0);
                gpio_set_level(FOURTH_LED_PIN, 0);
                xQueueReset(clock_task_queue);
                vTaskSuspend(NULL);
                break;
            case CLOCK_QUEUE_START:
                is_on = true;
                midi_tick_counter = 0;
                delta_tau_sync = 0;
                midi_clock_period = 0; // delta for the synchronizingprocess
                memset(delta_tau_latency, 0, sizeof(delta_tau_latency));
                time_to_next_8th = 0;
                bc.bar_position = 0;
                bc.layer = 3;
                bc.most_recent_onset_index = 0;
                bc.last_relevant_onset_index_for_sync = 0;
                bc.last_relevant_onset_index_for_tempo = 0;
                bc.there_is_an_onset = false;
                int onset_adc_queue_value = ONSET_ADC_ALLOW_ONSET;
                if (xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, 1))
                {
                    // ESP_LOGI("CLOCK", "ONSET_ADC_DISALLOW_ONSET sent");
                };
                uint64_t wait_time_until_first_clock = rx_buffer.value - GET_CURRENT_TIME_MS(); // value we receive
                last_clock_tick_val = xTaskGetTickCount();
                xQueueReset(clock_task_queue);
                ESP_LOGI("CLOCK", "NOW: %ld", last_clock_tick_val);
                ESP_LOGI("CLOCK", "WAIT FOR: %lld", wait_time_until_first_clock);
                xTaskDelayUntil(&last_clock_tick_val, pdMS_TO_TICKS(wait_time_until_first_clock));
                uart_write_bytes(uart_num, &MIDI_MSG_START, 1);
                ESP_LOGI("CLOCK", "MIDI START sent");
                break;
            default:
                ESP_LOGE("ERROR", "INVALID queue value 1234");
                break;
            }
        }
        if (is_on)
        {
            switch (midi_tick_counter)
            {
            case MIDI_CLOCK_ON_BEAT:
                // 0 (on the 8th note)
                time_to_next_8th = bc.tau + delta_tau_latency[bc.bar_position]; //  // calculate time distance from now to the next step
                delta_tau_latency[bc.bar_position] = 0;
                // reset delta_tau_latency for current position
                midi_clock_period = (time_to_next_8th + 6) / 12; // calculate tau for MIDI Clock
                switch (bc.bar_position % 8)
                {
                case 0:
                    gpio_set_level(FIRST_LED_PIN, 1);
                    //ESP_LOGI("CLOCK", "Time: %lld tau %lld midi %lld mstotick %ld", (esp_timer_get_time() - last) * 12, bc.tau, midi_clock_period, pdMS_TO_TICKS(midi_clock_period));
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
                gpio_set_level(FIRST_LED_PIN, 0);
                gpio_set_level(SECOND_LED_PIN, 0);
                gpio_set_level(THIRD_LED_PIN, 0);
                gpio_set_level(FOURTH_LED_PIN, 0);
                break;
            case MIDI_CLOCK_SECOND_THIRD:
                onset_adc_queue_value = ONSET_ADC_DISALLOW_ONSET_AND_START_SYNC;
                if (xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, 1))
                {
                    // ESP_LOGI("CLOCK", "ONSET_ADC_DISALLOW_ONSET sent");
                };
                break;
            case MIDI_CLOCK_HALFWAY:
                
                // 6 (half way between the 8th notes)
                if(delta_tau_sync!=0){
                    //ESP_LOGI("clock","delta_tau_sync added %lld", delta_tau_sync);
                }
                time_to_next_8th = ((bc.tau + 1) / 2) + delta_tau_sync;
                delta_tau_sync = 0;
                // reset delta_tau_sync
                midi_clock_period = (time_to_next_8th + 3) / 6;
                // calculate new period for MIDI Clock
                // xxx get mutex
                bc.expected_beat = GET_CURRENT_TIME_MS() + time_to_next_8th;     // set next clock as expectedBeat
                bc.bar_position = (bc.bar_position + 1) % TWO_BAR_LENGTH_IN_8TH; // set new bar position number
                bc.layer = layer_of[bc.bar_position];
                break;
            case MIDI_CLOCK_THIRD_THIRD:
                onset_adc_queue_value = ONSET_ADC_ALLOW_ONSET;
                if (xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, 1))
                {
                    // ESP_LOGI("CLOCK", "ONSET_ADC_ALLOW_ONSET sent");
                };

                break;
            default:
                // ESP_LOGI("clock","Pass");
                break;
            }

            last = esp_timer_get_time();
            uart_write_bytes(uart_num, &MIDI_MSG_TIMING_CLOCK, 1);
            xTaskDelayUntil(&last_clock_tick_val, pdMS_TO_TICKS(midi_clock_period));
            midi_tick_counter = (midi_tick_counter + 1) % 12;
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void clock_init()
{
    gpio_reset_pin(FIRST_LED_PIN);
    gpio_set_direction(FIRST_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(SECOND_LED_PIN);
    gpio_set_direction(SECOND_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(THIRD_LED_PIN);
    gpio_set_direction(THIRD_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(FOURTH_LED_PIN);
    gpio_set_direction(FOURTH_LED_PIN, GPIO_MODE_OUTPUT);
    xTaskCreate(clock_task, "Clock_Task", 4096, NULL, CLOCK_TASK_PRIORITY, &clock_task_handle);
}