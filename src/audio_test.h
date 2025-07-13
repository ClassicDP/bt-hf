#ifndef AUDIO_TEST_H
#define AUDIO_TEST_H

#include "esp_hf_ag_api.h"
#include "esp_bt_defs.h"

// Структура для статистики аудио
typedef struct {
    uint32_t sco_connections;
    uint32_t sco_disconnections;
    uint32_t audio_packets_sent;
    uint32_t audio_packets_received;
    uint32_t codec_switches;
    bool is_msbc_active;
    bool is_sco_active;
} audio_stats_t;

// Функции для тестирования аудио
void audio_test_init(void);
void audio_test_start_monitoring(void);
void audio_test_stop_monitoring(void);
void audio_test_print_stats(void);
void audio_test_reset_stats(void);

// Тестовые функции для звука
esp_err_t audio_test_connect_sco(esp_bd_addr_t remote_addr);
esp_err_t audio_test_disconnect_sco(esp_bd_addr_t remote_addr);
esp_err_t audio_test_send_tone(esp_bd_addr_t remote_addr, uint16_t frequency, uint32_t duration_ms);
esp_err_t audio_test_volume_control(esp_bd_addr_t remote_addr, int volume);

// Обработчики событий аудио
void audio_test_handle_sco_connected(void);
void audio_test_handle_sco_disconnected(void);
void audio_test_handle_codec_change(bool is_msbc);

// Получение статистики
audio_stats_t* audio_test_get_stats(void);

#endif // AUDIO_TEST_H
