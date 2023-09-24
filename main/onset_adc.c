#include "main_defs.h"
#include "esp_adc/adc_continuous.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "onset_adc.h"
#include "sync.h"
#include "hid.h"

#define GAIN_CLIP_VALUE 4094

/**
 * @{ \name ADC set up values (channels, attenuation, ...)
 */
#define ADC_UNIT ADC_UNIT_1
#define ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_0
#define ADC_BIT_WIDTH SOC_ADC_DIGI_MAX_BITWIDTH

#if CONFIG_IDF_TARGET_ESP32
#define ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE1 // 1 for esp32 |2 for esp32-s3
#define ADC_GET_CHANNEL(p_data) ((p_data)->type1.channel) // type1 for esp32 | type2 for esp32-s3
#define ADC_GET_DATA(p_data) ((p_data)->type1.data) // type1 for esp32 | type2 for esp32-s3
#define KICK_ADC_CHANNEL ADC_CHANNEL_3
#define SNARE_ADC_CHANNEL ADC_CHANNEL_6
#else
#define ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2      // 1 for esp32 2 for esp32-s3
#define ADC_GET_CHANNEL(p_data) ((p_data)->type2.channel) // type1 for esp32 | type2 for esp32-s3
#define ADC_GET_DATA(p_data) ((p_data)->type2.data)       // type1 for esp32 | type2 for esp32-s3
#define KICK_ADC_CHANNEL ADC_CHANNEL_3
#define SNARE_ADC_CHANNEL ADC_CHANNEL_4
#endif
/**
 * @}
 */

/**
 * @{ \name Buffer and oversampling parameters
 */
#define BUFFER_SIZE 512
#define OVERSAMPLING 8
#define SAMPLE_FREQ 48000
/**
 * @}
 */

/**
 * @{ \name Shortcuts
 */
#define KICK 0
#define SNARE 1
#define MAX_ONSET_DELTA_X_LENGTH 600
/**
 * @}
 */

/**
 * @brief Config struct for the ADC channel
 * It includes values for the onset detection.
*/
typedef struct
{
    uint16_t decrease; //  Value that is subtracted from the stored sample value for tracking envelope.
    uint16_t delta_threshold; // Minimum amount of the amplitude increase to trigger a new onset.
    uint16_t delta_x; // Number of sample for calculating the amplitude increase
    uint64_t gate_time_us;// Time until a new onset is triggered
} onset_channel_cfg;

/**
 * @brief Config struct for the blinking led timer
 * It includes the led pin, the blink duration and the handle of the timer
*/
typedef struct
{
    uint8_t pin;
    uint32_t blink_duration;
    gptimer_handle_t timer_handle;
} led_cfg;

/**
 * @brief Config struct for the runtime values of the onset detection
*/
typedef struct
{
    uint16_t current_sample;// Value of the current sample
    uint64_t last_onset_time;// Last time a onset has been triggered
    uint16_t past_samples_current_onset_index;// Index of the current sample in the array of past samples
    uint16_t past_samples[MAX_ONSET_DELTA_X_LENGTH];// Array of the past samples
} runtime_onset_values;

extern TaskHandle_t onset_adc_task_handle; // onset_adc task handle 
extern SemaphoreHandle_t bc_mutex_handle; // mutex for the access to bc struct 
extern TaskHandle_t sync_task_handle; // sync_task handle
extern main_runtime_vrbs bc; // global struct with runtime vrbs
extern onset_entry onsets[]; // array of onsets
extern void set_menu_item_pointer_to_vrb(menu_item_index index, void *ptr);

TaskHandle_t onset_adc_task_handle = NULL;
QueueHandle_t onset_adc_task_queue = NULL;
adc_continuous_handle_t adc_handle = NULL;
/*
Initialize adc channel array
*/
adc_channel_t channel[] = {KICK_ADC_CHANNEL, SNARE_ADC_CHANNEL};

/*
Set up led struct
*/
led_cfg onset_led[2] = {
    {
        .pin = KICK_LED_PIN,
        .blink_duration = KICK_LED_PIN_BLINK_DURATION,
        .timer_handle = NULL,
    },
    {
        .pin = SNARE_LED_PIN,
        .blink_duration = SNARE_LED_PIN_BLINK_DURATION,
        .timer_handle = NULL,
    }};

/*
Initialize onset detection variables
*/
bool allow_onset = false;
bool display_gain = false;
bool has_onset = false;

/**
 * @brief Callback function used to notify the onset_adc_task that buffer is ready
*/
static bool IRAM_ATTR conv_done_cb(adc_continuous_handle_t adc_handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    vTaskNotifyGiveFromISR(onset_adc_task_handle, &mustYield);
    return (mustYield == pdTRUE);
}

