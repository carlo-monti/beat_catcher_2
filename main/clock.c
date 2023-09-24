#include "main_defs.h"
#include "clock.h"
#include "sync.h"
#include "onset_adc.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"
#include "driver/ledc.h"
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

/**
 * @{ \name UART definitions
 */
#define UART_MIDI_1 UART_NUM_1
#define UART_MIDI_2 UART_NUM_2
/**
 * @}
 */

/**
 * @{ \name Audio click definitions
 */
#define AUDIO_CLICK_TIMER               LEDC_TIMER_0
#define AUDIO_CLICK_MODE                LEDC_LOW_SPEED_MODE
#define AUDIO_CLICK_OUTPUT_IO           (GPIO_NUM_15) // Define the output GPIO
#define AUDIO_CLICK_CHANNEL             LEDC_CHANNEL_0
#define AUDIO_CLICK_DUTY_RES            LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define AUDIO_CLICK_DUTY                (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define AUDIO_CLICK_FREQUENCY_FIRST     (640) // Frequency in Hertz. Set frequency at 5 kHz
#define AUDIO_CLICK_FREQUENCY           (440) // Frequency in Hertz. Set frequency at 5 kHz
/**
 * @}
 */

extern main_runtime_vrbs bc; // main runtime data
extern SemaphoreHandle_t bc_mutex_handle; // mutex for the access to bc struct
extern QueueHandle_t onset_adc_task_queue; // queue of onset_adc_task
TaskHandle_t clock_task_handle;
QueueHandle_t clock_task_queue;
gptimer_handle_t clock_timer_handle; // handle for the timer (that triggers cb to send midi clock)

const char MIDI_MSG_TIMING_CLOCK = 248; // MIDI CLOCK MESSAGE byte value
const char MIDI_MSG_START = 250; // MIDI START MESSAGE byte value
const char MIDI_MSG_STOP = 252; // MIDI STOP MESSAGE byte value
const uint8_t layer_of[TWO_BAR_LENGTH_IN_8TH] = {3, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0}; // layer value for every step in two bars
volatile uint8_t midi_tick_counter = 0; // current midi clock to send (0-11)
volatile uint64_t time_until_next_8th = 0; // time until next 8th note
volatile gptimer_alarm_config_t alarm_config = {
    .reload_count = 0,
    .alarm_count = 1000 * 1000,
    .flags.auto_reload_on_alarm = true,
};
volatile long long delta_tau_latency[TWO_BAR_LENGTH_IN_8TH] = {0}; // delta for compensating tempo change latency
volatile int64_t delta_tau_sync = 0; // delta for the sync process
ledc_timer_config_t audio_click_ledc_timer = {
    .speed_mode       = AUDIO_CLICK_MODE,
    .timer_num        = AUDIO_CLICK_TIMER,
    .duty_resolution  = AUDIO_CLICK_DUTY_RES,
    .freq_hz          = AUDIO_CLICK_FREQUENCY,  // Set output frequency at 5 kHz
    .clk_cfg          = LEDC_AUTO_CLK
};
ledc_channel_config_t audio_click_ledc_channel = {
    .speed_mode     = AUDIO_CLICK_MODE,
    .channel        = AUDIO_CLICK_CHANNEL,
    .timer_sel      = AUDIO_CLICK_TIMER,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = AUDIO_CLICK_OUTPUT_IO,
    .duty           = 0, // Set duty to 0%
    .hpoint         = 0
};

/**
 * @brief Function to initialize the uart
*/
static void init_uart()
{
    /*
    Set up first UART module parameters
    */
    uart_config_t uart_config_1 = {
        .baud_rate = 31250,
        .data_bits = UART_DATA_7_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_MIDI_1, &uart_config_1));
    ESP_ERROR_CHECK(uart_set_pin(UART_MIDI_1, UART_MIDI_1_TX, UART_MIDI_1_RX, 0, 0));
    const int uart_buffer_size = (256); 
    /*
    Create first UART queue
    */
    QueueHandle_t uart_queue_1;
    ESP_ERROR_CHECK(uart_driver_install(UART_MIDI_1, uart_buffer_size, uart_buffer_size, 10, &uart_queue_1, ESP_INTR_FLAG_SHARED));

    /*
    Set up second UART module parameters
    */
    uart_config_t uart_config_2 = {
        .baud_rate = 31250,
        .data_bits = UART_DATA_7_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_MIDI_2, &uart_config_2));
    ESP_ERROR_CHECK(uart_set_pin(UART_MIDI_2, UART_MIDI_2_TX, UART_MIDI_2_RX, 0, 0));
    /*
    Create second UART queue
    */
    QueueHandle_t uart_queue_2;
    ESP_ERROR_CHECK(uart_driver_install(UART_MIDI_2, uart_buffer_size, uart_buffer_size, 10, &uart_queue_2, ESP_INTR_FLAG_SHARED));
}

