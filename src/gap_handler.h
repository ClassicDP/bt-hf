#ifndef GAP_HANDLER_H
#define GAP_HANDLER_H

#include "esp_gap_bt_api.h"

extern bool found;
extern esp_bd_addr_t target_addr;

void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

#endif // GAP_HANDLER_H
