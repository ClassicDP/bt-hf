#ifndef GAP_HANDLER_H
#define GAP_HANDLER_H

#include "esp_gap_bt_api.h"

void gap_set_target_name(const char *name);
void gap_reset_connection_state();
void gap_set_connection_status(bool connected);
void gap_start_discovery();
void gap_try_reconnect_to_last_device();
void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

#endif // GAP_HANDLER_H
