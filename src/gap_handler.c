#include "gap_handler.h"
#include "auto_reconnect.h"
#include "paired_devices.h"
#include "esp_log.h"
#include "esp_gap_bt_api.h"
#include "esp_hf_ag_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "GAP_HANDLER";

static char target_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1] = {0};
static esp_bd_addr_t target_addr = {0};
static bool found = false;
static bool connection_in_progress = false;

void gap_set_target_name(const char *name) {
    if (name && strlen(name) < sizeof(target_name)) {
        strncpy(target_name, name, sizeof(target_name) - 1);
        target_name[sizeof(target_name) - 1] = '\0';
        ESP_LOGI(TAG, "Target device name set to: %s", target_name);
    }
}

void gap_reset_connection_state() {
    connection_in_progress = false;
    found = false;
}

void gap_set_connection_status(bool connected) {
    connection_in_progress = connected;
}

void gap_start_discovery() {
    ESP_LOGI(TAG, "Starting device discovery");
    esp_err_t ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start discovery: %s", esp_err_to_name(ret));
    }
}

void gap_try_reconnect_to_last_device() {
    ESP_LOGI(TAG, "Trying to reconnect to last device");
    auto_reconnect_start();
}

void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (param == NULL) {
        ESP_LOGE(TAG, "GAP callback param is NULL");
        return;
    }

    switch (event) {
        case ESP_BT_GAP_DISC_RES_EVT: {
            uint8_t *bd_addr = param->disc_res.bda;
            ESP_LOGI(TAG, "Device found: " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bd_addr));
            
            char device_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1] = {0};
            uint32_t cod = 0;
            bool is_hf_device = false;
            
            // Парсим свойства устройства
            for (int i = 0; i < param->disc_res.num_prop; i++) {
                if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR) {
                    uint8_t *eir = param->disc_res.prop[i].val;
                    uint8_t len = 0;
                    uint8_t *name_ptr = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);
                    
                    if (name_ptr && len < sizeof(device_name)) {
                        memcpy(device_name, name_ptr, len);
                        device_name[len] = '\0';
                        ESP_LOGI(TAG, "Device name: %s", device_name);
                    }
                } else if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_COD) {
                    cod = *(uint32_t*)(param->disc_res.prop[i].val);
                    uint32_t major_class = (cod & 0x1F00) >> 8;
                    if (major_class == 0x04) { // Audio/Video major class
                        is_hf_device = true;
                    }
                }
            }

            // Проверяем целевое устройство
            if (target_name[0] && strlen(device_name) > 0 && strstr(device_name, target_name)) {
                ESP_LOGI(TAG, "🎯 Target device found: %s", device_name);
                memcpy(target_addr, bd_addr, ESP_BD_ADDR_LEN);
                found = true;
                
                // Добавляем в список сопряженных устройств
                paired_devices_add(bd_addr, device_name, cod, is_hf_device);
                
                // Останавливаем поиск
                esp_bt_gap_cancel_discovery();
                
                if (!connection_in_progress) {
                    connection_in_progress = true;
                    
                    // Небольшая задержка перед подключением
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    
                    // Инициируем HF AG соединение
                    esp_err_t ret = esp_hf_ag_slc_connect(target_addr);
                    if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to connect HF AG: %s", esp_err_to_name(ret));
                        connection_in_progress = false;
                        auto_reconnect_notify_connection_failed();
                    }
                }
            }
            break;
        }
        
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
            ESP_LOGI(TAG, "Discovery state changed: %d", param->disc_st_chg.state);
            if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
                ESP_LOGI(TAG, "Discovery stopped");
                auto_reconnect_notify_discovery_complete();
            }
            break;
        }
        
        case ESP_BT_GAP_PIN_REQ_EVT: {
            ESP_LOGI(TAG, "PIN request for device " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(param->pin_req.bda));
            esp_bt_pin_code_t pin_code = {0};
            strncpy((char*)pin_code, "0000", 4);
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
            break;
        }
        
        case ESP_BT_GAP_CFM_REQ_EVT: {
            ESP_LOGI(TAG, "Confirmation request for device " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(param->cfm_req.bda));
            esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
            break;
        }
        
        case ESP_BT_GAP_KEY_NOTIF_EVT: {
            ESP_LOGI(TAG, "Key notification for device " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(param->key_notif.bda));
            break;
        }
        
        case ESP_BT_GAP_AUTH_CMPL_EVT: {
            ESP_LOGI(TAG, "Authentication complete for device " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(param->auth_cmpl.bda));
            if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "Authentication successful");
            } else {
                ESP_LOGE(TAG, "Authentication failed: %d", param->auth_cmpl.stat);
            }
            break;
        }
        
        default:
            ESP_LOGD(TAG, "Unhandled GAP event: %d", event);
            break;
    }
}
