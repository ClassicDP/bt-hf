#include "gap_handler.h"
#include "esp_log.h"
#include "esp_bt_device.h"
#include <string.h>
#include "esp_hf_ag_api.h"

#define TARGET_NAME "Jabra"  // замените на имя вашей гарнитуры

const char *TAG = "HF_AG_CONNECT";
esp_bd_addr_t target_addr;
bool found = false;

void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (event == ESP_BT_GAP_DISC_RES_EVT) {
        uint8_t *bd_addr = param->disc_res.bda;
        for (int i = 0; i < param->disc_res.num_prop; i++) {
            if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR) {
                uint8_t *eir = param->disc_res.prop[i].val;
                char name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
                {
                    uint8_t len = 0;
                    uint8_t *name_ptr = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);
                    if (name_ptr && len < sizeof(name)) {
                        memcpy(name, name_ptr, len);
                        name[len] = '\0';
                        ESP_LOGI(TAG, "Found device: %s", name);
                        if (strstr(name, TARGET_NAME)) {
                            ESP_LOGI(TAG, "🎯 Target found: %s", name);
                            memcpy(target_addr, bd_addr, ESP_BD_ADDR_LEN);
                            found = true;
                            esp_bt_gap_cancel_discovery();
                            esp_hf_ag_slc_connect(target_addr);
                        }
                    }
                }
            }
        }
    } else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED && !found) {
            ESP_LOGW(TAG, "❌ Target device not found");
        }
    }
}