/**
 * @brief Function to initialize the audio_click with PWM
*/
static void init_audio_click(){
    ESP_ERROR_CHECK(ledc_timer_config(&audio_click_ledc_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&audio_click_ledc_channel));
    ledc_set_duty(AUDIO_CLICK_MODE, AUDIO_CLICK_CHANNEL, AUDIO_CLICK_DUTY);
    ledc_stop(audio_click_ledc_channel.speed_mode,audio_click_ledc_channel.channel,0);
}

/**
 * @brief Callback function that sends midi clock and reset timer
*/
void IRAM_ATTR send_midi_clock(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *param)
{
    gptimer_stop(timer);
    uart_tx_chars(UART_MIDI_1, &MIDI_MSG_TIMING_CLOCK, 1);
    uart_tx_chars(UART_MIDI_2, &MIDI_MSG_TIMING_CLOCK, 1);
    switch (midi_tick_counter)
        {
        case MIDI_CLOCK_ON_BEAT:
            /*
            Midi counter = 0/12 (first of the 8th note)
            */
            time_until_next_8th = bc.tau + delta_tau_latency[bc.bar_position]; // calculate time distance from now to the next step
            delta_tau_latency[bc.bar_position] = 0; // reset delta_tau_latency for current position
            alarm_config.alarm_count = ((time_until_next_8th + 6) / 12); // calculate and set period for MIDI Clock
            /*
            Turn on led and play audio click based on bar position
            */
            switch (bc.bar_position % 8)
            {
            case 0:
                ledc_set_freq(AUDIO_CLICK_MODE,AUDIO_CLICK_TIMER,AUDIO_CLICK_FREQUENCY_FIRST);
                ledc_update_duty(AUDIO_CLICK_MODE, AUDIO_CLICK_CHANNEL);
                gpio_set_level(FIRST_LED_PIN, 1);
                break;
            case 2:
                ledc_set_freq(AUDIO_CLICK_MODE,AUDIO_CLICK_TIMER,AUDIO_CLICK_FREQUENCY);
                ledc_update_duty(AUDIO_CLICK_MODE, AUDIO_CLICK_CHANNEL);
                gpio_set_level(SECOND_LED_PIN, 1);
                break;
            case 4:
                ledc_update_duty(AUDIO_CLICK_MODE, AUDIO_CLICK_CHANNEL);
                gpio_set_level(THIRD_LED_PIN, 1);
                break;
            case 6:
                ledc_update_duty(AUDIO_CLICK_MODE, AUDIO_CLICK_CHANNEL);
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
            /*
            Stop playing audio click
            */
            ledc_stop(audio_click_ledc_channel.speed_mode,audio_click_ledc_channel.channel,0);
            break;
        case MIDI_CLOCK_SECOND_THIRD:
            /*
            Ask onset_adc_task to stop logging onsets (notch of 16th) and start sync evaluation
            */
            int onset_adc_queue_value = ONSET_ADC_DISALLOW_ONSET_AND_START_SYNC;
            xQueueSendFromISR(onset_adc_task_queue, &onset_adc_queue_value, NULL);
            break;
        case MIDI_CLOCK_HALFWAY:
            /*
            Midi counter = 6/12 (Halfway between two 8th notes)
            */
            xSemaphoreTakeFromISR(bc_mutex_handle, NULL);
            /*
            Calculate the position of next 8th note keeping into account
            the delta_tau_sync_factor
            */
            time_until_next_8th = (bc.tau / 2) + delta_tau_sync;
            delta_tau_sync = 0;
            alarm_config.alarm_count = ((time_until_next_8th + 3) / 6);
            bc.expected_beat = esp_timer_get_time() + time_until_next_8th;     // set next clock as expected_beat
            bc.bar_position = (bc.bar_position + 1) % TWO_BAR_LENGTH_IN_8TH; // set new bar position number
            bc.layer = layer_of[bc.bar_position];
            xSemaphoreGiveFromISR(bc_mutex_handle,NULL);
            break;
        case MIDI_CLOCK_THIRD_THIRD:
            /*
            Ask onset_adc_task to resume logging onsets
            */
            onset_adc_queue_value = ONSET_ADC_ALLOW_ONSET;
            xQueueSendFromISR(onset_adc_task_queue, &onset_adc_queue_value, NULL);
            break;
        default:
            break;
        }
    midi_tick_counter = (midi_tick_counter + 1) % 12;
    ESP_ERROR_CHECK(gptimer_set_alarm_action(clock_timer_handle, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(timer));
}

void clock_timer_init()
{    
    /*
    Set up timer parameters
    */
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
        .flags.intr_shared = false,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &clock_timer_handle));
    /*
    Set timer callback function
    */
    gptimer_event_callbacks_t cb = {
        .on_alarm = send_midi_clock,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(clock_timer_handle, &cb, (void*)NULL));
    /*
    Set timer alarm value
    */
    ESP_ERROR_CHECK(gptimer_set_alarm_action(clock_timer_handle, &alarm_config));
    ESP_ERROR_CHECK(gptimer_enable(clock_timer_handle));
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
    uint16_t tempo_latency_amount = 0; // amount of the latency compensation
    uint16_t tempo_latency_smooth = 0; // amount of the latency compensation

    while (1)
    {
        /*
        Check for messages waiting in queue
        */
        clock_task_queue_entry rx_buffer;
        if (xQueueReceive(clock_task_queue, &rx_buffer, portMAX_DELAY))
        {
            /*
            If there is a message do what it asks:
            */
            int onset_adc_queue_value; // prepare message to be sent to onset_adc_task
            switch (rx_buffer.type)
            {
            case CLOCK_QUEUE_SET_DELTA_TAU_SYNC:
                ESP_LOGI("CLOCK","SET SYNC%lld",rx_buffer.value);
                /*
                Sync module asked to update sync value
                */
                delta_tau_sync = rx_buffer.value;
                break;
            case CLOCK_QUEUE_SET_DELTA_TAU_TEMPO:
                ESP_LOGI("CLOCK","SET TEMPO\t\t %lld",rx_buffer.value);
                /*
                Tempo module asked to update tempo value (takes mutex on bc!)
                */
                xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
                int64_t delta_tau_tempo = rx_buffer.value; // Get delta tau tempo value
                if (delta_tau_tempo > bc.tau * -0.8) // this avoids having a too much lower value and going back in time!
                {
                    bc.tau += delta_tau_tempo;  // update the current bpm
                }
                else
                {
                    ESP_LOGE("CLOCK", "delta_tau_tempo too low!");
                }
                /*
                Distribute delta_tau_tempo value for compensating latency
                for (int j = 1; j <= 8; j++)
                {
                    delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH] += delta_tau_tempo * (float)(1/tempo_latency_amount);
                    if (delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH] < (long)(bc.tau * -0.8)) // this avoids having a too much lower value and going back in time!
                    {
                        ESP_LOGE("CLOCK", "delta_tau_tempo latency too low!");
                        delta_tau_latency[(bc.bar_position + j) % TWO_BAR_LENGTH_IN_8TH] = (long)(bc.tau * -0.8);
                    }
                }
                */
                xSemaphoreGive(bc_mutex_handle);
                break;
            case CLOCK_QUEUE_STOP:
                /*
                Someone asked to stop sequence
                */
                gptimer_stop(clock_timer_handle);
                uart_write_bytes(UART_MIDI_1, &MIDI_MSG_STOP, 1); // send stop message
                uart_write_bytes(UART_MIDI_2, &MIDI_MSG_STOP, 1); // send stop message
                /*
                Ask onset_adc to stop logging onset 
                */
                onset_adc_queue_value = ONSET_ADC_DISALLOW_ONSET;
                xQueueSend(onset_adc_task_queue, &onset_adc_queue_value, 0);
                /*
                Turn off all leds
                */
                gpio_set_level(FIRST_LED_PIN, 0);
                gpio_set_level(SECOND_LED_PIN, 0);
                gpio_set_level(THIRD_LED_PIN, 0);
                gpio_set_level(FOURTH_LED_PIN, 0);
                /*
                Stop playing audio click
                */
                ledc_stop(audio_click_ledc_channel.speed_mode,audio_click_ledc_channel.channel,0);
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
                Tap ask to start the sequence:
                Reset parameters
                */
                midi_tick_counter = 0;
                delta_tau_sync = 0;
                memset(delta_tau_latency, 0, sizeof(delta_tau_latency));
                time_until_next_8th = 0;
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
                uint64_t wait_time_until_first_clock = rx_buffer.value - esp_timer_get_time();
                xQueueReset(clock_task_queue);
                /*
                Send MIDI START MESSAGE
                */
                uart_write_bytes(UART_MIDI_1, &MIDI_MSG_START, 1);
                uart_write_bytes(UART_MIDI_2, &MIDI_MSG_START, 1);
                alarm_config.alarm_count = wait_time_until_first_clock;
                ESP_ERROR_CHECK(gptimer_set_alarm_action(clock_timer_handle, &alarm_config));
                gptimer_start(clock_timer_handle);
                break;
            default:
                ESP_LOGE("CLOCK", "INVALID queue value");
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
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
    clock_timer_init();
    init_audio_click();
    xTaskCreate(clock_task, "Clock_Task", CLOCK_TASK_STACK_SIZE, NULL, CLOCK_TASK_PRIORITY, &clock_task_handle);
}