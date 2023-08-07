#include "sync.h"
#include "clock.h"
#include "onset_adc.h"
#include "tempo.h"
#include "hid.h"

extern QueueHandle_t clock_task_queue;
extern TaskHandle_t tempo_task_handle;
TaskHandle_t sync_task_handle;
extern main_runtime_vrbs bc;
extern onset_entry onsets[];
extern void set_menu_item_pointer_to_vrb(menu_item_index index, void *ptr);

static void sync_task(void *arg)
{
    const float SYNC_WEIGHT[2][TWO_BAR_LENGTH_IN_8TH] = {{1, 0.1, 1, 0.1,
                                                          1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1},
                                                         {1, 0.1, 1, 0.1,
                                                          1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1, 1, 0.1}}; // snare weights
    static float beta = 0.6;
    set_menu_item_pointer_to_vrb(MENU_INDEX_SYNC_BETA, &beta);
    static float narrow_ratio = 1;
    set_menu_item_pointer_to_vrb(MENU_INDEX_SYNC_NARROW, &narrow_ratio);
    static float expand_ratio = 1;
    set_menu_item_pointer_to_vrb(MENU_INDEX_SYNC_EXPAND, &expand_ratio);
    static long sigma_sync_width = 10;
    //set_menu_item_pointer_to_vrb(MENU_INDEX_SYNC_EXPAND, &expand_ratio); xxx add menu entyry
    static long sigma_sync = 60;
    static float theta_sync = 0.80;
    static uint8_t last_layer_of_bar_pos = 0;
    static uint8_t last_synced_layer = 0;
    static float accuracy_of_last_synced_layer = 0;
    while (1)
    {
        uint32_t notify_code = 0;
        xTaskNotifyWait(0, 0, &notify_code, portMAX_DELAY);
        switch (notify_code)
        {
        case SYNC_START_EVALUATION_NOTIFY:
            if(bc.there_is_an_onset){ // if it has been an onset to sync
                // xxx put mutex
                int i = bc.last_relevant_onset_index_for_sync;
                //ESP_LOGI("Sync_task", "Start Sync_task: last_relevant_onset %d",i);
                uint32_t most_recent_onset_index = bc.most_recent_onset_index;
                float final_accuracy_to_sync = -1;
                long final_delta_tau_sync = 0;
                long long two_sigma_squared = -2 * pow(sigma_sync, 2);
                while(1){
                    float accuracy = 0;
                    float gaussian;
                    float current_sync_weight;
                    long error;
                    current_sync_weight = SYNC_WEIGHT[onsets[i].type][bc.bar_position]; // chose between kick or snare
                    error = onsets[i].time - bc.expected_beat;
                    //ESP_LOGI("Sync_task", "Sync_task: NEW ONSET----------------------");
                    //ESP_LOGI("Sync_task", "Tipo: %d",onsets[i].type);
                    // ESP_LOGI("Sync_task", "tau: %lld", bc.tau);
                    // ESP_LOGI("Sync_task", "onset: %lld",onsets[i].time);
                    // ESP_LOGI("Sync_task", "expected: %lld",bc.expected_beat);
                    // ESP_LOGI("Sync_task", "error: %ld", error);
                    // ESP_LOGI("Sync_task", "sigma_sync: %ld", sigma_sync);
                     // calculate accuracy for the current onset
                    gaussian = exp(pow(error, 2) / two_sigma_squared);
                    accuracy = gaussian * current_sync_weight;
                    //ESP_LOGI("Sync_task", "accuracy: %f", accuracy);
                    //ESP_LOGI("Sync_task", "theta_sync: %f", theta_sync);
                    if (accuracy > theta_sync)// if accuracy is higher than the threshold
                    {   
                        if (bc.layer >= last_synced_layer)
                        {// if the current layer is equal or higher than the higherlayer
                            // and if the accuracy is higher than than the last synced
                            if(accuracy > final_accuracy_to_sync){
                                final_accuracy_to_sync = accuracy;
                                final_delta_tau_sync = ((gaussian + beta) / (beta + 1)) * final_accuracy_to_sync * error; // calculate the value of delta_tau_sync
                            }
                            if (accuracy >= theta_sync + HEADROOM_VALUE) // if accuracy is higher than the threshold + headroom
                            {
                                theta_sync = theta_sync + (0.3 * (accuracy - theta_sync - 0.1)); // change parameters (raise threshold, narrow window)
                                sigma_sync = sigma_sync * (narrow_ratio + ((0.7 * current_sync_weight) - accuracy));
                                if (sigma_sync < round(bc.tau / sigma_sync_width))
                                {
                                    sigma_sync = round(bc.tau / sigma_sync_width); // xxx mettere 20
                                }
                                //ESP_LOGI("Sync_task", "Update parameters");
                            }
                        }
                        else
                        { 
                            if (accuracy > accuracy_of_last_synced_layer)
                            {   // if the current layer is lower than the higher layer
                                // but the accuracy is higher than the last synced then sync to this onset
                                if(accuracy > final_accuracy_to_sync){
                                    final_accuracy_to_sync = accuracy;
                                    final_delta_tau_sync = ((gaussian + beta) / (beta + 1)) * final_accuracy_to_sync * error; // calculate the value of delta_tau_sync
                                }
                            }
                        }
                    }
                    else
                    {
                        // if accuracy is lower than the threshold then only change parameters 
                        /// (lower threshold, expand window) if layer >= globlayer
                        if (bc.layer >= last_synced_layer)
                        {
                            theta_sync = 0.6 * theta_sync;
                            sigma_sync = sigma_sync * (expand_ratio + ((0.7 * current_sync_weight) - accuracy));
                            if (sigma_sync < round(bc.tau / sigma_sync_width))
                            {
                                sigma_sync = round(bc.tau / sigma_sync_width); // xxx mettere 20
                            }
                        }
                        //ESP_LOGI("Sync_task", "Only update parameters");
                    }
                    if(i==most_recent_onset_index){
                        break;
                    };
                    i=(i+1)%ONSET_BUFFER_SIZE;
                }
                if(final_accuracy_to_sync > 0){
                    clock_task_queue_entry txBuffer = {
                        .type = CLOCK_QUEUE_SET_DELTA_TAU_SYNC,
                        .value = final_delta_tau_sync,
                    };
                    xQueueSend(clock_task_queue, &txBuffer,(TickType_t)0);
                    //ESP_LOGI("Sync_task", "SYnc to onset!");
                    if (last_layer_of_bar_pos >= last_synced_layer)
                    {
                        // and the current layer is >= than the higher layer
                        last_synced_layer = last_layer_of_bar_pos;
                        // and update the layer
                    }
                }
            }else{
                // if there is no onset and current layer is == higher layer then decrease higher layer
                if (bc.layer == last_synced_layer) // xxx shoulndn't it be >=?
                    {
                        if (last_synced_layer > 1)
                        {
                            last_synced_layer -= 1;
                        }
                    }
            }
            // ESP_LOGI("Sync_task", "Ultimo onset: %lld", onsets[bc.most_recent_onset_index].time);
            // ESP_LOGI("Sync_task", "Tipo: %d", onsets[bc.most_recent_onset_index].type);
            xTaskNotify(tempo_task_handle, TEMPO_START_EVALUATION_NOTIFY, eSetValueWithOverwrite);
            break;
        case SYNC_RESET_PARAMETERS:
            sigma_sync = round(bc.tau / sigma_sync_width);
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
    xTaskCreate(sync_task, "Sync_Task", 4096, NULL, SYNC_TASK_PRIORITY, &sync_task_handle);
}