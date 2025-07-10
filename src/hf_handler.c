#include "hf_handler.h"
#include "esp_log.h"

static const char *TAG = "HF_AG_CONNECT";

void hf_ag_event_handler(esp_hf_cb_event_t event, esp_hf_cb_param_t *param) {
    switch (event) {
        case ESP_HF_CONNECTION_STATE_EVT:
            ESP_LOGI(TAG, "HF Connection state: %d", param->conn_stat.state);
            break;
        case ESP_HF_AUDIO_STATE_EVT:
            ESP_LOGI(TAG, "HF Audio state: %d", param->audio_stat.state);
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
            ESP_LOGI(TAG, "⚙️  Unknown AT command response");
            break;
        case ESP_HF_IND_UPDATE_EVT:
            ESP_LOGI(TAG, "📟 Indicator Update");
            break;
        case ESP_HF_CIND_RESPONSE_EVT:
            ESP_LOGI(TAG, "📱 Response");
            break;
        case ESP_HF_COPS_RESPONSE_EVT:
            ESP_LOGI(TAG, "🏢 Response");
            break;
        case ESP_HF_CLCC_RESPONSE_EVT:
            ESP_LOGI(TAG, "📞 Response");
            break;
        case ESP_HF_CNUM_RESPONSE_EVT:
            ESP_LOGI(TAG, "👤 Response");
            break;
        case ESP_HF_VTS_RESPONSE_EVT:
            ESP_LOGI(TAG, "🎹 Response");
            break;
        case ESP_HF_NREC_RESPONSE_EVT:
            ESP_LOGI(TAG, "🔇 NREC response");
            break;
        case ESP_HF_WBS_RESPONSE_EVT:
            ESP_LOGI(TAG, "🎧 Response");
            break;
        case ESP_HF_BCS_RESPONSE_EVT:
            ESP_LOGI(TAG, "🎼 Response");
            break;
        case ESP_HF_PKT_STAT_NUMS_GET_EVT:
            ESP_LOGI(TAG, "📦 Packet stat requested");
            break;
        default:
            break;
    }
}
