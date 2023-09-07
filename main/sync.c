#include "sync.h"
#include "clock.h"
#include "onset_adc.h"
#include "tempo.h"
#include "hid.h"

extern QueueHandle_t clock_task_queue;
extern TaskHandle_t tempo_task_handle;
extern SemaphoreHandle_t bc_mutex_handle;
TaskHandle_t sync_task_handle;
extern main_runtime_vrbs bc;
extern onset_entry onsets[];
extern void set_menu_item_pointer_to_vrb(menu_item_index index, void *ptr);

/**
 * @brief Weights for the sync process for each bar position (for kick and snare)
 */
const float SYNC_WEIGHT[2][TWO_BAR_LENGTH_IN_8TH] = {{1, 0.1, 1, 0.1,
                                                          1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1},
                                                         {1, 0.1, 1, 0.1,
                                                          1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1}};

static void sync_task(void *arg)
{
    /*
    Set up initial values
    */
    static float beta = 0.6;
    set_menu_item_pointer_to_vrb(MENU_INDEX_SYNC_BETA, &beta); // add variable to menu
    static float narrow_ratio = 1;
    set_menu_item_pointer_to_vrb(MENU_INDEX_SYNC_NARROW, &narrow_ratio); // add variable to menu
    static float expand_ratio = 1;
    set_menu_item_pointer_to_vrb(MENU_INDEX_SYNC_EXPAND, &expand_ratio); // add variable to menu
    static long sigma_sync_width = 10;
    static long sigma_sync = 60;
    static float theta_sync = 0.80;
    static uint8_t last_layer_of_bar_pos = 0;
    static uint8_t last_synced_layer = 0;
    static float accuracy_of_last_synced_layer = 0;
    while (1)
    {
        /*
        Block waiting until notified
        */
        uint32_t notify_code = 0;
        xTaskNotifyWait(0, 0, &notify_code, portMAX_DELAY);
        /*
        Get current runtime values
        */
        xSemaphoreTake(bc_mutex_handle, portMAX_DELAY);
        bool there_is_an_onset = bc.there_is_an_onset;
        uint64_t tau = bc.tau;
        uint64_t expected_beat = bc.expected_beat;
        uint8_t bar_position = bc.bar_position;
        uint8_t layer = bc.layer;
        int i = bc.last_relevant_onset_index_for_sync;
        uint32_t most_recent_onset_index = bc.most_recent_onset_index;
        xSemaphoreGive(bc_mutex_handle);
        switch (notify_code)
        {
        case SYNC_START_EVALUATION_NOTIFY:
            if(there_is_an_onset){ 
                /*
                If there is an onset...
                */
                float final_accuracy_to_sync = -1;
                long final_delta_tau_sync = 0;
                long long two_sigma_squared = -2 * pow(sigma_sync, 2);
                while(1){
                    /* 
                    Repeat this calculation for all the onsets
                    */
                    float accuracy = 0;
                    float gaussian;
                    float current_sync_weight;
                    long error;
                    /*
                    Set the weight depending on onset type (kick or snare)
                    */
                    current_sync_weight = SYNC_WEIGHT[onsets[i].type][bar_position]; 
                    error = onsets[i].time - expected_beat;
                    /*
                    calculate accuracy for the current onset
                    */
                    gaussian = exp(pow(error, 2) / two_sigma_squared);
                    accuracy = gaussian * current_sync_weight;
                    if (accuracy > theta_sync)
                    {   
                        /*
                        if accuracy is higher than the threshold
                        */
                        if (layer >= last_synced_layer)
                        {
                            /*
                            if the current layer is equal or higher than the higherlayer
                            and if the accuracy is higher than than the last synced...
                            */
                            if(accuracy > final_accuracy_to_sync){
                                final_accuracy_to_sync = accuracy;
                                /*
                                calculate the value of delta_tau_sync
                                */
                                final_delta_tau_sync = ((gaussian + beta) / (beta + 1)) * final_accuracy_to_sync * error; 
                            }
                            if (accuracy >= theta_sync + HEADROOM_VALUE_SYNC)
                            {
                                /*
                                if accuracy is higher than the threshold + headroom
                                change parameters (raise threshold, narrow window)
                                */
                                theta_sync = theta_sync + (0.3 * (accuracy - theta_sync - 0.1));
                                sigma_sync = sigma_sync * (narrow_ratio + ((0.7 * current_sync_weight) - accuracy));
                                if (sigma_sync < round(tau / sigma_sync_width))
                                {
                                    sigma_sync = round(tau / sigma_sync_width);
                                }
                            }
                        }
                        else
                        { 
                            if (accuracy > accuracy_of_last_synced_layer)
                            {   
                                /*
                                if the current layer is lower than the higher layer
                                but the accuracy is higher than the last synced then sync to this onset
                                */
                                if(accuracy > final_accuracy_to_sync){
                                    final_accuracy_to_sync = accuracy;
                                    /*
                                    calculate the value of delta_tau_sync
                                    */
                                    final_delta_tau_sync = ((gaussian + beta) / (beta + 1)) * final_accuracy_to_sync * error; 
                                }
                            }
                        }
                    }
                    else
                    {
                        /*
                        if accuracy is lower than the threshold then only change parameters 
                        (lower threshold, expand window) if layer >= globlayer
                        */
                        if (layer >= last_synced_layer)
                        {
                            theta_sync = 0.6 * theta_sync;
                            sigma_sync = sigma_sync * (expand_ratio + ((0.7 * current_sync_weight) - accuracy));
                            if (sigma_sync < round(tau / sigma_sync_width))
                            {
                                sigma_sync = round(tau / sigma_sync_width);
                            }
                        }
                    }
                    if(i==most_recent_onset_index){
                        /* 
                        Break if all the onsets has been evaluated
                        */
                        break;
                    };
                    i=(i+1)%ONSET_BUFFER_SIZE;
                }
                if(final_accuracy_to_sync > 0){
                    /*
                    if there is something to sync
                    send message to clock queue
                    */
                    clock_task_queue_entry txBuffer = {
                        .type = CLOCK_QUEUE_SET_DELTA_TAU_SYNC,
                        .value = final_delta_tau_sync,
                    };
                    xQueueSend(clock_task_queue, &txBuffer,(TickType_t)0);
                    if (last_layer_of_bar_pos >= last_synced_layer)
                    {
                        /*
                        If the current layer is >= than the higher layer
                        update the layer
                        */
                        last_synced_layer = last_layer_of_bar_pos;
                    }
                }
            }else{
                /* 
                if there is no onset and current layer is == higher layer then decrease higher layer
                */
                if (layer == last_synced_layer)
                    {
                        if (last_synced_layer > 1)
                        {
                            last_synced_layer -= 1;
                        }
                    }
            }
            /* 
            Notify Tempo module to start evaluation
            */
            xTaskNotify(tempo_task_handle, TEMPO_START_EVALUATION_NOTIFY, eSetValueWithOverwrite);
            break;
        case SYNC_RESET_PARAMETERS:
            /* 
            A new sequence is starting: reset parameters with the new tau
            */        
            sigma_sync = round(tau / sigma_sync_width);
            sigma_sync = 60;
            theta_sync = 0.80;
            last_layer_of_bar_pos = 0;
            last_synced_layer = 0;
            accuracy_of_last_synced_layer = 0;
            break;
        default:
            break;
        }
    }
}

void sync_init(){
    /*
    Create sync_task
    */
    xTaskCreate(sync_task, "Sync_Task", SYNC_TASK_STACK_SIZE, NULL, SYNC_TASK_PRIORITY, &sync_task_handle);
}