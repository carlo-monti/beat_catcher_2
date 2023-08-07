/**
 * @file hid.h
 * @brief HID Module handles the encoder, the display and the permanent storage of the variables values.
 * The HID Module allow the user to edit the parameters of the system modifying the value of a variable,
 * it permanently stores the modified value and displays some information.
 * 
 * It has three modes of working.
 * - SETTINGS mode: the module shows the name and the value of the currently selected parameter.
 *  - When the encoder rotates the value is increased/decreased.
 *  - When the encoder is clicked, the next parameter is selected.
 *  - The last parameter allow to permanently save values
 * - BPM mode: shows the current bpm value
 * - TAP mode: shows the number of the hits received (0 to 4)
 * 
 * 
 */

#ifndef BC_HID_H
#define BC_HID_H

#include "main_defs.h"

/* MENU SETTINGS */
#define ALPHA_PARAMETER_NAME "TEMPO: alpha   "
#define ALPHA_MIN_VALUE 0.1
#define ALPHA_MAX_VALUE 2
#define ALPHA_DEFAULT_PERCENTAGE 50
#define ALPHA_PERCENTAGE_STEP 1

#define LATENCY_AMOUNT_PARAMETER_NAME "TEMPO: amount  "
#define LATENCY_AMOUNT_MIN_VALUE 0
#define LATENCY_AMOUNT_MAX_VALUE 10
#define LATENCY_AMOUNT_DEFAULT_PERCENTAGE 50
#define LATENCY_AMOUNT_PERCENTAGE_STEP 10

#define LATENCY_SMOOTH_PARAMETER_NAME "TEMPO: smooth  "
#define LATENCY_SMOOTH_MIN_VALUE 0
#define LATENCY_SMOOTH_MAX_VALUE 10
#define LATENCY_SMOOTH_DEFAULT_PERCENTAGE 50
#define LATENCY_SMOOTH_PERCENTAGE_STEP 10

#define NARROW_RATIO_PARAMETER_NAME "SYNC: narrow r "
#define NARROW_RATIO_MIN_VALUE 0.1
#define NARROW_RATIO_MAX_VALUE 2
#define NARROW_RATIO_DEFAULT_PERCENTAGE 50
#define NARROW_RATIO_PERCENTAGE_STEP 1

#define EXPAND_RATIO_PARAMETER_NAME "SYNC: expand r "
#define EXPAND_RATIO_MIN_VALUE 0.1
#define EXPAND_RATIO_MAX_VALUE 2
#define EXPAND_RATIO_DEFAULT_PERCENTAGE 50
#define EXPAND_RATIO_PERCENTAGE_STEP 1

#define BETA_PARAMETER_NAME "SYNC: beta     "
#define BETA_MIN_VALUE 0.1
#define BETA_MAX_VALUE 1
#define BETA_DEFAULT_PERCENTAGE 50
#define BETA_PERCENTAGE_STEP 1

#define KICK_LOW_PASS_PARAMETER_NAME "KICK: filter   "
#define KICK_LOW_PASS_MIN_VALUE 1
#define KICK_LOW_PASS_MAX_VALUE 100
#define KICK_LOW_PASS_DEFAULT_PERCENTAGE 50
#define KICK_LOW_PASS_PERCENTAGE_STEP 1

#define KICK_THRESHOLD_PARAMETER_NAME "KICK: thresh   "
#define KICK_THRESHOLD_MIN_VALUE 0
#define KICK_THRESHOLD_MAX_VALUE 4096
#define KICK_THRESHOLD_DEFAULT_PERCENTAGE 50
#define KICK_THRESHOLD_PERCENTAGE_STEP 1

#define KICK_GATE_TIMER_PARAMETER_NAME "KICK: gate     "
#define KICK_GATE_TIMER_MIN_VALUE 1000
#define KICK_GATE_TIMER_MAX_VALUE 500000
#define KICK_GATE_TIMER_DEFAULT_PERCENTAGE 50
#define KICK_GATE_TIMER_PERCENTAGE_STEP 1

