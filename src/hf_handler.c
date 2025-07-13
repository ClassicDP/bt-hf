#include "hf_handler.h"
#include "auto_reconnect.h"
#include "paired_devices.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "HF_HANDLER";

esp_bd_addr_t hf_peer_addr = {0};

void hf_ag_event_handler(esp_hf_cb_event_t event, esp_hf_cb_param_t *param) {
    if (param == NULL) {
        ESP_LOGE(TAG, "HF AG event handler param is NULL");
        return;
    }

    switch (event) {
        case ESP_HF_CONNECTION_STATE_EVT:
            ESP_LOGI(TAG, "HF connection state: %d", param->conn_stat.state);
            if (param->conn_stat.state == ESP_HF_CONNECTION_STATE_CONNECTED) {
                ESP_LOGI(TAG, "HF connected to " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(param->conn_stat.remote_bda));
                memcpy(hf_peer_addr, param->conn_stat.remote_bda, sizeof(esp_bd_addr_t));
                
                // Добавляем устройство в список сопряженных
                paired_devices_add(param->conn_stat.remote_bda, "HF Device", 0x200408, true);
                
                // Уведомляем модуль автоматического переподключения
                auto_reconnect_notify_connection_state(true);
            } else if (param->conn_stat.state == ESP_HF_CONNECTION_STATE_DISCONNECTED) {
                ESP_LOGI(TAG, "HF disconnected");
                memset(hf_peer_addr, 0, sizeof(esp_bd_addr_t));
                
                // Уведомляем модуль автоматического переподключения
                auto_reconnect_notify_connection_state(false);
            }
            break;

        case ESP_HF_AUDIO_STATE_EVT:
            ESP_LOGI(TAG, "HF audio state: %d", param->audio_stat.state);
            break;

        case ESP_HF_VOLUME_CONTROL_EVT:
            ESP_LOGI(TAG, "Volume control: type=%d, volume=%d", param->volume_control.type, param->volume_control.volume);
            break;

        default:
            ESP_LOGW(TAG, "Unhandled HF event: %d", event);
            break;
    }
}
