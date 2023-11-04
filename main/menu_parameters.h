/**
 * @file menu_parameters.h
 * @brief This file contains the parameters for each menu entry
 */

#ifndef BC_MENU_PARAMETERS
#define BC_MENU_PARAMETERS

/**
 * @brief Name displayed when the system asks to save values
 */
#define SAVE_VALUES_PARAMETER_NAME_TOP "SAVE VALUES    "
#define SAVE_VALUES_PARAMETER_NAME "SAVE VALUES    "

#define CHECK_GAIN_NAME_TOP "GAIN:          "
#define CHECK_GAIN_NAME "Set the gain so"
#define CHECK_GAIN_TEXT_1 "the light don't"
#define CHECK_GAIN_TEXT_2 "blink.         " 

/**
 * @{ \name alpha menu entry parameters
 */
#define ALPHA_PARAMETER_NAME_TOP "TEMPO          "
#define ALPHA_PARAMETER_NAME "Responsiveness:"
#define ALPHA_STORAGE_KEY "tempo_alpha    "
#define ALPHA_MIN_VALUE 0.2
#define ALPHA_MAX_VALUE 1.2
#define ALPHA_DEFAULT_PERCENTAGE 50
#define ALPHA_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name beta menu entry parameters
 */
#define BETA_PARAMETER_NAME_TOP "SYNC           "
#define BETA_PARAMETER_NAME "Responsiveness:"
#define BETA_STORAGE_KEY "sync_beta      "
#define BETA_MIN_VALUE 0.2
#define BETA_MAX_VALUE 1.2
#define BETA_DEFAULT_PERCENTAGE 50
#define BETA_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name Tempo spread menu entry parameters
 */
#define SPREAD_PARAMETER_NAME_TOP "SYNC           "
#define SPREAD_PARAMETER_NAME "Increase value:"
#define SPREAD_STORAGE_KEY "sync_beta      "
#define SPREAD_MIN_VALUE 0
#define SPREAD_MAX_VALUE 8
#define SPREAD_DEFAULT_PERCENTAGE 50
#define SPREAD_PERCENTAGE_STEP 13
/**
 * @}
 */

/**
 * @{ \name kick low pass menu entry parameters
 */
#define KICK_LOW_PASS_PARAMETER_NAME_TOP "KICK           "
#define KICK_LOW_PASS_PARAMETER_NAME "Filter:        "
#define KICK_LOW_PASS_STORAGE_KEY "kick_filter    "
#define KICK_LOW_PASS_MIN_VALUE 1
#define KICK_LOW_PASS_MAX_VALUE 100
#define KICK_LOW_PASS_DEFAULT_PERCENTAGE 50
#define KICK_LOW_PASS_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name kick threshold menu entry parameters
 */
#define KICK_THRESHOLD_PARAMETER_NAME_TOP "KICK           "
#define KICK_THRESHOLD_PARAMETER_NAME "Threshold:     "
#define KICK_THRESHOLD_STORAGE_KEY "kick_thresh    "
#define KICK_THRESHOLD_MIN_VALUE 0
#define KICK_THRESHOLD_MAX_VALUE 4096
#define KICK_THRESHOLD_DEFAULT_PERCENTAGE 50
#define KICK_THRESHOLD_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name kick gate menu entry parameters
 */
#define KICK_GATE_TIMER_PARAMETER_NAME_TOP "KICK           "
#define KICK_GATE_TIMER_PARAMETER_NAME "Retrigger gate:"
#define KICK_GATE_TIMER_STORAGE_KEY "kick_gate      "
#define KICK_GATE_TIMER_MIN_VALUE 70000 // 70ms (72ms is the distance between two 16th notes at ~208bpm)
#define KICK_GATE_TIMER_MAX_VALUE 500000 // 500ms (1500ms is the distance between two 4th notes at ~40bpm)
#define KICK_GATE_TIMER_DEFAULT_PERCENTAGE 50
#define KICK_GATE_TIMER_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name kick delta menu entry parameters
 */
#define KICK_DELTA_X_PARAMETER_NAME_TOP "KICK           "
#define KICK_DELTA_X_PARAMETER_NAME "Onset length:  "
#define KICK_DELTA_X_STORAGE_KEY "kick_delta     "
#define KICK_DELTA_X_MIN_VALUE 3
#define KICK_DELTA_X_MAX_VALUE 200
#define KICK_DELTA_X_DEFAULT_PERCENTAGE 50
#define KICK_DELTA_X_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name snare low pass menu entry parameters
 */
#define SNARE_LOW_PASS_PARAMETER_NAME_TOP "SNARE          "
#define SNARE_LOW_PASS_PARAMETER_NAME "Filter:        "
#define SNARE_LOW_PASS_STORAGE_KEY "snare_filter   "
#define SNARE_LOW_PASS_MIN_VALUE 1
#define SNARE_LOW_PASS_MAX_VALUE 100
#define SNARE_LOW_PASS_DEFAULT_PERCENTAGE 50
#define SNARE_LOW_PASS_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name snare threshold menu entry parameters
 */
#define SNARE_THRESHOLD_PARAMETER_NAME_TOP "SNARE          "
#define SNARE_THRESHOLD_PARAMETER_NAME "Threshold:     "
#define SNARE_THRESHOLD_STORAGE_KEY "snare_thresh   "
#define SNARE_THRESHOLD_MIN_VALUE 0
#define SNARE_THRESHOLD_MAX_VALUE 4096
#define SNARE_THRESHOLD_DEFAULT_PERCENTAGE 50
#define SNARE_THRESHOLD_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name snare gate menu entry parameters
 */
#define SNARE_GATE_TIMER_PARAMETER_NAME_TOP "SNARE          "
#define SNARE_GATE_TIMER_PARAMETER_NAME "Retrigger gate:"
#define SNARE_GATE_TIMER_STORAGE_KEY "snare_gate     "
#define SNARE_GATE_TIMER_MIN_VALUE 1000
#define SNARE_GATE_TIMER_MAX_VALUE 500000
#define SNARE_GATE_TIMER_DEFAULT_PERCENTAGE 50
#define SNARE_GATE_TIMER_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name snare delta menu entry parameters
 */
#define SNARE_DELTA_X_PARAMETER_NAME_TOP "SNARE          "
#define SNARE_DELTA_X_PARAMETER_NAME "Onset length:  "
#define SNARE_DELTA_X_STORAGE_KEY "snare_delta    "
#define SNARE_DELTA_X_MIN_VALUE 3
#define SNARE_DELTA_X_MAX_VALUE 200
#define SNARE_DELTA_X_DEFAULT_PERCENTAGE 50
#define SNARE_DELTA_X_PERCENTAGE_STEP 1
/**
 * @}
 */
#endif