#define KICK_DELTA_X_PARAMETER_NAME "KICK: delta x  "
#define KICK_DELTA_X_MIN_VALUE 3
#define KICK_DELTA_X_MAX_VALUE 200
#define KICK_DELTA_X_DEFAULT_PERCENTAGE 50
#define KICK_DELTA_X_PERCENTAGE_STEP 1

#define SNARE_LOW_PASS_PARAMETER_NAME "SNARE: filter  "
#define SNARE_LOW_PASS_MIN_VALUE 1
#define SNARE_LOW_PASS_MAX_VALUE 100
#define SNARE_LOW_PASS_DEFAULT_PERCENTAGE 50
#define SNARE_LOW_PASS_PERCENTAGE_STEP 1

#define SNARE_THRESHOLD_PARAMETER_NAME "SNARE: thresh  "
#define SNARE_THRESHOLD_MIN_VALUE 0
#define SNARE_THRESHOLD_MAX_VALUE 4096
#define SNARE_THRESHOLD_DEFAULT_PERCENTAGE 50
#define SNARE_THRESHOLD_PERCENTAGE_STEP 1

#define SNARE_GATE_TIMER_PARAMETER_NAME "SNARE: gate    "
#define SNARE_GATE_TIMER_MIN_VALUE 1000
#define SNARE_GATE_TIMER_MAX_VALUE 500000
#define SNARE_GATE_TIMER_DEFAULT_PERCENTAGE 50
#define SNARE_GATE_TIMER_PERCENTAGE_STEP 1

#define SNARE_DELTA_X_PARAMETER_NAME "SNARE: delta x "
#define SNARE_DELTA_X_MIN_VALUE 3
#define SNARE_DELTA_X_MAX_VALUE 200
#define SNARE_DELTA_X_DEFAULT_PERCENTAGE 50
#define SNARE_DELTA_X_PERCENTAGE_STEP 1

#define SAVE_VALUES_PARAMETER_NAME "SAVE VALUES    "

/* HID */
#if CONFIG_IDF_TARGET_ESP32
#define OLED_SDA_GPIO_PIN GPIO_NUM_21
#define OLED_SCL_GPIO_PIN GPIO_NUM_22
#define OLED_RESET_GPIO_PIN GPIO_NUM_23
#define ENCODER_CLICK_PIN GPIO_NUM_19
#define ENCODER_PIN_0 GPIO_NUM_5
#define ENCODER_PIN_1 GPIO_NUM_18
#else
#define OLED_SDA_GPIO_PIN GPIO_NUM_42
#define OLED_SCL_GPIO_PIN GPIO_NUM_44
#define OLED_RESET_GPIO_PIN GPIO_NUM_43
#define ENCODER_CLICK_PIN GPIO_NUM_40
#define ENCODER_PIN_0 GPIO_NUM_38
#define ENCODER_PIN_1 GPIO_NUM_39
#endif

#define ENCODER_CLICK_DEBOUNCE_TIME_US 300000
#define ENCODER_GPIO_INPUT_PIN_SEL (1ULL << ENCODER_CLICK_PIN)

/**
 * @brief Type of message sent to the queue.
 * Every message that the queue receives has to be one of the types specified here
 */
typedef enum
{
    ENCODER_INCREASE = 4,
    ENCODER_DECREASE = -4,
    ENCODER_CLICK = 1,
    HID_BPM_MODE_SELECT = 12, /**< Asks the hid to shift to bpm mode (shows current bpm) */
    HID_SETTINGS_MODE_SELECT = 13, /**< Asks the hid to shift to settings mode (allows menu diving) */
    HID_TAP_MODE_SELECT = 14, /**< Asks the hid to shift to tap mode (shows 0->1->2->3->4 for every hit) */
    HID_TAP_0, /**< Asks the hid in tap mode to show 0 */
    HID_TAP_1, /**< Asks the hid in tap mode to show 1 */
    HID_TAP_2, /**< Asks the hid in tap mode to show 2 */
    HID_TAP_3, /**< Asks the hid in tap mode to show 3 */
    HID_TAP_4, /**< Asks the hid in tap mode to show 4 */
} hid_queue_msg;