/**
 * @brief Callback function used to turn off led to make it blink only once
*/
bool IRAM_ATTR turn_off_led_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *led_pin)
{
    BaseType_t mustYield = pdFALSE;
    gpio_set_level(*(uint8_t *)led_pin, 0);
    gptimer_stop(timer);
    return (mustYield == pdTRUE);
}

/**
 * @brief ADC initialization function
*/
static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    /*
    Set up ADC config structs and DMA parameters
    */
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = BUFFER_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle));
    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_FREQ,
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_OUTPUT_TYPE,
    };
    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++)
    {
        adc_pattern[i].atten = ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = ADC_UNIT;
        adc_pattern[i].bit_width = ADC_BIT_WIDTH;
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));
    *out_handle = adc_handle;
    /*
    Add callback function to continuous adc module
    */
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
}

/** 
 * @brief Init function for the blinking led action
 * When an onset is triggered, the function blink_led is called. The led is than turned on and
 * a timer is started to trigger an interrupt that will turn it off after a duration period.
 * This function inits the GPIO and the timer.
*/
void led_blink_init()
{
    /*
    Config the GPIO for the two leds
    */
    uint8_t n_of_leds = sizeof(onset_led) / sizeof(onset_led[0]);
    for (uint8_t i = 0; i < n_of_leds; i++)
    {
        gpio_reset_pin(onset_led[i].pin);
        gpio_set_direction(onset_led[i].pin, GPIO_MODE_OUTPUT);
        /*
        Create timers
        */
        gptimer_config_t timer_config = {
            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = 1000000, // 1MHz, 1 tick=1us
            .flags.intr_shared = true,
        };
        ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &onset_led[i].timer_handle));
        /*
        Set timer callback function
        */
        gptimer_event_callbacks_t cb = {
            .on_alarm = turn_off_led_cb,
        };
        ESP_ERROR_CHECK(gptimer_register_event_callbacks(onset_led[i].timer_handle, &cb, &onset_led[i].pin));
        /*
        Set timer alarm value
        */
        gptimer_alarm_config_t alarm_config = {
            .reload_count = 0,
            .alarm_count = onset_led[i].blink_duration * 1000,
            .flags.auto_reload_on_alarm = true,
        };
        ESP_ERROR_CHECK(gptimer_set_alarm_action(onset_led[i].timer_handle, &alarm_config));
        ESP_ERROR_CHECK(gptimer_enable(onset_led[i].timer_handle));
    }
}

/** @brief Function to blink led
 * When this function is called it turns on the led and set a timer to turn off it after a defined period
*/
void blink_led(led_cfg led_cfg_struct)
{
    gpio_set_level(led_cfg_struct.pin, 1);
    ESP_ERROR_CHECK(gptimer_start(led_cfg_struct.timer_handle));
}

