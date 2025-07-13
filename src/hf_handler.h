#ifndef HF_HANDLER_H
#define HF_HANDLER_H

#include "esp_hf_ag_api.h"
#include "esp_bt_defs.h"

// Экспортированные переменные
extern esp_bd_addr_t hf_peer_addr;

// Функции
void hf_ag_event_handler(esp_hf_cb_event_t event, esp_hf_cb_param_t *param);

#endif // HF_HANDLER_H
