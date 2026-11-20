#include "rg_system.h"
#include "rg_audio.h"

#if RG_AUDIO_USE_INT_DAC || RG_AUDIO_USE_EXT_DAC

#ifndef ESP_PLATFORM
#error "I2S support can only be built inside esp-idf!"
#elif !CONFIG_IDF_TARGET_ESP32 && RG_AUDIO_USE_INT_DAC
#error "Your chip has no DAC! Please set RG_AUDIO_USE_INT_DAC to 0 in your target file."
#endif

#include <driver/gpio.h>
#include <driver/i2s.h>

static struct {
    const char *last_error;
    int device;
    int volume;
    bool muted;
} state;

#ifdef RG_AUDIO_USE_ES8311
#include "rg_i2c.h"

#define ES8311_I2C_ADDRESS 0x18

// ES8311 DAC output level (register 0x32): 0.5 dB per step, 0xBF = 0 dB, 0xFF = +32 dB.
// Lowering it by 12 steps (6 dB) halves the output amplitude at every volume setting.
#ifndef RG_AUDIO_ES8311_DAC_VOLUME
#define RG_AUDIO_ES8311_DAC_VOLUME 0xFF
#endif

static bool es8311_configure_sample_rate(int sample_rate)
{
    if (sample_rate < 8000 || sample_rate > 96000)
    {
        state.last_error = "ES8311 supports sample rates from 8000 to 96000 Hz";
        return false;
    }

    // These dividers support the I2S driver's 256x-Fs MCLK for 16-bit stereo.
    static const uint8_t clock_registers[][2] = {
        {0x01, 0x3F}, {0x02, 0x00}, {0x03, 0x10}, {0x04, 0x10},
        {0x05, 0x00}, {0x06, 0x03}, {0x07, 0x00}, {0x08, 0xFF},
        {0x09, 0x0C}, {0x0A, 0x0C},
    };

    for (size_t i = 0; i < RG_COUNT(clock_registers); ++i)
    {
        if (!rg_i2c_write_byte(ES8311_I2C_ADDRESS, clock_registers[i][0], clock_registers[i][1]))
        {
            state.last_error = "Failed to configure ES8311 clock over I2C";
            return false;
        }
    }
    return true;
}

static bool es8311_init_codec(int sample_rate)
{
    static const uint8_t output_registers[][2] = {
        {0x0D, 0x01}, {0x0E, 0x02}, {0x12, 0x00}, {0x13, 0x10},
        {0x1C, 0x6A}, {0x32, RG_AUDIO_ES8311_DAC_VOLUME}, {0x37, 0x08},
    };

    if (!rg_i2c_init() ||
        !rg_i2c_write_byte(ES8311_I2C_ADDRESS, 0x00, 0x1F))
    {
        state.last_error = "Failed to reset ES8311 over I2C";
        return false;
    }
    rg_usleep(20000);
    if (!rg_i2c_write_byte(ES8311_I2C_ADDRESS, 0x00, 0x00) ||
        !rg_i2c_write_byte(ES8311_I2C_ADDRESS, 0x00, 0x80) ||
        !es8311_configure_sample_rate(sample_rate))
    {
        if (!state.last_error)
            state.last_error = "Failed to start ES8311";
        return false;
    }

    for (size_t i = 0; i < RG_COUNT(output_registers); ++i)
    {
        if (!rg_i2c_write_byte(ES8311_I2C_ADDRESS, output_registers[i][0], output_registers[i][1]))
        {
            state.last_error = "Failed to configure ES8311 output over I2C";
            return false;
        }
    }
    return true;
}
#endif

#ifdef RG_GPIO_SND_AMP_ENABLE_INVERT
#define MUTE_ENABLE 1
#define MUTE_DISABLE 0
#else
#define MUTE_ENABLE 0
#define MUTE_DISABLE 1
#endif

// We can safely assume that no application will submit more than 640 audio frames per call to
// driver_submit (32000/50). Using a single large buffer risks blocking the call needlessly because
// some apps submit more than once per cycle or there could be occasional jitter (early submission).
#define DMA_BUFFER_COUNT 4
#define DMA_BUFFER_LEN 180

