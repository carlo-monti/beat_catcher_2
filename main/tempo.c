#include "main_defs.h"
#include "tempo.h"
#include "onset_adc.h"
#include "hid.h"
#include "clock.h"

extern SemaphoreHandle_t bc_mutex_handle;
extern main_runtime_vrbs bc; 
extern onset_entry onsets[]; 

TaskHandle_t tempo_task_handle = NULL;

/**
 * @brief Weights for the tempo process for each Inter Onset Interval
 */
// 

const double TEMPO_WEIGHT[17] = {0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0.92, 0, 0.8, 0, 0, 0, 0};

/**
 * @brief Factor for calculating the max width of the window
 */
const uint16_t SIGMA_TEMPO_WIDTH_FACTOR = 20;

/**
 * @brief Main task for the Tempo module
 */
static void tempo_task(void *arg)
{
    static double alpha = 1; // Alpha value of the tempo algorithm
    set_menu_item_pointer_to_vrb(MENU_INDEX_TEMPO_ALPHA, &alpha); // Add this variable to the menu
    static long long sigma_tempo = 60; // Sigma of the tempo algorithm (width of the window)
    static double theta_tempo = 0.80; // Threshold of the tempo algorithm

    while (1)
    {
        /*
        Block waiting for a notify
        */
        uint32_t notify_code = 0;
        xTaskNotifyWait(0, 0, &notify_code, portMAX_DELAY);
        xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
        /*
        Check if there was an onset and get bpm
        */
        bool there_is_an_onset = bc.there_is_an_onset;
        uint64_t tau = bc.tau; 
        xSemaphoreGive(bc_mutex_handle);
        switch (notify_code){
            case TEMPO_START_EVALUATION_NOTIFY:
                if(there_is_an_onset){
                    /*
                    Reset all parameters
                    */
                    double accuracyWin = 0;
                    long long errorWin = 0;
                    int vWin = 0;
                    long long deltaTauTempo = 0;
                    double gaussian = 0;
                    double interOnsetInterval;
                    int v;
                    long long error;
                    double currentAccuracy;
                    /*
                    Update the index of the last onset of the last two bars
                    (remove the onsets that are older than two bars)
                    */
                    xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
                    while (onsets[bc.last_relevant_onset_index_for_tempo].time < (esp_timer_get_time() - (bc.tau * TWO_BAR_LENGTH_IN_8TH)))
                    {
                        if (bc.last_relevant_onset_index_for_tempo == bc.most_recent_onset_index)
                        {
                            break;
                        }
                        bc.last_relevant_onset_index_for_tempo = (bc.last_relevant_onset_index_for_tempo + 1) % ONSET_BUFFER_SIZE;
                    }
                    /* 
                    Read from bc the indexes of first and last onsets
                    */
                    int i = bc.last_relevant_onset_index_for_tempo;
                    int current_onset = bc.most_recent_onset_index;
                    xSemaphoreGive(bc_mutex_handle);
                    /*
                    Start tempo algorithm evaluation
                    */
                    while (i != current_onset)
                    {   
                        /*
                        Calculate IOI for current onset and previous onset (tn - tn-k)
                        */
                        interOnsetInterval = (onsets[current_onset].time - onsets[i].time);
                        v = round(interOnsetInterval / tau); // IOI in tatum intervals v(k)
                        if(v > 17){
                            accuracyWin = 0;
                            break;
                        }
                        error = interOnsetInterval - (tau * v); // error = IOI - v(k) * tau
                        /*
                        Calculate gaussian
                        */
                        gaussian = exp(pow(error, 2) / (double)(-2 * pow(sigma_tempo, 2)));
                        currentAccuracy = gaussian * TEMPO_WEIGHT[v]; // g(en,k)*Ltempo(v(k))
                        if (currentAccuracy > accuracyWin)
                        { 
                            /* 
                            Check if it is the winner
                            */
                            accuracyWin = currentAccuracy;
                            errorWin = error;
                            vWin = v;
                        }
                        i = (i + 1) % ONSET_BUFFER_SIZE;
                    }

                    if (accuracyWin >= theta_tempo)
                    { 
                        /*
                        Update tempo + change parameters (raise threshold)
                        */
                        deltaTauTempo = alpha * accuracyWin * (errorWin / vWin);
                        if (deltaTauTempo >= 900 || deltaTauTempo <= -900)
                        {
                            /*
                            Send to clock module the value for tempo change
                            */
                            clock_task_queue_entry txBuffer = {
                                .type = CLOCK_QUEUE_SET_DELTA_TAU_TEMPO,
                                .value = deltaTauTempo,
                            };
                            xQueueSend(clock_task_queue, &txBuffer,(TickType_t)0);
                        }
                        if (accuracyWin >= theta_tempo + HEADROOM_VALUE_TEMPO)
                        {
                            /*
                            Update parameters of the threshold
                            */
                            theta_tempo = theta_tempo + (0.3 * (accuracyWin - theta_tempo - 0.1));
                        }
                    }
                    else
                    {   
                        /* 
                        Only change parameters (lower threshold)
                        */
                        theta_tempo = 0.6 * theta_tempo;
                    }
                    /* 
                    Update size of the window
                    */
                    sigma_tempo = sigma_tempo * (1 + ((0.7 * TEMPO_WEIGHT[vWin]) - accuracyWin));
                    if (sigma_tempo < round(tau / SIGMA_TEMPO_WIDTH_FACTOR))
                    {
                        sigma_tempo = round(tau / SIGMA_TEMPO_WIDTH_FACTOR);
                    }
                }
                break;
            case TEMPO_RESET_PARAMETERS:
                /* 
                A new sequence is starting, set parameters with the new bpm
                */
                sigma_tempo = round(tau / SIGMA_TEMPO_WIDTH_FACTOR);
                theta_tempo = 0.80;
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Init function for the tempo module
 * It just creates the tempo_task
*/
void tempo_init(){
    xTaskCreate(tempo_task, "Tempo_Task", TEMPO_TASK_STACK_SIZE, NULL, TEMPO_TASK_PRIORITY, &tempo_task_handle);
};