#include "hf_handler.h"
#include "gap_handler.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <inttypes.h>
#include "esp_hf_ag_api.h"

static const char *TAG = "HF_AG_CONNECT";
static int connection_attempt_count = 0;
static bool connection_established = false;
static esp_bd_addr_t connected_device_addr;

// Экспортированная переменная для main.c
esp_bd_addr_t hf_peer_addr;

// Функция для поддержания соединения
static void maintain_connection_task(void *param) {
    while (connection_established) {
        ESP_LOGI(TAG, "💓 Heartbeat: Maintaining HFP connection...");
        vTaskDelay(pdMS_TO_TICKS(30000)); // Каждые 30 секунд
    }
    vTaskDelete(NULL);
}

// Массивы строк для отладки - как в официальном примере
static const char *c_hf_evt_str[] = {
    "CONNECTION_STATE_EVT",
    "AUDIO_STATE_EVT", 
    "VR_STATE_CHANGE_EVT",
    "VOLUME_CONTROL_EVT",
    "UNKNOW_AT_CMD",
    "IND_UPDATE",
    "CIND_RESPONSE_EVT",
    "COPS_RESPONSE_EVT",
    "CLCC_RESPONSE_EVT",
    "CNUM_RESPONSE_EVT",
    "DTMF_RESPONSE_EVT",
    "NREC_RESPONSE_EVT",
    "ANSWER_INCOMING_EVT",
    "REJECT_INCOMING_EVT", 
    "DIAL_EVT",
    "WBS_EVT",
    "BCS_EVT",
    "PKT_STAT_EVT",
    "PROF_STATE_EVT",
};

static const char *c_connection_state_str[] = {
    "DISCONNECTED",
    "CONNECTING", 
    "CONNECTED",
    "SLC_CONNECTED",
    "DISCONNECTING",
};

static const char *c_audio_state_str[] = {
    "disconnected",
    "connecting",
    "connected",
    "connected_msbc",
};

