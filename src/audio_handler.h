#ifndef AUDIO_HANDLER_H
#define AUDIO_HANDLER_H

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "esp_hf_ag_api.h"
#include "esp_hf_defs.h"

/**
 * @brief Инициализация аудио обработчика для HCI data path
 */
void audio_handler_init(void);

/**
 * @brief Установка состояния аудио соединения
 * @param connected Статус соединения (true = подключено, false = отключено)
 * @param sync_conn_hdl Дескриптор SCO соединения
 * @param msbc_mode Режим кодека (true = mSBC, false = CVSD)
 */
void audio_handler_set_connection_state(bool connected, uint16_t sync_conn_hdl, bool msbc_mode);

/**
 * @brief Отправка тестового аудио сигнала
 */
void audio_handler_send_test_audio(void);

/**
 * @brief Проверка состояния аудио соединения
 * @return true если аудио соединение активно, false иначе
 */
bool audio_handler_is_connected(void);

#endif // AUDIO_HANDLER_H