/**
 * @brief Specify the variable type for the menu entry.
 * Every variable whose value can be modified by the user must have a type
 * choosen from here
 */
typedef enum
{
    BC_FLOAT, /**< float */
    BC_UINT16, /**< uint16_t */
    BC_UINT64, /**< uint64_t */
    BC_YESNO, /**< bool */
} bc_variable_type;

/**
 * @brief Container for the min/max values of the variable in the menu entry.
 * This union is used to contain values from differen variables type by using a single pointer
 * (the casting of the pointer is then made using the vrb type specified in the bc_variable_type enum)
 */
typedef union
{
    uint16_t u16;
    uint64_t u64;
    float f;
    bool b;
} range_values;

/**
 * @brief Entry of the settings menu.
 * Every variable whose value can be modified by the user has an entry
 * modeled by this struct
 */
typedef struct
{
    char name_displayed[16]; /**< Name of the parameter to be displayed */
    void *pointer_to_vrb; /**< Pointer to the variable */
    bc_variable_type vrb_type; /**< Type of variable choosen from the bc_variable_type enum */
    range_values min; /**< Minimum value the variable can assume */
    range_values max; /**< Maximum value the variable can assume */
    uint8_t percentage; /**< Current value of the variable in percentage (min to max) */
    uint8_t percentage_step; /**< Increase/decrease percentage value with every step for the encoder */
} hid_parameter_entry;

/**
 * @brief Index for the menu entries.
 * Change the order of this enum to change the order of the presentation of entries in the menu
 */
typedef enum
{
    MENU_INDEX_SYNC_NARROW,
    MENU_INDEX_SYNC_EXPAND,
    MENU_INDEX_SYNC_BETA,
    MENU_INDEX_TEMPO_ALPHA,
    MENU_INDEX_TEMPO_LATENCY_AMOUNT,
    MENU_INDEX_TEMPO_LATENCY_SMOOTH,
    MENU_INDEX_KICK_THRESHOLD,
    MENU_INDEX_KICK_GATE,
    MENU_INDEX_KICK_FILTER,
    MENU_INDEX_KICK_DELTA_X,
    MENU_INDEX_SNARE_THRESHOLD,
    MENU_INDEX_SNARE_GATE,
    MENU_INDEX_SNARE_FILTER,
    MENU_INDEX_SNARE_DELTA_X,
    MENU_INDEX_SAVE_VALUES,
    MENU_ITEM_INDEX_LENGTH,
} menu_item_index;

extern QueueHandle_t hid_task_queue;
extern TaskHandle_t hid_task_handle;

/**
 * @brief Init function for the hid.
 * This function:
 * - Sets up the OLED display using i2c protocol
 * - Sets up the encoder using the Pulse Counter Module
 * - Sets up the menu and loads variable values from the NVS partition. 
 * Be careful that this function doesn't set up the pointer to the variables! 
 * Every module has to do it by itself calling the set_menu_item_pointer_to_vrb. 
 * After that, calling the hid_set_up_values function actually assign the stored value to the variables.
 */
void hid_init();

/**
 * @brief Assign the value to the variables based on the percentage value.
 * This function has to be called only after every module has registered its variable to the menu.
 */
void hid_set_up_values();

/**
 * @brief Register a variable to the corresponding menu entry.
 * This function has to be called when a module needs to register a variable to the menu entry.
 * It takes the menu entry index (from the enum menu_item_index) and a void pointer to the variable.
 * The type of the pointer is then correctly casted by the module knowing the variable type.
 */
void set_menu_item_pointer_to_vrb(menu_item_index index, void *ptr);

#endif