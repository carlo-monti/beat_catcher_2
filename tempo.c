#include "main_defs.h"
#include "tempo.h"
#include "onset_adc.h"
#include "hid.h"
#include "clock.h"

extern SemaphoreHandle_t onset_array_mutex_handle;
extern TaskHandle_t tempo_task_handle;
extern main_runtime_vrbs bc;
extern onset_entry onsets[];
extern volatile main_mode mode;

TaskHandle_t tempo_task_handle = NULL;
const float TEMPO_WEIGHT[17] = {0, 0, 1, 0, 1, 0, 0, 0, 0.92, 0, 0, 0, 0.8, 0, 0, 0, 0.8};

static void tempo_task(void *arg)
{
    static float alpha = 1;
    set_menu_item_pointer_to_vrb(MENU_INDEX_TEMPO_ALPHA, &alpha);
    static uint16_t sigma_tempo_width = 10;
    set_menu_item_pointer_to_vrb(MENU_INDEX_TEMPO_ALPHA, &alpha); // xxxx add menu for this
    static long sigma_tempo = 60; // width of the window xxx can be uint ?
    static float theta_tempo = 0.80;

    while (1)
    {
        uint32_t notify_code = 0;
        xTaskNotifyWait(0, 0, &notify_code, portMAX_DELAY);
        switch (notify_code){
            case TEMPO_START_EVALUATION_NOTIFY:
                if(bc.there_is_an_onset){
                    float accuracyWin = 0; // accuracy of the winner interval
                    long errorWin = 0;     // error of the winner interval
                    int vWin = 0;          // v of the winner interval
                    long long deltaTauTempo = 0;
                    float gaussian = 0;
                    float interOnsetInterval; // actually a int but defined float for the need of division without truncating
                    int v;
                    long error;
                    float currentAccuracy;
                    // take semaphore xxx
                    int i = bc.last_relevant_onset_index_for_tempo;
                    //ESP_LOGI("tempo_task", "Started-> relevant %d last %ld",i,bc.most_recent_onset_index);
                    //ESP_LOGI("tempo_task", "last_relevant_onset_index_for_tempo %d",i);
                    while (i != bc.most_recent_onset_index)
                    { // for every onset...
                        interOnsetInterval = (onsets[bc.most_recent_onset_index].time - onsets[i].time);
                        // calculate IOI for current onset and previous onset (tn - tn-k)
                        //ESP_LOGI("TEMPO","interOnsetInterval %f",interOnsetInterval);
                        v = round(interOnsetInterval / bc.tau);
                        if(v > 17){
                            accuracyWin = 0;
                            break;
                        }
                        // IOI in tatum intervals v(k)
                        error = interOnsetInterval - (bc.tau * v);
                        // error = IOI - v(k) * tau
                        gaussian = exp(pow(error, 2) / (float)(-2 * pow(sigma_tempo, 2))); // calculate gaussian
                        currentAccuracy = gaussian * TEMPO_WEIGHT[v];                      // g(en,k)*Ltempo(v(k))
                        //ESP_LOGI("TEMPO","currentAccuracy %f",currentAccuracy);
                        if (currentAccuracy > accuracyWin)
                        { // check if it is the winner
                            accuracyWin = currentAccuracy;
                            errorWin = error;
                            vWin = v;
                        }
                        i = (i + 1) % ONSET_BUFFER_SIZE;
                    }

                    if (accuracyWin >= theta_tempo)
                    { // Update tempo + change parameters (raise threshold)
                        deltaTauTempo = alpha * accuracyWin * (errorWin / vWin);
                        if (deltaTauTempo != 0)
                        {
                            clock_task_queue_entry txBuffer = {
                                .type = CLOCK_QUEUE_SET_DELTA_TAU_TEMPO,
                                .value = deltaTauTempo,
                            };
                            xQueueSend(clock_task_queue, &txBuffer,(TickType_t)0);
                            // ESP_LOGI("TEMPO","alpha %f",alpha);
                            // ESP_LOGI("TEMPO","accuracyWin %f",accuracyWin);
                            // ESP_LOGI("TEMPO","errorWin %ld",errorWin);
                            // ESP_LOGI("TEMPO","vWin %d",vWin);
                            ESP_LOGI("tempo_task", "Send tempo update msg to clock! %lld",deltaTauTempo);
                        }
                        if (accuracyWin >= theta_tempo + 0.1)
                        {
                            theta_tempo = theta_tempo + (0.3 * (accuracyWin - theta_tempo - 0.1));
                        }
                    }
                    else
                    {   // just change parameters (lower threshold)
                        theta_tempo = 0.6 * theta_tempo;
                    }
                    sigma_tempo = sigma_tempo * (1 + ((0.7 * TEMPO_WEIGHT[vWin]) - accuracyWin));
                    if (sigma_tempo < round(bc.tau / sigma_tempo_width))
                    {
                        sigma_tempo = round(bc.tau / sigma_tempo_width);
                    }
                }
                // garbage collector
                xSemaphoreTake(onset_array_mutex_handle, portMAX_DELAY);
                while (onsets[bc.last_relevant_onset_index_for_tempo].time < (GET_CURRENT_TIME_MS() - (bc.tau * TWO_BAR_LENGTH_IN_8TH)))
                {
                    if (bc.last_relevant_onset_index_for_tempo == bc.most_recent_onset_index)
                    {
                        break;
                    }
                    bc.last_relevant_onset_index_for_tempo = (bc.last_relevant_onset_index_for_tempo + 1) % ONSET_BUFFER_SIZE;
                }
                xSemaphoreGive(onset_array_mutex_handle);
                break;
            case TEMPO_RESET_PARAMETERS:
                ESP_LOGI("tempo_task", "SIGMA SETTED!");
                sigma_tempo = round(bc.tau / sigma_tempo_width); // xxx mettere 20
                theta_tempo = 0.80;
                break;
            default:
                break;
        }
    }
}

void tempo_init(){
    xTaskCreate(tempo_task, "Tempo_Task", 4096, NULL, TEMPO_TASK_PRIORITY, &tempo_task_handle);
};