static bool driver_init(int device, int sample_rate)
{
    esp_err_t ret = ESP_OK;
    bool driver_installed = false;

    state.last_error = NULL;
    state.device = device;

    #ifdef RG_GPIO_SND_AMP_ENABLE
    gpio_reset_pin(RG_GPIO_SND_AMP_ENABLE);
    gpio_set_level(RG_GPIO_SND_AMP_ENABLE, MUTE_ENABLE);
    gpio_set_direction(RG_GPIO_SND_AMP_ENABLE, GPIO_MODE_OUTPUT);
    #endif

    if (state.device == 0)
    {
    #if RG_AUDIO_USE_INT_DAC
        ret = i2s_driver_install(I2S_NUM_0, &(i2s_config_t){
            .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
            .sample_rate = sample_rate,
            .bits_per_sample = 16,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_MSB,
            .intr_alloc_flags = 0, // ESP_INTR_FLAG_LEVEL1
            .dma_buf_count = DMA_BUFFER_COUNT,
            .dma_buf_len = DMA_BUFFER_LEN,
        }, 0, NULL);
        if (ret == ESP_OK)
        {
            driver_installed = true;
            ret = i2s_set_dac_mode(RG_AUDIO_USE_INT_DAC);
        }
    #else
        state.last_error = "This device does not support internal DAC mode!";
    #endif
    }
    else if (state.device == 1)
    {
    #if RG_AUDIO_USE_EXT_DAC
        ret = i2s_driver_install(I2S_NUM_0, &(i2s_config_t){
            .mode = I2S_MODE_MASTER | I2S_MODE_TX,
            .sample_rate = sample_rate,
            .bits_per_sample = 16,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = 0, // ESP_INTR_FLAG_LEVEL1
            .dma_buf_count = DMA_BUFFER_COUNT,
            .dma_buf_len = DMA_BUFFER_LEN,
        #if CONFIG_IDF_TARGET_ESP32
            .use_apll = true, // External DAC may care about accuracy
        #endif
        }, 0, NULL);
        if (ret == ESP_OK)
        {
            driver_installed = true;
            ret = i2s_set_pin(I2S_NUM_0, &(i2s_pin_config_t) {
                .mck_io_num =
                #ifdef RG_AUDIO_USE_ES8311
                    RG_GPIO_SND_I2S_MCK,
                #else
                    GPIO_NUM_NC,
                #endif
                .bck_io_num = RG_GPIO_SND_I2S_BCK,
                .ws_io_num = RG_GPIO_SND_I2S_WS,
                .data_out_num = RG_GPIO_SND_I2S_DATA,
                .data_in_num = GPIO_NUM_NC
            });
        }
#ifdef RG_AUDIO_USE_ES8311
        if (ret == ESP_OK && !es8311_init_codec(sample_rate))
        {
            ret = ESP_FAIL;
        }
#endif
    #else
        state.last_error = "This device does not support external DAC mode!";
    #endif
    }
    else
    {
        state.last_error = "Invalid I2S device";
    }

    if (ret != ESP_OK)
    {
        if (!state.last_error)
            state.last_error = esp_err_to_name(ret);
        if (driver_installed)
            i2s_driver_uninstall(I2S_NUM_0);
        #ifdef RG_GPIO_SND_I2S_MCK
        gpio_reset_pin(RG_GPIO_SND_I2S_MCK);
        #endif
        gpio_reset_pin(RG_GPIO_SND_I2S_BCK);
        gpio_reset_pin(RG_GPIO_SND_I2S_DATA);
        gpio_reset_pin(RG_GPIO_SND_I2S_WS);
        #ifdef RG_GPIO_SND_AMP_ENABLE
        gpio_set_level(RG_GPIO_SND_AMP_ENABLE, MUTE_ENABLE);
        #endif
        return false;
    }
    return state.last_error == NULL;
}

static bool driver_set_sample_rates(int sampleRate)
{
    if (i2s_set_sample_rates(I2S_NUM_0, sampleRate) != ESP_OK)
        return false;
#ifdef RG_AUDIO_USE_ES8311
    return es8311_configure_sample_rate(sampleRate);
#else
    return true;
#endif
}

