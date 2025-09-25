#include "audio/MICT3902HAL.h"
#include "shared_resources/globals.h"
#include <driver/i2s_pdm.h>   // PDM RX driver
#include <Arduino.h>
#include <cmath>

// Handle for the PDM RX channel
static i2s_chan_handle_t rx_handle = nullptr;

constexpr size_t DECIMATION = 24;    // Number of PDM bits per PCM sample
constexpr size_t FIR_LEN    = 8;    // FIR smoothing length

// FIR history buffer
static int32_t fir_buffer[FIR_LEN] = {0};
static size_t fir_index = 0;

MICT3902HAL::MICT3902HAL(i2s_port_t i2s_num) : i2s_num(i2s_num) {}

MICT3902HAL::~MICT3902HAL() {
    if (rx_handle) {
        i2s_channel_disable(rx_handle);
        i2s_del_channel(rx_handle);
    }
}

void MICT3902HAL::init() {
    // === 1. Create new channel ===
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_num, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, nullptr, &rx_handle));

    // === 2. Configure PDM RX ===
    i2s_pdm_rx_config_t pdm_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_MONO
        ),
        .gpio_cfg = {
            .clk = static_cast<gpio_num_t>(I2S_CLK_PIN),   // PDM clock
            .din = static_cast<gpio_num_t>(I2S_DATA_PIN),  // PDM data
            .invert_flags = {
                .clk_inv = false
            }
        }
    };

    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_handle, &pdm_cfg));

    // === 3. Enable channel ===
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    Serial.println("[MICT3902HAL] Initialized MICT3902 microphone with PDM RX driver (software decimation).");
}

void MICT3902HAL::readBuffer(int16_t* buffer, size_t pcm_len) {
    if (!buffer || pcm_len == 0) return;

    // --- Allocate temporary PDM raw buffer (16-bit words) ---
    const size_t pdm_words = pcm_len * DECIMATION;   // number of 16-bit words to capture
    int16_t* pdm_raw = (int16_t*)heap_caps_malloc(pdm_words * sizeof(int16_t), MALLOC_CAP_INTERNAL);
    if (!pdm_raw) {
        Serial.println("[MICT3902HAL] ERROR: Out of memory for PDM buffer");
        return;
    }

    size_t bytes_read = 0;
    esp_err_t err = i2s_channel_read(rx_handle, (void*)pdm_raw, pdm_words * sizeof(int16_t), &bytes_read, portMAX_DELAY);

    if (err != ESP_OK) {
        Serial.printf("[MICT3902HAL] Error reading PDM RX: %d\n", err);
        heap_caps_free(pdm_raw);
        return;
    }

    // --- Software PDM → PCM decimation ---
    for (size_t i = 0; i < pcm_len; ++i) {
        int32_t sum = 0;

        // Integrate DECIMATION raw PDM words into one PCM sample
        for (size_t j = 0; j < DECIMATION; ++j) {
            int16_t word = pdm_raw[i * DECIMATION + j];
            // Each word is 16 bits → expand into +/-1 contributions
            for (int b = 0; b < 16; ++b) {
                bool bit = (word >> b) & 1;
                sum += bit ? 1 : -1;
            }
        }

        // Store in FIR history
        fir_buffer[fir_index] = sum;
        fir_index = (fir_index + 1) % FIR_LEN;

        // FIR smoothing
        int32_t fir_sum = 0;
        for (size_t k = 0; k < FIR_LEN; ++k) {
            fir_sum += fir_buffer[k];
        }

        // Normalize to 16-bit PCM
        buffer[i] = (int16_t)((fir_sum * 32767) / (DECIMATION * 16 * FIR_LEN));
    }
    
    heap_caps_free(pdm_raw);
}

float MICT3902HAL::computeRMSdB(int16_t* buffer, size_t len) {
    int64_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += (int64_t)buffer[i] * buffer[i];
    }
    float rms = sqrt(sum / (float)len);
    return 20.0f * log10(rms / 32768.0f);
}
