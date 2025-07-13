#include "gap_handler.h"
#include "esp_log.h"
#include "esp_bt_device.h"
#include <string.h>
#include "esp_hf_ag_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"

static const char *TAG = "GAP_HANDLER";
static esp_bd_addr_t target_addr;
static const char *target_name = NULL;
static bool found = false;
static bool connection_in_progress = false;
static bool is_connected = false;

void gap_set_target_name(const char *name) {
    target_name = name;
    ESP_LOGI(TAG, "Target device name set to: %s", target_name);
}

void gap_reset_connection_state() {
    ESP_LOGI(TAG, "Resetting connection state");
    found = false;
    connection_in_progress = false;
    is_connected = false;
}

void gap_set_connection_status(bool connected) {
    is_connected = connected;
    if (connected) {
        connection_in_progress = false;
    }
    ESP_LOGI(TAG, "Connection status set to: %s", connected ? "Connected" : "Disconnected");
}

void gap_start_discovery() {
    if (connection_in_progress || is_connected) {
        ESP_LOGW(TAG, "Discovery skipped - connection in progress or already connected");
        return;
    }
    
    ESP_LOGI(TAG, "Starting device discovery...");
    esp_err_t ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start discovery: %s", esp_err_to_name(ret));
    }
}

void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
        if (target_name == NULL) {
            ESP_LOGE(TAG, "Target name not set! Call gap_set_target_name() first.");
            return;
        }
        
        uint8_t *bd_addr = param->disc_res.bda;
        ESP_LOGI(TAG, "Device found: %02x:%02x:%02x:%02x:%02x:%02x", 
                 bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
        
        for (int i = 0; i < param->disc_res.num_prop; i++) {
            if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR) {
                uint8_t *eir = param->disc_res.prop[i].val;
                char name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
                uint8_t len = 0;
                uint8_t *name_ptr = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);
                
                if (name_ptr && len < sizeof(name)) {
                    memcpy(name, name_ptr, len);
                    name[len] = '\0';
                    ESP_LOGI(TAG, "Device name: %s", name);
                    
                    if (strstr(name, target_name)) {
                        ESP_LOGI(TAG, "🎯 Target device found: %s", name);
                        memcpy(target_addr, bd_addr, ESP_BD_ADDR_LEN);
                        found = true;
                        
                        // Stop discovery
                        esp_bt_gap_cancel_discovery();
                        
                        if (connection_in_progress) {
                            ESP_LOGW(TAG, "Connection already in progress, skipping");
                            return;
                        }
                        
                        connection_in_progress = true;
                        
                        // Small delay before connection
                        vTaskDelay(pdMS_TO_TICKS(1000));
                        
                        // Initiate HF AG connection
                        esp_err_t ret = esp_hf_ag_slc_connect(target_addr);
                        if (ret != ESP_OK) {
                            ESP_LOGE(TAG, "Failed to connect HF AG: %s", esp_err_to_name(ret));
                            connection_in_progress = false;
                        } else {
                            ESP_LOGI(TAG, "HF AG connection initiated");
                        }
                    }
                }
            }
        }
        break;
    }
    
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            ESP_LOGI(TAG, "Discovery stopped");
            if (!found) {
                ESP_LOGW(TAG, "Target device not found. Retrying in 5 seconds...");
                vTaskDelay(pdMS_TO_TICKS(5000));
                gap_start_discovery();
            }
        } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
            ESP_LOGI(TAG, "Discovery started");
        }
        break;
    }
    
    case ESP_BT_GAP_PIN_REQ_EVT: {
        ESP_LOGI(TAG, "[PAIRING] PIN request from %02x:%02x:%02x:%02x:%02x:%02x",
                 param->pin_req.bda[0], param->pin_req.bda[1], param->pin_req.bda[2],
                 param->pin_req.bda[3], param->pin_req.bda[4], param->pin_req.bda[5]);
        esp_bt_pin_code_t pin_code = {'0', '0', '0', '0'};
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        break;
    }
    
    case ESP_BT_GAP_CFM_REQ_EVT: {
        ESP_LOGI(TAG, "[PAIRING] SSP confirm request, code: %lu", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    }
    
    case ESP_BT_GAP_KEY_NOTIF_EVT: {
        ESP_LOGI(TAG, "[PAIRING] Key notification, passkey: %lu", param->key_notif.passkey);
        break;
    }
    
    case ESP_BT_GAP_AUTH_CMPL_EVT: {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "[PAIRING] ✅ Authentication successful");
        } else {
            ESP_LOGE(TAG, "[PAIRING] ❌ Authentication failed, status: %d", param->auth_cmpl.stat);
        }
        break;
    }
    
    case ESP_BT_GAP_RMT_SRVCS_EVT: {
        ESP_LOGI(TAG, "Remote services discovery result");
        if (param->rmt_srvcs.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(TAG, "✅ Found %d services", param->rmt_srvcs.num_uuids);
            for (int i = 0; i < param->rmt_srvcs.num_uuids; i++) {
                ESP_LOGI(TAG, "Service %d: UUID 0x%04x", i, param->rmt_srvcs.uuid_list[i].uuid.uuid16);
            }
        } else {
            ESP_LOGE(TAG, "❌ Service discovery failed, status: %d", param->rmt_srvcs.stat);
        }
        break;
    }
    
    default:
        ESP_LOGD(TAG, "GAP event: %d", event);
        break;
    }
}