void hf_ag_event_handler(esp_hf_cb_event_t event, esp_hf_cb_param_t *param) {
    // Логирование событий как в официальном примере
    if (event <= ESP_HF_PROF_STATE_EVT) {
        ESP_LOGI(TAG, "APP HFP event: %s", c_hf_evt_str[event]);
    } else {
        ESP_LOGE(TAG, "APP HFP invalid event %d", event);
    }

    switch (event) {
        case ESP_HF_CONNECTION_STATE_EVT:
            ESP_LOGI(TAG, "--connection state %s, peer feats 0x%"PRIx32", chld_feats 0x%"PRIx32,
                     c_connection_state_str[param->conn_stat.state],
                     param->conn_stat.peer_feat,
                     param->conn_stat.chld_feat);
            
            // Сохраняем адрес устройства
            memcpy(connected_device_addr, param->conn_stat.remote_bda, sizeof(esp_bd_addr_t));
            
            switch (param->conn_stat.state) {
                case ESP_HF_CONNECTION_STATE_DISCONNECTED:
                    ESP_LOGW(TAG, "🔌 HF Disconnected");
                    connection_established = false;
                    gap_set_connection_status(false);
                    gap_reset_connection_state();
                    
                    ESP_LOGI(TAG, "⏳ Waiting 5 seconds before restarting discovery...");
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    gap_start_discovery();
                    break;
                    
                case ESP_HF_CONNECTION_STATE_CONNECTING:
                    ESP_LOGI(TAG, "🔄 HF Connecting...");
                    break;
                    
                case ESP_HF_CONNECTION_STATE_CONNECTED:
                    connection_attempt_count++;
                    ESP_LOGI(TAG, "✅ HF Connected (attempt #%d)", connection_attempt_count);
                    connection_established = true;
                    gap_set_connection_status(true);
                    ESP_LOGI(TAG, "� HF connection established, waiting for SLC...");
                    break;
                    
                case ESP_HF_CONNECTION_STATE_SLC_CONNECTED:
                    ESP_LOGI(TAG, "� HF SLC Connected - Full HFP functionality available");
                    
                    // Запускаем задачу поддержания соединения
                    xTaskCreate(maintain_connection_task, "maintain_conn", 2048, NULL, 5, NULL);
                    
                    // Небольшая задержка для стабилизации
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    
                    // Пытаемся установить аудио соединение
                    ESP_LOGI(TAG, "🔊 Opening audio connection (SCO)...");
                    esp_err_t audio_result = esp_hf_ag_audio_connect(param->conn_stat.remote_bda);
                    ESP_LOGI(TAG, "Audio connection result: %s", esp_err_to_name(audio_result));
                    break;
                    
                case ESP_HF_CONNECTION_STATE_DISCONNECTING:
                    ESP_LOGI(TAG, "🔄 HF Disconnecting...");
                    break;
                    
                default:
                    ESP_LOGI(TAG, "HF Unknown state: %d", param->conn_stat.state);
                    break;
            }
            break;
            
        case ESP_HF_AUDIO_STATE_EVT:
            ESP_LOGI(TAG, "--Audio State %s", c_audio_state_str[param->audio_stat.state]);
            switch (param->audio_stat.state) {
                case ESP_HF_AUDIO_STATE_DISCONNECTED:
                    ESP_LOGW(TAG, "🔇 SCO Disconnected");
                    break;
                case ESP_HF_AUDIO_STATE_CONNECTING:
                    ESP_LOGI(TAG, "🔊 SCO Connecting...");
                    break;
                case ESP_HF_AUDIO_STATE_CONNECTED:
                    ESP_LOGI(TAG, "🔊 SCO Connected - Audio channel is now open!");
                    ESP_LOGI(TAG, "🎉 SUCCESS: Full HFP connection with audio established!");
                    break;
                case ESP_HF_AUDIO_STATE_CONNECTED_MSBC:
                    ESP_LOGI(TAG, "🔊 SCO Connected with mSBC codec - High quality audio!");
                    ESP_LOGI(TAG, "🎉 SUCCESS: Full HFP connection with mSBC audio established!");
                    break;
                default:
                    ESP_LOGI(TAG, "SCO Unknown state: %d", param->audio_stat.state);
                    break;
            }
            break;
            
        case ESP_HF_VOLUME_CONTROL_EVT:
            ESP_LOGI(TAG, "🔊 Volume control: volume=%d", param->volume_control.volume);
            break;
            
        case ESP_HF_BVRA_RESPONSE_EVT:
            ESP_LOGI(TAG, "🎙️ Voice Recognition response");
            break;
            
        case ESP_HF_ATA_RESPONSE_EVT:
            ESP_LOGI(TAG, "📞 Answer incoming call");
            break;
            
        case ESP_HF_CHUP_RESPONSE_EVT:
            ESP_LOGI(TAG, "❌ Reject call");
            break;
            
        case ESP_HF_UNAT_RESPONSE_EVT:
            ESP_LOGI(TAG, "--UNKNOWN AT CMD: %s", param->unat_rep.unat ? param->unat_rep.unat : "NULL");
            // Отправляем стандартный ответ как в официальном примере
            esp_hf_ag_unknown_at_send(param->unat_rep.remote_addr, NULL);
            break;
            
        case ESP_HF_IND_UPDATE_EVT:
            ESP_LOGI(TAG, "--UPDATE INDICATOR!");
            // Отправляем обновления индикаторов как в официальном примере
            {
                esp_hf_call_status_t call_state = 1;
                esp_hf_call_setup_status_t call_setup_state = 2;
                esp_hf_network_state_t ntk_state = 1;
                int signal = 2;
                int battery = 3;
                esp_hf_ag_ciev_report(param->ind_upd.remote_addr, ESP_HF_IND_TYPE_CALL, call_state);
                esp_hf_ag_ciev_report(param->ind_upd.remote_addr, ESP_HF_IND_TYPE_CALLSETUP, call_setup_state);
                esp_hf_ag_ciev_report(param->ind_upd.remote_addr, ESP_HF_IND_TYPE_SERVICE, ntk_state);
                esp_hf_ag_ciev_report(param->ind_upd.remote_addr, ESP_HF_IND_TYPE_SIGNAL, signal);
                esp_hf_ag_ciev_report(param->ind_upd.remote_addr, ESP_HF_IND_TYPE_BATTCHG, battery);
            }
            break;
            
        case ESP_HF_CIND_RESPONSE_EVT:
            ESP_LOGI(TAG, "--CIND Start.");
            // Отправляем стандартные индикаторы статуса как в официальном примере
            esp_hf_call_status_t call_status = 0;
            esp_hf_call_setup_status_t call_setup_status = 0;
            esp_hf_network_state_t ntk_state = 1;
            int signal = 4;
            esp_hf_roaming_status_t roam = 0;
            int batt_lev = 3;
            esp_hf_call_held_status_t call_held_status = 0;
            esp_hf_ag_cind_response(param->cind_rep.remote_addr, call_status, call_setup_status, ntk_state, signal, roam, batt_lev, call_held_status);
            break;
            
        case ESP_HF_CNUM_RESPONSE_EVT:
            ESP_LOGI(TAG, "📱 CNUM Response - Subscriber Information");
            break;
            
        case ESP_HF_VTS_RESPONSE_EVT:
            ESP_LOGI(TAG, "🔢 VTS Response - DTMF");
            break;
            
        case ESP_HF_COPS_RESPONSE_EVT:
            ESP_LOGI(TAG, "📡 COPS Response - Operator Information");
            break;
            
        case ESP_HF_CLCC_RESPONSE_EVT:
            ESP_LOGI(TAG, "📞 CLCC Response - Current Calls");
            break;
            
        case ESP_HF_DIAL_EVT:
            ESP_LOGI(TAG, "☎️ Dial Event - Outgoing Call");
            break;
            
        case ESP_HF_NREC_RESPONSE_EVT:
            ESP_LOGI(TAG, "🎛️ NREC Response");
            break;
            
        case ESP_HF_WBS_RESPONSE_EVT:
            ESP_LOGI(TAG, "📻 WBS Response: codec=%d", param->wbs_rep.codec);
            break;
            
        case ESP_HF_BCS_RESPONSE_EVT:
            ESP_LOGI(TAG, "🎵 BCS Response");
            break;
            
        case ESP_HF_PKT_STAT_NUMS_GET_EVT:
            ESP_LOGI(TAG, "📈 Packet Statistics Event");
            break;
            
        case ESP_HF_PROF_STATE_EVT:
            ESP_LOGI(TAG, "🔧 HF Profile State Event");
            break;
            
        default:
            ESP_LOGI(TAG, "⚠️ Unhandled HF event: %d", event);
            break;
    }
}
