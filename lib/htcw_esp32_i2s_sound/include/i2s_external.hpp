#pragma once
#include <sfx_core.hpp>
#ifndef ESP32
#error "This library only supports the ESP32"
#endif
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
namespace arduino {
enum struct i2s_channels {
    left = 0,
    right,
    both
};
template<int8_t MckPin, int8_t BckPin, int8_t WsPin, int8_t DOutPin, size_t SampleRate = 44100, size_t BitDepth = 16, i2s_channels ChannelConfiguration = i2s_channels::left, bool UseApll = true, size_t DmaSamples = 32, size_t DmaBufferCount = 2>
class i2s_external final : public sfx::audio_destination {
    static_assert(BitDepth==8||BitDepth==16||BitDepth==32,"BitDepth must be 8, 16, or 32");
public:
    constexpr static const int8_t mck_pin = MckPin;
    constexpr static const int8_t bck_pin = BckPin;
    constexpr static const int8_t ws_pin = WsPin;
    constexpr static const int8_t dout_pin = DOutPin;
    constexpr static const size_t output_sample_rate = SampleRate;
    constexpr static const size_t output_bit_depth = BitDepth;
    constexpr static const i2s_channels output_channel_configuration = ChannelConfiguration;
    constexpr static const size_t output_channels = 1+(output_channel_configuration==i2s_channels::both);
    constexpr static const size_t dma_samples = DmaSamples;
    constexpr static const size_t dma_buffer_count = DmaBufferCount;
    constexpr static const size_t dma_size = dma_samples*output_channels;
    constexpr static const bool use_apll = UseApll;
    using type = i2s_external;
private:
    bool m_initialized;
    static uint8_t m_out_buffer[];
    size_t copy_raw(const void* samples, size_t sample_count) {
        size_t result = 0;
        const uint8_t* p = (const uint8_t*)samples;
        while(sample_count) {
            size_t to_write = sample_count<dma_size?sample_count:dma_size;
            memcpy(i2s_external::m_out_buffer, p,to_write);
            size_t written;
            i2s_write((i2s_port_t)0,i2s_external::m_out_buffer,to_write,&written,portMAX_DELAY);
            p+=written;
            sample_count-=written;
            result+=written;
            if(written!=to_write) {
                return written;
            }
        }
        return result;
    }

public:
    i2s_external() : m_initialized(false) {

    }
    inline bool initialized() const { return m_initialized; }
    bool initialize() {
        if(!m_initialized) {
            i2s_config_t i2s_config;
            i2s_pin_config_t pins;
            memset(&i2s_config,0,sizeof(i2s_config_t));
            i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
            i2s_config.sample_rate = output_sample_rate;
            i2s_config.bits_per_sample = (i2s_bits_per_sample_t)output_bit_depth;
            i2s_config.channel_format = 
                (output_channel_configuration==i2s_channels::left?I2S_CHANNEL_FMT_ALL_LEFT:
                        output_channel_configuration==i2s_channels::right?I2S_CHANNEL_FMT_ALL_RIGHT:
                            I2S_CHANNEL_FMT_RIGHT_LEFT);
            i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
            i2s_config.dma_buf_count = dma_buffer_count;
            i2s_config.dma_buf_len = dma_size;
            i2s_config.use_apll = use_apll;
           // i2s_config.fixed_mclk = I2S_PIN_NO_CHANGE;
            i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;
            if(ESP_OK!=i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL)) {
                return false;
            }
            pins = {
                .mck_io_num = mck_pin, // Unused
                .bck_io_num = bck_pin,
                .ws_io_num = ws_pin,
                .data_out_num = dout_pin,
                .data_in_num = I2S_PIN_NO_CHANGE
            };
            if(ESP_OK!=i2s_set_pin((i2s_port_t)0,&pins)) {
                return false;
            }
            m_initialized = true;
        }
        return true;
    }
    virtual size_t bit_depth() const {
        return output_bit_depth;
    }
    virtual size_t sample_rate() const {
        return output_sample_rate;
    }
    virtual size_t channels() const {
        return output_channels;
    }
    virtual sfx::audio_format format() const {
        return sfx::audio_format::pcm;
    }
    
    virtual size_t write(const void* samples, size_t sample_count) {
        if(samples==nullptr) {
            return 0;
        }
        if(sample_count==0) {
            return 0;
        }
        if(!initialize()) {
            return 0;
        }
        size_t result = 0;
        result = copy_raw(samples,sample_count);
        vTaskDelay(0);
        return result;
    }
};
template<int8_t MckPin, int8_t BkcPin, int8_t WsPin, int8_t DOutPin, size_t SampleRate, size_t BitDepth, i2s_channels ChannelConfiguration, bool UseApll, size_t DmaSamples, size_t DmaBufferCount>
uint8_t i2s_external<MckPin,BkcPin,WsPin,DOutPin,SampleRate,BitDepth,ChannelConfiguration,UseApll,DmaSamples,DmaBufferCount>::m_out_buffer[DmaSamples*(1+(ChannelConfiguration==i2s_channels::both))]={0};
}

