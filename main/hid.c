#include "hid.h"
#include "sync.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/pulse_cnt.h"
#include "../components/ssd1306/ssd1306.h"
#include "../components/ssd1306/font8x8_basic.h"

extern main_runtime_vrbs bc;
extern main_mode mode;
extern bool store_values;
extern hid_parameter_entry menu_item[MENU_ITEM_INDEX_LENGTH];

hid_parameter_entry menu_item[MENU_ITEM_INDEX_LENGTH];
QueueHandle_t hid_task_queue = NULL;
TaskHandle_t hid_task_handle = NULL;
SSD1306_t oled_screen;
nvs_handle_t bc_nvs_handle;
bool store_values = false;

/**
 * @brief Hex representation of big numbers (4x) for the SSD1306
*/
const uint8_t BIG_NUMBERS[11][4][24] = {{{0x00, 0x00, 0xc0, 0xf0, 0xf8, 0xfc, 0x3e, 0x1e, 0x0f, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x0f, 0x1e, 0x3e, 0xfc, 0xf8, 0xf0, 0xc0, 0x00, 0x00, 0x00}, {0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xff, 0xff, 0xfe, 0x00, 0x00}, {0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xff, 0xff, 0x3f, 0x00, 0x00}, {0x00, 0x00, 0x03, 0x0f, 0x1f, 0x3f, 0x7e, 0x78, 0x70, 0xf0, 0xf0, 0xe0, 0xf0, 0xf0, 0x70, 0x78, 0x7c, 0x3f, 0x1f, 0x0f, 0x03, 0x00, 0x00, 0x00}}, {{0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xf0, 0xf0, 0x70, 0x78, 0x78, 0x3c, 0x3c, 0x3c, 0xfe, 0xfe, 0xfe, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f, 0x3f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, {{0x00, 0x00, 0xc0, 0xf0, 0xf8, 0xfc, 0x7e, 0x1e, 0x0f, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x0f, 0x0f, 0x3e, 0xfe, 0xfc, 0xf8, 0xf0, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x01, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xf0, 0xff, 0xff, 0x3f, 0x1f, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0x7e, 0x3f, 0x0f, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x78, 0x7c, 0x7e, 0x7f, 0x7f, 0x77, 0x73, 0x71, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x00}}, {{0x00, 0x00, 0x00, 0xe0, 0xf8, 0xfc, 0xfe, 0x1e, 0x0f, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x0f, 0x0f, 0x1e, 0xfe, 0xfc, 0xf8, 0xf0, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xe0, 0xf0, 0xfe, 0x3f, 0x3f, 0x0f, 0x01, 0x00, 0x00}, {0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x07, 0x0f, 0xff, 0xfe, 0xfc, 0xf0, 0x00, 0x00}, {0x00, 0x00, 0x01, 0x0f, 0x1f, 0x3f, 0x3e, 0x78, 0x78, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x70, 0x78, 0x7e, 0x3f, 0x1f, 0x0f, 0x03, 0x00, 0x00}}, {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xc0, 0xf0, 0xf8, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xe0, 0xf0, 0xfc, 0x7e, 0x1f, 0x0f, 0x03, 0x01, 0x00, 0x87, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0xe0, 0xf0, 0xfc, 0xff, 0xff, 0xe7, 0xe3, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xe0, 0xe0, 0xe0, 0xe0, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00}}, {{0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0x3f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xf0, 0x70, 0x78, 0x78, 0x78, 0x78, 0x78, 0xf8, 0xf0, 0xf0, 0xe0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x80, 0x80, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x07, 0x0f, 0x1f, 0x3f, 0x7c, 0x78, 0x70, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x70, 0x78, 0x7e, 0x3f, 0x1f, 0x0f, 0x03, 0x00, 0x00, 0x00}}, {{0x00, 0x00, 0x00, 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0x7c, 0x3c, 0x1e, 0x1e, 0x1e, 0x0e, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0xe7, 0xe1, 0x70, 0x70, 0x78, 0x38, 0x38, 0x78, 0x78, 0xf8, 0xf0, 0xf0, 0xe0, 0xc0, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x07, 0x0f, 0x1f, 0x3f, 0x7c, 0x78, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x78, 0x7c, 0x3f, 0x1f, 0x0f, 0x03, 0x00, 0x00, 0x00}}, {{0x00, 0x06, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x8f, 0xef, 0xff, 0xff, 0x7f, 0x1f, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xf0, 0xfe, 0xff, 0x3f, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xf0, 0xfc, 0xff, 0x7f, 0x0f, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x78, 0x7e, 0x7f, 0x1f, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, {{0x00, 0x00, 0x00, 0xe0, 0xf8, 0xfc, 0xfe, 0x3e, 0x0f, 0x0f, 0x07, 0x07, 0x07, 0x0f, 0x0f, 0x1e, 0x3e, 0xfc, 0xfc, 0xf8, 0xe0, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x0f, 0x1f, 0x3f, 0xff, 0xf8, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0xe0, 0xe0, 0xf0, 0xf8, 0x7f, 0x3f, 0x1f, 0x07, 0x00, 0x00, 0x00}, {0x00, 0x00, 0xf0, 0xfc, 0xfe, 0xff, 0x0f, 0x07, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x07, 0x1f, 0xfe, 0xfe, 0xf8, 0xe0, 0x00, 0x00}, {0x00, 0x00, 0x03, 0x0f, 0x1f, 0x3f, 0x7e, 0x78, 0x70, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x78, 0x7c, 0x3e, 0x3f, 0x1f, 0x0f, 0x01, 0x00, 0x00}}, {{0x00, 0x00, 0x00, 0xe0, 0xf0, 0xfc, 0xfc, 0x3e, 0x1e, 0x0f, 0x0f, 0x07, 0x07, 0x0f, 0x0f, 0x1f, 0x3e, 0xfc, 0xfc, 0xf0, 0xe0, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x0c, 0xff, 0xff, 0xff, 0xf1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x03, 0x07, 0x0f, 0x0f, 0x1f, 0x1e, 0x1e, 0x1c, 0x1c, 0x1e, 0x0e, 0x0f, 0x07, 0xe7, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x70, 0x70, 0x70, 0x78, 0x78, 0x38, 0x3c, 0x3e, 0x1f, 0x0f, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00}}, {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}};

/*
Set pointer of the variable in the selected struct for the menu
*/
void set_menu_item_pointer_to_vrb(menu_item_index index, void *ptr)
{
    if (ptr == NULL){
        ESP_LOGE("HID","POINTER IS NULL!");
    }else{
        menu_item[index].pointer_to_vrb = ptr;
    }
}

/**
 * @brief Gets the variable value in percentage
*/
static uint8_t get_variable_perc_value(hid_parameter_entry *variable)
{
    return variable->percentage;
}

/**
 * @brief Sets the selected variable to the given value
*/
static void set_variable_value(hid_parameter_entry *variable, uint8_t percentage_value)
{
    if(variable->pointer_to_vrb == NULL){
        ESP_LOGE("set_variable_value", "VARIABLE: %s has a null pointer!", variable->name_displayed);
        return;
    }
    /*
    Set the percentage value in the struct
    */
    variable->percentage = percentage_value;
    ESP_LOGI("set_variable_value", "VARIABLE: %s", variable->name_displayed);
    ESP_LOGI("set_variable_value", "variable.percentage set to %d", percentage_value);
    ESP_LOGI("set_variable_value", "variable.type is %d", variable->vrb_type);
    /*
    Convert the percentage value to the correct data to be stored given the vrb_type attribute
    */
    switch (variable->vrb_type)
    {
    case BC_UINT64:
        float return_64 = ((float)(variable->max.u64 - variable->min.u64) / 100);
        return_64 = return_64 * (float)percentage_value;
        return_64 = return_64 + variable->min.u64;
        *(uint64_t *)variable->pointer_to_vrb = (uint64_t)return_64;
        ESP_LOGI("OLED", "variable_value set to %lld", (uint64_t)return_64);
        break;
    case BC_UINT16:
        float return_16 = ((float)(variable->max.u16 - variable->min.u16) / 100);
        return_16 = return_16 * (float)percentage_value;
        return_16 = return_16 + variable->min.u16;
        *(uint16_t *)variable->pointer_to_vrb = (uint16_t)return_16;
        ESP_LOGI("OLED", "variable_value set to %d", (uint16_t)return_16);
        break;
    case BC_FLOAT:
        float return_f = ((float)(variable->max.f - variable->min.f) / 100);
        return_f = return_f * (float)percentage_value;
        return_f = return_f + variable->min.f;
        *(float *)variable->pointer_to_vrb = return_f;
        ESP_LOGI("OLED", "variable_value set to %f", return_f);
        break;
    case BC_YESNO:
        if (percentage_value > 0)
        {
            *(bool *)variable->pointer_to_vrb = 1;
            ESP_LOGI("OLED", "variaboole_value set to 1");
        }
        else
        {
            *(bool *)variable->pointer_to_vrb = 0;
            ESP_LOGI("OLED", "variaboole_value set to 0");
        }
        break;
    default:
        ESP_LOGE("OLED", "ERROR in convertin value");
        break;
    }
}

/*
Loads stored values from the NVS partition
*/
static void bc_menu_nvs_load(){
    /*
    Check NVS status
    */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /*
        NVS partition was truncated and needs to be erased
        Retry nvs_flash_init
        */
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    err = nvs_open("storage", NVS_READWRITE, &bc_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI("hid","Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        /*
        If the NVS partition is OK read values from it
        */
        for(int i=0; i<MENU_ITEM_INDEX_LENGTH - 1; i++){ // avoid last item (save values)
            uint8_t perc = menu_item[i].percentage;
            ESP_LOGI("hid","Now working on %s",menu_item[i].name_displayed);
            ESP_LOGI("hid","Current value is: %d",perc);
            err = nvs_get_u8(bc_nvs_handle, menu_item[i].name_displayed, &menu_item[i].percentage);
            switch (err) {
                case ESP_OK:
                    ESP_LOGI("hid","Found stored value of: %d", menu_item[i].percentage);
                    break;
                case ESP_ERR_NVS_NOT_FOUND:
                    /*
                    If the current name has not been initialized than set it to 50% value
                    */
                    ESP_LOGI("hid","The value is not initialized yet!");
                    menu_item[i].percentage = 50;
                    break;
                default :
                    ESP_LOGI("hid","Error (%s) reading!\n", esp_err_to_name(err));
            }
        }
        nvs_close(bc_nvs_handle);
        /*
        Set the "save value" option to false
        */
        menu_item[MENU_INDEX_SAVE_VALUES].percentage = 0;
        store_values = 0;
    }
}

/**
 * @brief Initialize the menu with all the entries
 * For each entry it sets the value but the pointer is set to NULL
 * (must be set from outside with the given function)
 * Whenever a new variable is added to the menu, this function must be updated
*/
static void menu_init()
{
    menu_item_index index;

    /* MENU_INDEX_TEMPO_ALPHA */
    index = MENU_INDEX_TEMPO_ALPHA;
    strcpy(menu_item[index].name_displayed, ALPHA_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_FLOAT;
    menu_item[index].min.f = ALPHA_MIN_VALUE;
    menu_item[index].max.f = ALPHA_MAX_VALUE;
    menu_item[index].percentage = ALPHA_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = ALPHA_PERCENTAGE_STEP;

    /* MENU_INDEX_TEMPO_LATENCY_AMOUNT */
    index = MENU_INDEX_TEMPO_LATENCY_AMOUNT;
    strcpy(menu_item[index].name_displayed, LATENCY_AMOUNT_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_UINT16;
    menu_item[index].min.u16 = LATENCY_AMOUNT_MIN_VALUE;
    menu_item[index].max.u16 = LATENCY_AMOUNT_MAX_VALUE;
    menu_item[index].percentage = LATENCY_AMOUNT_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = LATENCY_AMOUNT_PERCENTAGE_STEP;

    /* MENU_INDEX_SYNC_BETA */
    index = MENU_INDEX_SYNC_BETA;
    strcpy(menu_item[index].name_displayed, BETA_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_FLOAT;
    menu_item[index].min.f = BETA_MIN_VALUE;
    menu_item[index].max.f = BETA_MAX_VALUE;
    menu_item[index].percentage = BETA_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = BETA_PERCENTAGE_STEP;

    /* MENU_INDEX_SYNC_NARROW */
    index = MENU_INDEX_SYNC_NARROW;
    strcpy(menu_item[index].name_displayed, NARROW_RATIO_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_FLOAT;
    menu_item[index].min.f = NARROW_RATIO_MIN_VALUE;
    menu_item[index].max.f = NARROW_RATIO_MAX_VALUE;
    menu_item[index].percentage = NARROW_RATIO_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = NARROW_RATIO_PERCENTAGE_STEP;

    /* MENU_INDEX_SYNC_EXPAND */
    index = MENU_INDEX_SYNC_EXPAND;
    strcpy(menu_item[index].name_displayed, EXPAND_RATIO_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_FLOAT;
    menu_item[index].min.f = EXPAND_RATIO_MIN_VALUE;
    menu_item[index].max.f = EXPAND_RATIO_MAX_VALUE;
    menu_item[index].percentage = EXPAND_RATIO_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = EXPAND_RATIO_PERCENTAGE_STEP;

    /* MENU_INDEX_KICK_THRESHOLD */
    index = MENU_INDEX_KICK_THRESHOLD;
    strcpy(menu_item[index].name_displayed,KICK_THRESHOLD_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_UINT16;
    menu_item[index].min.u16 = KICK_THRESHOLD_MIN_VALUE;
    menu_item[index].max.u16 = KICK_THRESHOLD_MAX_VALUE;
    menu_item[index].percentage = KICK_THRESHOLD_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = KICK_THRESHOLD_PERCENTAGE_STEP;

    /* MENU_INDEX_KICK_GATE */
    index = MENU_INDEX_KICK_GATE;
    strcpy(menu_item[index].name_displayed,KICK_GATE_TIMER_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_UINT64;
    menu_item[index].min.u64 = KICK_GATE_TIMER_MIN_VALUE;
    menu_item[index].max.u64 = KICK_GATE_TIMER_MAX_VALUE;
    menu_item[index].percentage = KICK_GATE_TIMER_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = KICK_GATE_TIMER_PERCENTAGE_STEP;

    /* MENU_INDEX_KICK_FILTER */
    index = MENU_INDEX_KICK_FILTER;
    strcpy(menu_item[index].name_displayed, KICK_LOW_PASS_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_UINT16;
    menu_item[index].min.u16 = KICK_LOW_PASS_MIN_VALUE;
    menu_item[index].max.u16 = KICK_LOW_PASS_MAX_VALUE;
    menu_item[index].percentage = KICK_LOW_PASS_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = KICK_LOW_PASS_PERCENTAGE_STEP;

    /* MENU_INDEX_KICK_DELTA_X */
    index = MENU_INDEX_KICK_DELTA_X;
    strcpy(menu_item[index].name_displayed, KICK_DELTA_X_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_UINT16;
    menu_item[index].min.u16 = KICK_DELTA_X_MIN_VALUE;
    menu_item[index].max.u16 = KICK_DELTA_X_MAX_VALUE;
    menu_item[index].percentage = KICK_DELTA_X_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = KICK_DELTA_X_PERCENTAGE_STEP;

    /* MENU_INDEX_SNARE_THRESHOLD */
    index = MENU_INDEX_SNARE_THRESHOLD;
    strcpy(menu_item[index].name_displayed,SNARE_THRESHOLD_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_UINT16;
    menu_item[index].min.u16 = SNARE_THRESHOLD_MIN_VALUE;
    menu_item[index].max.u16 = SNARE_THRESHOLD_MAX_VALUE;
    menu_item[index].percentage = SNARE_THRESHOLD_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = SNARE_THRESHOLD_PERCENTAGE_STEP;

    /* MENU_INDEX_SNARE_GATE */
    index = MENU_INDEX_SNARE_GATE;
    strcpy(menu_item[index].name_displayed,SNARE_GATE_TIMER_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_UINT64;
    menu_item[index].min.u64 = SNARE_GATE_TIMER_MIN_VALUE;
    menu_item[index].max.u64 = SNARE_GATE_TIMER_MAX_VALUE;
    menu_item[index].percentage = SNARE_GATE_TIMER_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = SNARE_GATE_TIMER_PERCENTAGE_STEP;

    /* MENU_INDEX_SNARE_FILTER */
    index = MENU_INDEX_SNARE_FILTER;
    strcpy(menu_item[index].name_displayed,SNARE_LOW_PASS_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_UINT16;
    menu_item[index].min.u16 = SNARE_LOW_PASS_MIN_VALUE;
    menu_item[index].max.u16 = SNARE_LOW_PASS_MAX_VALUE;
    menu_item[index].percentage = SNARE_LOW_PASS_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = SNARE_LOW_PASS_PERCENTAGE_STEP;

    /* MENU_INDEX_SNARE_DELTA_X */
    index = MENU_INDEX_SNARE_DELTA_X;
    strcpy(menu_item[index].name_displayed, SNARE_DELTA_X_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = NULL;
    menu_item[index].vrb_type = BC_UINT16;
    menu_item[index].min.u16 = SNARE_DELTA_X_MIN_VALUE;
    menu_item[index].max.u16 = SNARE_DELTA_X_MAX_VALUE;
    menu_item[index].percentage = SNARE_DELTA_X_DEFAULT_PERCENTAGE;
    menu_item[index].percentage_step = SNARE_DELTA_X_PERCENTAGE_STEP;

    /* MENU_INDEX_SAVE_VALUES */
    index = MENU_INDEX_SAVE_VALUES;
    strcpy(menu_item[index].name_displayed, SAVE_VALUES_PARAMETER_NAME);
    menu_item[index].pointer_to_vrb = &store_values;
    menu_item[index].vrb_type = BC_YESNO;
    menu_item[index].min.b = 0;
    menu_item[index].max.b = 1;
    menu_item[index].percentage = 0;
    menu_item[index].percentage_step = 100;

    /*
    Load saved values from the persistent storage
    */
    bc_menu_nvs_load();
}

/**
 * @brief Saves all the current values of the menu variables into the permanent storage
*/
static void bc_menu_nvs_write(){
    /*
    Check NVS status
    */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /*
        NVS partition was truncated and needs to be erased
        Retry nvs_flash_init
        */
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    err = nvs_open("storage", NVS_READWRITE, &bc_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGI("hidnvs","Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        /*
        If NVS partition is OK save values to it
        */
        for(int i=0; i<MENU_ITEM_INDEX_LENGTH - 1; i++){ // avoid last item (save values)
            uint8_t perc = menu_item[i].percentage;
            ESP_LOGI("hid","------");
            ESP_LOGI("hid","Now working on %s",menu_item[i].name_displayed);
            ESP_LOGI("hid","Current value is: %d",perc);
            err = nvs_set_u8(bc_nvs_handle, menu_item[i].name_displayed, menu_item[i].percentage);
            ESP_LOGI("hid","%s",(err != ESP_OK) ? "Failed!\n" : "Done\n");
            ESP_LOGI("hid","Committing updates in NVS ... ");
            err = nvs_commit(bc_nvs_handle);
            ESP_LOGI("hid","%s",(err != ESP_OK) ? "Failed!\n" : "Done\n");
        }
        nvs_close(bc_nvs_handle);
    }
}

/**
 * @brief Callback function for the encoder click
 * This function does debouncing on the click and notify the task
*/
static void encoder_has_clicked(void *args)
{
    /*
    Debounce
    */
    static uint64_t previous_encoder_click_debounce = 0;
    uint64_t current_time_us = esp_timer_get_time();
    if (current_time_us > previous_encoder_click_debounce + ENCODER_CLICK_DEBOUNCE_TIME_US)
    {
        /*
        Send message to the hid task queue about encoder click
        */
        int click_value = ENCODER_CLICK;
        xQueueSendFromISR(hid_task_queue, &click_value, NULL);
        previous_encoder_click_debounce = current_time_us;
    }
}

/**
 * @brief Callback function for the encoder rotation
 * This function sends message to the hid_task queue about the encoder moving. It tells the moving steps (+4/-4).
*/
static bool encoder_has_moved(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    xQueueSendFromISR(hid_task_queue, &(edata->watch_point_value), NULL);
    return (pdFALSE);
}

/**
 * @brief Encoder and queue init function
 * This function sets up the encoder using the ESP-IDF pulse counter module.
 * It also creates the queue of the task (needed here to set the callback function for the encoder rotation)
*/
static void encoder_and_queue_init()
{
    /*
    Set up GPIO
    */
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = ENCODER_GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    gpio_isr_handler_add(ENCODER_CLICK_PIN, encoder_has_clicked, (void *)ENCODER_CLICK_PIN);
    /*
    Set up Pulse Counter limits at +4/-4
    (When the encoder reaches this points, the module resets to zero)
    */
    pcnt_unit_config_t unit_config = {
        .high_limit = ENCODER_INCREASE,
        .low_limit = ENCODER_DECREASE,
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));
    /*
    Set up glitch filter
    */
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));
    /*
    Set up the two channels of the encoder
    */
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = ENCODER_PIN_0,
        .level_gpio_num = ENCODER_PIN_1,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = ENCODER_PIN_1,
        .level_gpio_num = ENCODER_PIN_0,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    /*
    Set up watchpoints
    */
    int watch_points[] = {ENCODER_DECREASE, ENCODER_INCREASE};
    for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++)
    {
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]));
    }
    /*
    Set up callback function
    */
    pcnt_event_callbacks_t cbs = {
        .on_reach = encoder_has_moved,
    };
    /*
    Create queue for the hid_task
    */
    hid_task_queue = xQueueCreate(10, sizeof(int));
    /*
    Attach callback function to the pulse counter unit
    */
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, hid_task_queue));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}

/**
 * @brief Displays the parameter on the OLED
 * This function displays the text for the parameter and creates a horizontal bar for displaying the percentage
*/
static void display_parameter_value(SSD1306_t *dev, char *text1, char *text2, uint8_t percentage, bool is_yesno)
{
    /*
    Check if percentage value is ok
    */
    if (percentage > 100)
    {
        ESP_LOGE("OLED", "ERROR here Parameter %s", text2);
        ESP_LOGE("OLED", "Request to print a wrong percentage of %d", percentage);
        return;
    }
    /*
    Check if text value is of the right length
    */
    char full_text[16] = {0};
    uint16_t text_length = strlen(text1);
    memset(full_text, 0x00, strlen(full_text));
    memcpy(full_text, text1, text_length);
    ssd1306_display_text(dev, 0, full_text, 16, false);
    text_length = strlen(text2);
    memset(full_text, 0x00, strlen(full_text));
    memcpy(full_text, text2, text_length);
    ssd1306_display_text(dev, 1, full_text, 16, false);
    /*
    Create the image to be printed on the display
    */
    uint8_t image[128];
    /*
    Decompose the percentage in the corresponding digits
    */
    char percentage_digits[3] = {0};
    if (is_yesno)
    {
        percentage = (percentage > 0) ? 100 : 0;
        percentage_digits[2] = (percentage > 0) ? 'S' : 'O';
        percentage_digits[1] = (percentage > 0) ? 'E' : 'N';
        percentage_digits[0] = (percentage > 0) ? 'Y' : ' ';
    }
    else
    {
        percentage_digits[2] = (percentage % 10) + 48;
        percentage_digits[1] = ((percentage / 10) % 10) + 48;
        percentage_digits[0] = ((percentage / 100) % 10) + 48;
        /*
        Add space instead of zeros at start of number
        */
        if (percentage_digits[0] == 48 && percentage_digits[1] == 48)
        {
            percentage_digits[0] = 20;
            percentage_digits[1] = 20;
        }
        else if (percentage_digits[0] == 48)
        {
            percentage_digits[0] = 20;
        }
    }
    /*
    Print the image on the display
    */
    memset(image, 0xF0, percentage);
    memset(&image[percentage], 0x00, 127 - percentage);
    ssd1306_display_image(dev, 2, 0, image, 127);
    memset(image, 0xFF, percentage);
    memset(&image[percentage], 0x00, 127 - percentage);
    memcpy(&image[102], font8x8_basic_tr[(uint8_t)percentage_digits[0]], 8);
    memcpy(&image[111], font8x8_basic_tr[(uint8_t)percentage_digits[1]], 8);
    memcpy(&image[119], font8x8_basic_tr[(uint8_t)percentage_digits[2]], 8);
    ssd1306_display_image(dev, 3, 0, image, 127);
}

/**
 * @brief Displays the value using big numbers
*/
static void display_big_numbers(SSD1306_t *dev, uint16_t value)
{
    if (value > 999)
    {
        ESP_LOGE("OLED", "ERROR asked to print a bpm of %d", value);
        return;
    }
    /*
    Create image to be printed on the display
    */
    uint8_t image[128];
    /*
    Decompose the value into corresponding digits
    */
    uint8_t value_digits[3];
    value_digits[2] = value % 10;
    value_digits[1] = (value / 10) % 10;
    value_digits[0] = (value / 100) % 10;
    /*
    Add space instead of zeros at start of number
    */
    if (value_digits[0] == 0 && value_digits[1] == 0)
    {
        value_digits[0] = 10;
        value_digits[1] = 10;
    }
    else if (value_digits[0] == 0)
    {
        value_digits[0] = 10;
    }
    /*
    Print the image on the display
    */
    for (int i = 0; i < 4; i++)
    {
        memset(image, 0x00, 128);
        memcpy(&image[0], BIG_NUMBERS[value_digits[0]][i], 24);
        memcpy(&image[24], BIG_NUMBERS[value_digits[1]][i], 24);
        memcpy(&image[48], BIG_NUMBERS[value_digits[2]][i], 24);
        ssd1306_display_image(dev, i, 0, image, 127);
    }
}

/**
 * @brief Init function for the SSD1306 display (I2C)
*/
static void oled_init(){
    /*
    Set up I2C port
    */
    i2c_master_init(&oled_screen, OLED_SDA_GPIO_PIN, OLED_SCL_GPIO_PIN,OLED_RESET_GPIO_PIN);
    /*
    Send to the display the setup messages
    */
    ssd1306_init(&oled_screen, 128, 32);
    ssd1306_clear_screen(&oled_screen, false);
    ssd1306_contrast(&oled_screen, 0xff);
}

/**
 * @brief Main task for the Hid module
*/
static void hid_task(void *arg)
{
    static uint8_t menu_index = 0;
    uint8_t tap_hits_counter = 0;
    while (1)
    {
        uint8_t percentage_value = 0;
        bool has_changed = false;
        int event_code;
        /*
        Checks for messages in the queue
        */
        while (xQueueReceive(hid_task_queue, &event_code, 1))
        {
            /*
            If there are messages...
            */
            percentage_value = get_variable_perc_value(&menu_item[menu_index]);
            switch (event_code)
            {
            case ENCODER_INCREASE:
                /*
                The encoder increases:
                Increase the value of the selected parameter (only if in SETTINGS mode)
                */
                if (mode == MODE_SETTINGS)
                {
                    percentage_value = percentage_value + menu_item[menu_index].percentage_step;
                    if(percentage_value > 100){
                        percentage_value = 100;
                    }
                    set_variable_value(&menu_item[menu_index],percentage_value);
                }
                break;
            case ENCODER_DECREASE:
                /*
                The encoder decreases:
                Decrease the value of the selected parameter (only if in SETTINGS mode)
                */
                if (mode == MODE_SETTINGS)
                {
                    if(percentage_value > menu_item[menu_index].percentage_step){
                        percentage_value = percentage_value - menu_item[menu_index].percentage_step;
                    }else{
                        percentage_value = 0;
                    }
                    set_variable_value(&menu_item[menu_index],percentage_value);
                }
                break;
            case ENCODER_CLICK:
                /*
                The encoder has clicked:
                Change the selected parameter (only if in SETTINGS mode)
                */
                if (mode == MODE_SETTINGS)
                {
                    /*
                    Check if previous value was last of menu -> save values
                    */
                    if (menu_index == MENU_ITEM_INDEX_LENGTH - 1 && store_values)
                    {
                        menu_item[MENU_INDEX_SAVE_VALUES].percentage = 0;
                        store_values = 0;
                        bc_menu_nvs_write();
                    }
                    /*
                    Increase menu index
                    */
                    menu_index = (menu_index + 1) % (sizeof(menu_item) / sizeof(hid_parameter_entry));
                    percentage_value = get_variable_perc_value(&menu_item[menu_index]);
                }
                break;
            case HID_SETTINGS_MODE_SELECT: 
                /*
                Switch to Settings mode:
                Turn on display (it was sleeping) and prepare data to show
                */
                turn_on_oled(&oled_screen);
                menu_index = 0;
                percentage_value = get_variable_perc_value(&menu_item[menu_index]);
                break;
            case HID_TAP_MODE_SELECT:
                /*
                Switch to Tap mode:
                Turn on display (it was sleeping) and prepare data to show
                */
                turn_on_oled(&oled_screen);
                tap_hits_counter = 0;
                break;
            case HID_PLAY_MODE_SELECT: 
                /*
                Switch to bpm mode
                */
                break;
            case HID_ENTER_SLEEP_MODE: 
                /*
                Switch to Sleep mode:
                Turn off display
                */
                turn_off_oled(&oled_screen);
                break;
            case HID_TAP_0:
                tap_hits_counter = 0;
                break;
            case HID_TAP_1:
                tap_hits_counter = 1;
                break;
            case HID_TAP_2:
                tap_hits_counter = 2;
                break;
            case HID_TAP_3:
                tap_hits_counter = 3;
                break;
            case HID_TAP_4:
                tap_hits_counter = 4;
                break;
            default:
                ESP_LOGE("hid_task", "ERROR: event code invalid");
                break;
            }
            /*
            Set has_changed to true to avoid printing the same thing every time the task repeats itself
            */
            has_changed = true;
        }
        switch (mode)
        {
        case MODE_TAP:
            /*
            Displays the current tap hit received
            */
            if (has_changed){
                display_big_numbers(&oled_screen, tap_hits_counter);
                has_changed = false;
            }
            break;
        case MODE_PLAY:
            /*
            Calculates and display bpm
            */
            uint8_t bpm = 30000 / bc.tau;
            display_big_numbers(&oled_screen, bpm);
            break;
        case MODE_SETTINGS:
            if (has_changed)
            {
                /*
                Display the current selected parameter name and value (with bar and numbers)
                */
                display_parameter_value(&oled_screen, "MENU", menu_item[menu_index].name_displayed, percentage_value, menu_item[menu_index].vrb_type == BC_YESNO);
                has_changed = false;
            }
            break;
        case MODE_SLEEP:
            break;
        default:
            ESP_LOGE("hid_task", "ERROR: current_mode invalid");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*
This function assign the percentage value (loaded from the NVS) to the variable for every entry in the menu
*/
void hid_set_up_values(){
    for(int i=0; i<MENU_ITEM_INDEX_LENGTH - 1; i++){
        set_variable_value(&menu_item[i],menu_item[i].percentage);
        ESP_LOGI("Hid_Task","Variable %s loaded from storage",menu_item[i].name_displayed);
    }

}

/*
Init function:
Initialize encoder
Creates queue
Initialize OLED with I2C
Initialize menu
Creates the hid_task
*/
void hid_init(){
    encoder_and_queue_init();
    oled_init();
    menu_init();
    xTaskCreate(hid_task, "Hid_Task", HID_TASK_STACK_SIZE, NULL, HID_TASK_PRIORITY, &hid_task_handle);
}