/**
 * @file hid.h
 * @brief HID Module handles the encoder, the display and the permanent storage of the variables values.
 * The HID Module allow the user to edit the parameters of the system modifying the value of a variable,
 * it permanently stores the modified value and displays some information.
 * 
 * Its behaviour depends on the main mode (if the mode changes, the hid must be notified sending a message to its queue).
 * - SETTINGS mode: the module shows the name and the value of the currently selected parameter.
 *  - When the encoder rotates the value is increased/decreased.
 *  - When the encoder is clicked, the next parameter is selected.
 *  - The last parameter allow to permanently save values
 * - PLAY mode: shows the current bpm value
 * - TAP mode: shows the number of the hits received (0 to 4)
 * - SLEEP mode: the display is turned off
 * 
 * To be editable by the user, every parameter must have an entry on the menu.
 * To add a menu entry:
 * - Add a value on the menu_item_index enum (file: hid.h) to select the order for the parameter name in the menu
 * - Add the parameter values definitions (file: menu_parameters.h).
 * - Add in the menu_init function (file: hid.c) the instructions to fill all the
 * struct fields. Remember to set the pointer to the variable to NULL. 
 * - After that, place a call to the set_menu_item_pointer_to_vrb function inside the module which contains the variable
 * to register the pointer to the struct.
 */

#ifndef BC_HID_H
#define BC_HID_H

#include "main_defs.h"
#include "menu_parameters.h"

/**
 * @{ \name GPIO pins for encoder
 */
#if CONFIG_IDF_TARGET_ESP32
#define ENCODER_CLICK_PIN GPIO_NUM_19
#define ENCODER_PIN_0 GPIO_NUM_18
#define ENCODER_PIN_1 GPIO_NUM_5
#else
#define ENCODER_CLICK_PIN GPIO_NUM_40
#define ENCODER_PIN_0 GPIO_NUM_38
#define ENCODER_PIN_1 GPIO_NUM_39
#endif
/**
 * @}
 */

/**
 * @{ \name GPIO pins for OLED
 */
#if CONFIG_IDF_TARGET_ESP32
#define OLED_SDA_GPIO_PIN GPIO_NUM_21
#define OLED_SCL_GPIO_PIN GPIO_NUM_22
#define OLED_RESET_GPIO_PIN GPIO_NUM_23
#else
#define OLED_SDA_GPIO_PIN GPIO_NUM_42
#define OLED_SCL_GPIO_PIN GPIO_NUM_44
#define OLED_RESET_GPIO_PIN GPIO_NUM_43
#endif
/**
 * @}
 */

/**
 * @{ \name Encoder handling parameters
 */
#define ENCODER_CLICK_DEBOUNCE_TIME_US 300000
#define ENCODER_GPIO_INPUT_PIN_SEL (1ULL << ENCODER_CLICK_PIN)
/**
 * @}
 */

/**
 * @brief Type of message sent to the queue.
 * Every message that the queue receives has to be one of the types specified here
 */
typedef enum
{
    ENCODER_INCREASE = 4, /**< Actual value from the pulse counter (1 click = 4 steps) */
    ENCODER_DECREASE = -4, /**< Actual value from the pulse counter (-1 click = -4 steps) */
    ENCODER_CLICK = 1, /**< Asks the hid to change the currently selected menu item */
    HID_PLAY_MODE_SELECT = 12, /**< Asks the hid to shift to bpm mode (shows current bpm) */
    HID_SETTINGS_MODE_SELECT = 13, /**< Asks the hid to shift to settings mode (allows menu diving) */
    HID_TAP_MODE_SELECT = 14, /**< Asks the hid to shift to tap mode (shows 0->1->2->3->4 for every hit) */
    HID_ENTER_SLEEP_MODE = 15, /**< Asks the hid to turn off screen */
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
    char top_name_displayed[16]; /**< Name of the parameter to be displayed on top*/
    char name_displayed[16]; /**< Name of the parameter to be displayed */
    char storage_key[16]; /**< Name of the parameter to be displayed */
    void *pointer_to_vrb; /**< Pointer to the variable */
    bc_variable_type vrb_type; /**< Type of variable choosen from the bc_variable_type enum */
    range_values min; /**< Minimum value the variable can assume */
    range_values max; /**< Maximum value the variable can assume */
    uint8_t percentage; /**< Current value of the variable in percentage (min to max) */
    uint8_t percentage_step; /**< Increase/decrease percentage value with every step for the encoder */
    bool has_corresponding_value; /**< If set to false, the parameter is just a dummy value for something else ("Save values","Gain") */
} hid_parameter_entry;

/**
 * @brief Index for the menu entries.
 * Change the order of this enum to change the order of the presentation of entries in the menu
 */
typedef enum
{
    MENU_INDEX_CHECK_GAIN,
    MENU_INDEX_SYNC_BETA,
    MENU_INDEX_TEMPO_ALPHA,
    MENU_INDEX_TEMPO_SPREAD,
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

/**
 * @brief Handle for the hid queue.
 * Handle of the hid queue to receive messages
 */
extern QueueHandle_t hid_task_queue;

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