static bool driver_deinit(void)
{
    i2s_driver_uninstall(I2S_NUM_0);
    if (state.device == 0)
    {
    #if RG_AUDIO_USE_INT_DAC
        i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE);
    #endif
    }
    else if (state.device == 1)
    {
    #if RG_AUDIO_USE_EXT_DAC
        #ifdef RG_GPIO_SND_I2S_MCK
        gpio_reset_pin(RG_GPIO_SND_I2S_MCK);
        #endif
        gpio_reset_pin(RG_GPIO_SND_I2S_BCK);
        gpio_reset_pin(RG_GPIO_SND_I2S_DATA);
        gpio_reset_pin(RG_GPIO_SND_I2S_WS);
    #endif
    }
    #ifdef RG_GPIO_SND_AMP_ENABLE
    gpio_reset_pin(RG_GPIO_SND_AMP_ENABLE);
    #endif
    return true;
}

static bool driver_submit(const rg_audio_frame_t *frames, size_t count)
{
    float volume = state.muted ? 0.f : (state.volume * 0.01f);
    bool use_internal_dac = state.device == 0;
    rg_audio_frame_t buffer[DMA_BUFFER_LEN];
    size_t pos = 0;

    for (size_t i = 0; i < count; ++i)
    {
        int left = frames[i].left * volume;
        int right = frames[i].right * volume;

        if (use_internal_dac)
        {
        #if RG_AUDIO_USE_INT_DAC == 1
            left = ((left + right) >> 1) + 0x8000; // the internal DAC expects unsigned data
            right = 0;
        #elif RG_AUDIO_USE_INT_DAC == 2
            left = 0;
            right = ((left + right) >> 1) + 0x8000; // the internal DAC expects unsigned data
        #elif RG_AUDIO_USE_INT_DAC == 3
            // In two channel mode we use left and right as a differential mono output to increase resolution.
            int sample = (left + right) >> 1;
            if (sample > 0x7F00)
            {
                left = 0x8000 + (sample - 0x7F00);
                right = -0x8000 + 0x7F00;
            }
            else if (sample < -0x7F00)
            {
                left = 0x8000 + (sample + 0x7F00);
                right = -0x8000 + -0x7F00;
            }
            else
            {
                left = 0x8000;
                right = -0x8000 + sample;
            }
        #endif
        }

        // Clipping   (not necessary, we have (int16 * vol) and volume is never more than 1.0)
        // if (left > 32767) left = 32767; else if (left < -32768) left = -32767;
        // if (right > 32767) right = 32767; else if (right < -32768) right = -32767;

        // Queue
        buffer[pos].left = left;
        buffer[pos].right = right;

        if (i == count - 1 || ++pos == RG_COUNT(buffer))
        {
            size_t written;
            if (i2s_write(I2S_NUM_0, (void *)buffer, pos * 4, &written, 1000) != ESP_OK)
                RG_LOGW("I2S Submission error! Written: %d/%d\n", written, pos * 4);
            pos = 0;
        }
    }
    return true;
}

static bool driver_set_mute(bool mute)
{
    i2s_zero_dma_buffer(I2S_NUM_0);
    #ifdef RG_GPIO_SND_AMP_ENABLE
    gpio_set_level(RG_GPIO_SND_AMP_ENABLE, mute ? MUTE_ENABLE : MUTE_DISABLE);
    #endif
    state.muted = mute;
    return true;
}

static bool driver_set_volume(int volume)
{
    state.volume = RG_MIN(RG_MAX(volume, 0), 100);
    return true;
}

static const char *driver_get_error(void)
{
    return state.last_error;
}

const rg_audio_driver_t rg_audio_driver_i2s = {
    .name = "i2s",
    .init = driver_init,
    .deinit = driver_deinit,
    .submit = driver_submit,
    .set_mute = driver_set_mute,
    .set_volume = driver_set_volume,
    .set_sample_rate = driver_set_sample_rates,
    .get_error = driver_get_error,
};

#endif // RG_AUDIO_USE_INT_DAC || RG_AUDIO_USE_EXT_DAC
