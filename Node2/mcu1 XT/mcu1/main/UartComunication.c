#include <stdio.h>
#include "../Lib/UartComunication.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"
static const char *TAG = "Uart_Comuniocation";

void uart_init(int tx_pin, int rx_pin)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));

    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0));
}

void uart_send_data(const char* data, size_t length)
{
    if (data == NULL || length == 0) {
        ESP_LOGE(TAG, "Invalid data or length");
        return;
    }

    int txBytes = uart_write_bytes(UART_NUM, data, length);
    if (txBytes != length) {
        ESP_LOGE(TAG, "Failed to write all bytes to UART");
    } else {
        ESP_LOGI(TAG, "Transmitted %d bytes: %s", txBytes, data);
    }
}

void uart_receive_data(char* data, size_t max_length)
{
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM, (size_t*)&length));
    if (length > 0) {
        int read_bytes = uart_read_bytes(UART_NUM, (uint8_t*)data, max_length, pdMS_TO_TICKS(5));
        if (read_bytes > 0) {
            data[read_bytes] = '\0'; // Null-terminate the received data
            
            ESP_LOGI(TAG, "Received %d bytes: %s", read_bytes, data);
        } else {
            ESP_LOGE(TAG, "UART read error");
        }
    } else {
        ESP_LOGW(TAG, "No data available to read");
    }
}

