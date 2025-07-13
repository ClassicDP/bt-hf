#include "console_handler.h"
#include "audio_handler.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "CONSOLE";

void console_handler_init(void)
{
    ESP_LOGI(TAG, "Console handler initialized");
    ESP_LOGI(TAG, "Available commands:");
    ESP_LOGI(TAG, "  'test_audio' - Send test audio signal");
    ESP_LOGI(TAG, "  'audio_status' - Check audio connection status");
}

void console_handler_process_command(const char *command)
{
    if (strncmp(command, "test_audio", 10) == 0) {
        ESP_LOGI(TAG, "🔊 Sending test audio signal...");
        audio_handler_send_test_audio();
    } else if (strncmp(command, "audio_status", 12) == 0) {
        bool connected = audio_handler_is_connected();
        ESP_LOGI(TAG, "🎙️ Audio status: %s", connected ? "CONNECTED" : "DISCONNECTED");
    } else {
        ESP_LOGW(TAG, "Unknown command: %s", command);
    }
}