/** 
 * @brief Main task for the adc and onset detection module
*/
void onset_adc_task(void *arg)
{    
    /*
    Set up initial values
    */
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[BUFFER_SIZE] = {0};
    /*
    Set up onset channels (kick and snare) with dummy values
    */
    static onset_channel_cfg kick_onset_cfg = {
        .decrease = 20,
        .delta_threshold = 100,
        .delta_x = 50,
        .gate_time_us = 200000,
    };
    static onset_channel_cfg snare_onset_cfg = {
        .decrease = 20,
        .delta_threshold = 100,
        .delta_x = 50,
        .gate_time_us = 200000,
    };
    /*
    Add reference to the struct fields above to menu
    */
    set_menu_item_pointer_to_vrb(MENU_INDEX_KICK_DELTA_X, &kick_onset_cfg.delta_x);
    set_menu_item_pointer_to_vrb(MENU_INDEX_KICK_THRESHOLD, &kick_onset_cfg.delta_threshold);
    set_menu_item_pointer_to_vrb(MENU_INDEX_KICK_GATE, &kick_onset_cfg.gate_time_us);
    set_menu_item_pointer_to_vrb(MENU_INDEX_KICK_FILTER, &kick_onset_cfg.decrease);
    set_menu_item_pointer_to_vrb(MENU_INDEX_SNARE_DELTA_X, &snare_onset_cfg.delta_x);
    set_menu_item_pointer_to_vrb(MENU_INDEX_SNARE_THRESHOLD, &snare_onset_cfg.delta_threshold);
    set_menu_item_pointer_to_vrb(MENU_INDEX_SNARE_GATE, &snare_onset_cfg.gate_time_us);
    set_menu_item_pointer_to_vrb(MENU_INDEX_SNARE_FILTER, &snare_onset_cfg.decrease);

    /*
    Set up runtime onset structs with zero values
    */
    static runtime_onset_values kick = {
        .current_sample = 0,
        .last_onset_time = 0,
        .past_samples_current_onset_index = 0,
        .past_samples = {0},
    };
    static runtime_onset_values snare = {
        .current_sample = 0,
        .last_onset_time = 0,
        .past_samples_current_onset_index = 0,
        .past_samples = {0},
    };

    /*
    Create queue for the onset_adc_task
    */
    onset_adc_task_queue = xQueueCreate(10, sizeof(int));

    while (1)
    {   
        /*
        Block waiting until notify by the continuous adc module
        */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (1)
        {
            int event_code = -1;
            /*
            Check for messages in the queue
            */
            if(xQueueReceive(onset_adc_task_queue, &event_code, 0)){
                switch(event_code){
                    case ONSET_ADC_ALLOW_ONSET:
                        /*
                        Start logging onsets
                        */
                        xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
                        bc.last_relevant_onset_index_for_sync = (bc.most_recent_onset_index + 1) % ONSET_BUFFER_SIZE;
                        bc.there_is_an_onset = false;
                        xSemaphoreGive(bc_mutex_handle);
                        has_onset = false;
                        allow_onset = true;
                        display_gain = false;
                        break;
                    case ONSET_ADC_DISALLOW_ONSET:
                        /*
                        Stop logging onsets
                        */
                        allow_onset = false;
                        display_gain = false;
                        break;
                    case ONSET_ADC_DISALLOW_ONSET_AND_START_SYNC:
                        /*
                        Stop logging onsets
                        */
                        allow_onset = false;
                        display_gain = false;
                        xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
                        bc.there_is_an_onset = has_onset;
                        xSemaphoreGive(bc_mutex_handle);
                        /*
                        Notify sync to start evaluation
                        */
                        xTaskNotify(sync_task_handle, SYNC_START_EVALUATION_NOTIFY, eSetValueWithOverwrite);
                        break;
                    case ONSET_ADC_START_DISPLAY_GAIN:
                        /*
                        Stop logging onsets
                        */
                        display_gain = true;
                        break;
                    case ONSET_ADC_STOP_DISPLAY_GAIN:
                        /*
                        Stop logging onsets
                        */
                        display_gain = false;
                        break;
                    default:
                        ESP_LOGE("adc_task", "invalid queue value!");
                        break;
                }
            };
            /*
            Read the buffer of the ADC
            */
            ret = adc_continuous_read(adc_handle, result, BUFFER_SIZE, &ret_num, 0);
            if (ret == ESP_OK)
            {
                /*
                Divide the buffer in pieces for the oversampling process
                */
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES * OVERSAMPLING * 2)
                {
                    uint16_t sample_avg[2] = {0};
                    for (int j = 0; j < SOC_ADC_DIGI_RESULT_BYTES * OVERSAMPLING * 2; j += SOC_ADC_DIGI_RESULT_BYTES)
                    {
                        /*
                        Sum the sample values to the appropriate variable (kick or snare)
                        */
                        adc_digi_output_data_t *p = (void *)&result[j + i];
                        uint32_t chan_num = ADC_GET_CHANNEL(p);
                        uint16_t data = ADC_GET_DATA(p);
                        switch (chan_num)
                        {
                        case KICK_ADC_CHANNEL:
                            sample_avg[KICK] += data;
                            break;
                        case SNARE_ADC_CHANNEL:
                            sample_avg[SNARE] += data;
                            break;
                        default:
                            ESP_LOGE("ADC", "ERROR CHANNEL: %ld", chan_num);
                            break;
                        }
                    }
                    /*
                    Calculate the average of the samples
                    */
                    sample_avg[KICK] /= OVERSAMPLING;
                    sample_avg[SNARE] /= OVERSAMPLING;
                    /*
                    If asked: display gain
                    */
                    if(display_gain){
                        if(sample_avg[KICK] > GAIN_CLIP_VALUE){
                            gpio_set_level(KICK_LED_PIN, 1);
                        }else{
                            gpio_set_level(KICK_LED_PIN, 0);
                        }
                        if(sample_avg[SNARE] > GAIN_CLIP_VALUE){
                            gpio_set_level(SNARE_LED_PIN, 1);
                        }else{
                            gpio_set_level(SNARE_LED_PIN, 0);
                        }
                    }
                    /*
                    Filter the kick sample
                    */
                    if (sample_avg[KICK] > kick.current_sample)
                    {
                        kick.current_sample = sample_avg[KICK];
                    }
                    else
                    {
                        if (kick.current_sample > kick_onset_cfg.decrease){
                            kick.current_sample -= kick_onset_cfg.decrease;
                        }else{
                            kick.current_sample = 0;
                        }
                    }
                    /*
                    Filter the snare sample
                    */
                    if (sample_avg[SNARE] > snare.current_sample)
                    {
                        snare.current_sample = sample_avg[SNARE];
                    }
                    else
                    {
                        if (snare.current_sample > snare_onset_cfg.decrease){
                            snare.current_sample -= snare_onset_cfg.decrease;
                        }else{
                            snare.current_sample = 0;
                        }
                    }
                    /*
                    Calculate delta y between current sample an previous sample for kick and snare
                    (is the same as calculating the slope, given the filtering above)
                    */
                    uint32_t previous_kick_for_delta = kick.past_samples[(kick.past_samples_current_onset_index + MAX_ONSET_DELTA_X_LENGTH - kick_onset_cfg.delta_x) % MAX_ONSET_DELTA_X_LENGTH];
                    uint32_t previous_snare_for_delta = snare.past_samples[(snare.past_samples_current_onset_index + MAX_ONSET_DELTA_X_LENGTH - snare_onset_cfg.delta_x) % MAX_ONSET_DELTA_X_LENGTH];
                    int delta_y_kick = kick.current_sample - previous_kick_for_delta;
                    int delta_y_snare = snare.current_sample - previous_snare_for_delta;
                    uint64_t current_time_us = esp_timer_get_time();
                    /*
                    Check for onset for kick
                    */
                    if (delta_y_kick > kick_onset_cfg.delta_threshold)
                    {
                        /*
                        Debounce onset
                        */
                        if (current_time_us > kick.last_onset_time + kick_onset_cfg.gate_time_us)
                        {
                            if(allow_onset){
                                /*
                                Log onset (if allowed)
                                */
                                xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
                                bc.most_recent_onset_index = (bc.most_recent_onset_index + 1) % ONSET_BUFFER_SIZE;
                                uint32_t current_onset_index = bc.most_recent_onset_index;
                                xSemaphoreGive(bc_mutex_handle);
                                onsets[current_onset_index].time = current_time_us;
                                onsets[current_onset_index].type = KICK;
                            }
                            kick.last_onset_time = current_time_us;
                            has_onset = true;
                            /*
                            Blink led
                            */
                            blink_led(onset_led[KICK]);
                        }
                    }
                    /*
                    Check for onset for snare
                    */
                    if (delta_y_snare > snare_onset_cfg.delta_threshold)
                    {
                        /*
                        Debounce onset
                        */
                        if (current_time_us > snare.last_onset_time + snare_onset_cfg.gate_time_us)
                        {
                            if(allow_onset){
                                /*
                                Log onset (if allowed)
                                */
                                xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
                                bc.most_recent_onset_index = (bc.most_recent_onset_index + 1) % ONSET_BUFFER_SIZE;
                                uint32_t current_onset_index = bc.most_recent_onset_index;
                                xSemaphoreGive(bc_mutex_handle);
                                onsets[current_onset_index].time = current_time_us;
                                onsets[current_onset_index].type = SNARE;
                            }
                            snare.last_onset_time = current_time_us;
                            has_onset = true;
                            /*
                            Blink led
                            */
                            blink_led(onset_led[SNARE]);
                        }
                    }
                    /*
                    Update past sample index and update its index
                    */
                    kick.past_samples_current_onset_index = (kick.past_samples_current_onset_index + 1) % MAX_ONSET_DELTA_X_LENGTH;
                    kick.past_samples[kick.past_samples_current_onset_index] = kick.current_sample;
                    snare.past_samples_current_onset_index = (snare.past_samples_current_onset_index + 1) % MAX_ONSET_DELTA_X_LENGTH;
                    snare.past_samples[snare.past_samples_current_onset_index] = snare.current_sample;
                }
                vTaskDelay(1);
            }else if (ret == ESP_ERR_INVALID_STATE){
                ESP_LOGE("Sync_task", "ERR INVALID STATE: adc rate is faster than processing rate");
                break;
            }
            else if (ret == ESP_ERR_TIMEOUT)
            {
                /*
                There's no available data because API returns timeout when we were reading buffer
                */
                break;
            }
        }
    }
}

void onset_adc_init()
{
    /*
    Init led blinking proc
    */
    led_blink_init();
    /*
    Initialize the continuous adc module
    */
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &adc_handle);
    /*
    Create the onset_adc_task
    */
    xTaskCreate(onset_adc_task, "Onset_Adc_Task", ONSET_ADC_TASK_STACK_SIZE, NULL, ONSET_ADC_TASK_PRIORITY, &onset_adc_task_handle);
}