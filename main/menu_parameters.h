/**
 * @file menu_parameters.h
 * @brief This file contains the parameters for each menu entry
 */

#ifndef BC_MENU_PARAMETERS
#define BC_MENU_PARAMETERS

/**
 * @{ \name alpha menu entry parameters
 */
#define ALPHA_PARAMETER_NAME "TEMPO: alpha   "
#define ALPHA_MIN_VALUE 0.1
#define ALPHA_MAX_VALUE 2
#define ALPHA_DEFAULT_PERCENTAGE 50
#define ALPHA_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name latency amount menu entry parameters
 */
#define LATENCY_AMOUNT_PARAMETER_NAME "TEMPO: amount  "
#define LATENCY_AMOUNT_MIN_VALUE 0
#define LATENCY_AMOUNT_MAX_VALUE 10
#define LATENCY_AMOUNT_DEFAULT_PERCENTAGE 50
#define LATENCY_AMOUNT_PERCENTAGE_STEP 10
/**
 * @}
 */

/**
 * @{ \name latency smooth menu entry parameters
 */
#define LATENCY_SMOOTH_PARAMETER_NAME "TEMPO: smooth  "
#define LATENCY_SMOOTH_MIN_VALUE 0
#define LATENCY_SMOOTH_MAX_VALUE 10
#define LATENCY_SMOOTH_DEFAULT_PERCENTAGE 50
#define LATENCY_SMOOTH_PERCENTAGE_STEP 10
/**
 * @}
 */

/**
 * @{ \name narrow ratio menu entry parameters
 */
#define NARROW_RATIO_PARAMETER_NAME "SYNC: narrow r "
#define NARROW_RATIO_MIN_VALUE 0.1
#define NARROW_RATIO_MAX_VALUE 2
#define NARROW_RATIO_DEFAULT_PERCENTAGE 50
#define NARROW_RATIO_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name expand ratio menu entry parameters
 */
#define EXPAND_RATIO_PARAMETER_NAME "SYNC: expand r "
#define EXPAND_RATIO_MIN_VALUE 0.1
#define EXPAND_RATIO_MAX_VALUE 2
#define EXPAND_RATIO_DEFAULT_PERCENTAGE 50
#define EXPAND_RATIO_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name beta menu entry parameters
 */
#define BETA_PARAMETER_NAME "SYNC: beta     "
#define BETA_MIN_VALUE 0.1
#define BETA_MAX_VALUE 1
#define BETA_DEFAULT_PERCENTAGE 50
#define BETA_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name kick low pass menu entry parameters
 */
#define KICK_LOW_PASS_PARAMETER_NAME "KICK: filter   "
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
#define KICK_THRESHOLD_PARAMETER_NAME "KICK: thresh   "
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
#define KICK_GATE_TIMER_PARAMETER_NAME "KICK: gate     "
#define KICK_GATE_TIMER_MIN_VALUE 1000
#define KICK_GATE_TIMER_MAX_VALUE 500000
#define KICK_GATE_TIMER_DEFAULT_PERCENTAGE 50
#define KICK_GATE_TIMER_PERCENTAGE_STEP 1
/**
 * @}
 */

/**
 * @{ \name kick delta menu entry parameters
 */
#define KICK_DELTA_X_PARAMETER_NAME "KICK: delta x  "
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
#define SNARE_LOW_PASS_PARAMETER_NAME "SNARE: filter  "
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
#define SNARE_THRESHOLD_PARAMETER_NAME "SNARE: thresh  "
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
#define SNARE_GATE_TIMER_PARAMETER_NAME "SNARE: gate    "
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
#define SNARE_DELTA_X_PARAMETER_NAME "SNARE: delta x "
#define SNARE_DELTA_X_MIN_VALUE 3
#define SNARE_DELTA_X_MAX_VALUE 200
#define SNARE_DELTA_X_DEFAULT_PERCENTAGE 50
#define SNARE_DELTA_X_PERCENTAGE_STEP 1
/**
 * @}
 */
#endif