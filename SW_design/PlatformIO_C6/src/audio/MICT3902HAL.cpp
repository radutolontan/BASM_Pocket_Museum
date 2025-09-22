#include "audio/MICT3902HAL.h"
#include "shared_resources/globals.h"
#include <driver/i2s_std.h>
#include <Arduino.h>
#include <cmath>

// Handle for the I2S channel
static i2s_chan_handle_t rx_handle = nullptr;

MICT3902HAL::MICT3902HAL(i2s_port_t i2s_num) : i2s_num(i2s_num) {}

MICT3902HAL::~MICT3902HAL() {
    if (rx_handle) {
        i2s_channel_disable(rx_handle);
        i2s_del_channel(rx_handle);
    }
}

void MICT3902HAL::init() {
    // === 1. Configure channel ===
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_num, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, nullptr, &rx_handle));
    

    // === 2. Configure standard mode ===
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,                            // No master clock
            .bclk = static_cast<gpio_num_t>(I2S_CLK_PIN),       // Bit clock (BCK)
            .ws   = I2S_GPIO_UNUSED,                            // WS/LRCLK not connected (mic R-CH configured)
            .dout = I2S_GPIO_UNUSED,                            // No DATA_OUT
            .din  = static_cast<gpio_num_t>(I2S_DATA_PIN)       // DATA_IN (SD)
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));

    // === 3. Enable channel ===
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    Serial.println("[MICT3902HAL] Initialized MICT3902 microphone with i2s_std driver.");
}

void MICT3902HAL::readBuffer(int16_t* buffer, size_t len) {
    size_t bytes_read = 0;
    ESP_ERROR_CHECK(
        i2s_channel_read(rx_handle, (void*)buffer, len * sizeof(int16_t), &bytes_read, portMAX_DELAY)
    );
}

float MICT3902HAL::computeRMSdB(int16_t* buffer, size_t len) {
    int64_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += static_cast<int64_t>(buffer[i]) * buffer[i];
    }
    float rms = sqrt(sum / static_cast<float>(len));
    float rms_dB = 20.0f * log10(rms / 32768.0f);
    return rms_dB;
}
