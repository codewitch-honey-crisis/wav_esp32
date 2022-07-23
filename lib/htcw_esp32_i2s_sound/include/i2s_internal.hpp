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
enum struct i2s_internal_format {
    mono_8 = 0,
    mono_16,
    stereo_8,
    stereo_16
};
template<i2s_internal_format InputFormat = i2s_internal_format::mono_8, size_t OutputSampleRate = 44100, i2s_channels OutputChannelConfiguration = i2s_channels::left, bool UseApll = true, size_t DmaSamples = 32, size_t DmaBufferCount = 2>
class i2s_internal final : public sfx::audio_destination {
    static_assert(OutputSampleRate==44100,"Resampling is not currently supported");
public:
    constexpr static const i2s_internal_format input_format = InputFormat;
    constexpr static const size_t output_sample_rate = OutputSampleRate;
    constexpr static const size_t output_bit_depth = 8;
    constexpr static const i2s_channels output_channel_configuration = OutputChannelConfiguration;
    constexpr static const size_t output_channels = 1+(output_channel_configuration==i2s_channels::both);
    constexpr static const size_t dma_samples = DmaSamples;
    constexpr static const size_t dma_buffer_count = DmaBufferCount;
    constexpr static const size_t dma_size = dma_samples*output_channels;
    constexpr static const bool use_apll = UseApll;
    using type = i2s_internal;
private:
    bool m_initialized;
    static uint8_t m_out_buffer[];
    size_t copy_raw(const void* samples, size_t sample_count) {
        size_t result = 0;
        const uint8_t* p = (const uint8_t*)samples;
        while(sample_count) {
            size_t to_write = sample_count<dma_size?sample_count:dma_size;
            memcpy(i2s_internal::m_out_buffer, p,to_write);
            size_t written;
            i2s_write((i2s_port_t)0,i2s_internal::m_out_buffer,to_write,&written,portMAX_DELAY);
            p+=written;
            sample_count-=written;
            result+=written;
            if(written!=to_write) {
                return written;
            }
        }
        return result;
    }
    size_t copy_stereo8_to_mono8(const void* samples, size_t sample_count) {
        size_t result = 0;
        sample_count /= 2;
        const uint8_t* p = (const uint8_t*)samples;
        while(sample_count) {
            size_t to_write = sample_count<dma_size?sample_count:dma_size;
            const uint8_t* in = (const uint8_t*)p;
            uint8_t* out = i2s_internal::m_out_buffer;
            for(int i = 0;i<to_write;++i) {
                *(out++)=(*(in++)+*(in++))>>1;
            }
            size_t written;
            i2s_write((i2s_port_t)0,i2s_internal::m_out_buffer,to_write,&written,portMAX_DELAY);
            p+=written*2;
            sample_count-=written;
            result+=written;
            if(written!=to_write) {
                return written;
            }
        }
        return result;
    }
    size_t copy_mono16_to_mono8(const void* samples, size_t sample_count) {
        size_t result = 0;
        size_t total_to_write = sample_count/2;
        const uint8_t* p = (const uint8_t*)samples;
        while(result<total_to_write) {
            size_t to_write = sample_count/2<dma_size?sample_count/2:dma_size;
            const uint16_t* in = (uint16_t*)p;
            uint8_t* out = i2s_internal::m_out_buffer;
            for(int i = 0;i<to_write;++i) {
                *(out++)=uint8_t((*in)>>8);
            }
            size_t written;//=to_write;
            i2s_write((i2s_port_t)0,i2s_internal::m_out_buffer,to_write,&written,portMAX_DELAY);
            p+=written*2;
            sample_count-=written*2;
            result+=written;
            if(written!=to_write) {
                return written;
            }
        }
        return result;
    }
    size_t copy_stereo16_to_stereo8(const void* samples, size_t sample_count) {
        size_t result = 0;
        const uint8_t* p = (const uint8_t*)samples;
        while(sample_count) {
            size_t to_write = sample_count<dma_size?sample_count:dma_size;
            const uint16_t* in = (const uint16_t*)p;
            uint8_t* out = i2s_internal::m_out_buffer;
            for(int i = 0;i<to_write;++i) {
                *(out++)=uint8_t((*in)>>8);
            }
            size_t written;
            i2s_write((i2s_port_t)0,i2s_internal::m_out_buffer,to_write,&written,portMAX_DELAY);
            p+=written*2;
            sample_count-=written*2;
            result+=written;
            if(written!=to_write) {
                return written;
            }
        }
        return result;
    }
    size_t copy_stereo16_to_mono8(const void* samples, size_t sample_count) {
        size_t result = 0;
        size_t total_to_write = sample_count/4;
        const uint8_t* p = (const uint8_t*)samples;
        while(result<total_to_write) {
            size_t to_write = sample_count/4<dma_size?sample_count/4:dma_size;
            const uint16_t* in = (uint16_t*)p;
            uint8_t* out = i2s_internal::m_out_buffer;
            for(int i = 0;i<to_write;++i) {
                *(out++)=uint8_t((((*in++)>>8)+((*in++)>>8))>>1);
            }
            size_t written;
            i2s_write((i2s_port_t)0,i2s_internal::m_out_buffer,to_write,&written,portMAX_DELAY);
            result+=written;
            if(written!=to_write) {
                return written;
            }
        }
        return sample_count;
    }
    size_t copy_mono8_to_stereo8(const void* samples, size_t sample_count) {
        size_t result = 0;
        const uint8_t* p = (const uint8_t*)samples;
        while(sample_count) {
            size_t to_write = sample_count<(dma_size/2)?sample_count:(dma_size/2);
            const uint8_t* in = (const uint8_t*)p;
            uint8_t* out = i2s_internal::m_out_buffer;
            for(int i = 0;i<to_write;++i) {
                *(out++)=*in;
                *(out++)=*(in++);
            }
            size_t written;
            i2s_write((i2s_port_t)0,i2s_internal::m_out_buffer,out-i2s_internal::m_out_buffer,&written,portMAX_DELAY);
            p+=written/2;
            sample_count-=written/2;
            result+=written;
            if(written!=out-i2s_internal::m_out_buffer) {
                return written;
            }
        }
        return result;
    }
    size_t copy_mono16_to_stereo8(const void* samples, size_t sample_count) {
        size_t result = 0;
        const uint8_t* p = (const uint8_t*)samples;
        while(sample_count) {
            size_t to_write = sample_count<dma_size?sample_count:dma_size;
            const uint16_t* in = (const uint16_t*)p;
            uint8_t* out = i2s_internal::m_out_buffer;
            for(int i = 0;i<to_write;++i) {
                uint8_t tmp = (*in)>>8;
                *(out++)=tmp;
                *(out++)=tmp;
            }
            size_t written;//=out-i2s_internal::m_out_buffer;
            i2s_write((i2s_port_t)0,i2s_internal::m_out_buffer,out-i2s_internal::m_out_buffer,&written,portMAX_DELAY);
            p+=written;
            sample_count-=written;
            result+=written;
            if(written!=out-i2s_internal::m_out_buffer) {
                return written;
            }
        }
        
        return result;
    }
public:
    i2s_internal() : m_initialized(false) {

    }
    inline bool initialized() const { return m_initialized; }
    bool initialize() {
        if(!m_initialized) {
            i2s_config_t i2s_config;
            memset(&i2s_config,0,sizeof(i2s_config_t));
            i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN);
            i2s_config.sample_rate = output_sample_rate;
            i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_8BIT;
            i2s_config.channel_format = 
                (output_channel_configuration==i2s_channels::left?I2S_CHANNEL_FMT_ALL_LEFT:
                        output_channel_configuration==i2s_channels::right?I2S_CHANNEL_FMT_ALL_RIGHT:
                            I2S_CHANNEL_FMT_RIGHT_LEFT);
            i2s_config.communication_format = I2S_COMM_FORMAT_STAND_MSB;
            i2s_config.dma_buf_count = dma_buffer_count;
            i2s_config.dma_buf_len = dma_size;
            i2s_config.use_apll = use_apll;
            i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;
            if(ESP_OK!=i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL)) {
                return false;
            }
            if(ESP_OK!=i2s_set_pin((i2s_port_t)0,nullptr)) {
                return false;
            }
            m_initialized = true;
        }
        return true;
    }
    virtual size_t bit_depth() const {
        switch(input_format) {
            case i2s_internal_format::mono_8:
            case i2s_internal_format::stereo_8:
                return 8;
            default:
                return 16;
        }
    }
    virtual size_t sample_rate() const {
        return output_sample_rate;
    }
    virtual size_t channels() const {
        switch(input_format) {
            case i2s_internal_format::mono_8:
            case i2s_internal_format::mono_16:
                return 1;
            default:
                return 2;
        }
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
        switch(input_format) {
            case i2s_internal_format::mono_8:
                if(output_channels==1) {
                    result = copy_raw(samples,sample_count);
                } else {
                    result = copy_mono8_to_stereo8(samples,sample_count);
                }
                break;
            case i2s_internal_format::stereo_8:
                if(output_channels==2) {
                    result = copy_raw(samples,sample_count);
                } else {
                    result = copy_stereo8_to_mono8(samples,sample_count);
                }
                break;
            case i2s_internal_format::mono_16:
                if(output_channels==1) {
                    result = copy_mono16_to_mono8(samples,sample_count);
                } else {
                    result = copy_mono16_to_stereo8(samples,sample_count);
                }
                break;
            case i2s_internal_format::stereo_16:
                if(output_channels==2) {
                    result = copy_stereo16_to_stereo8(samples,sample_count);
                } else {
                    result = copy_stereo16_to_mono8(samples,sample_count);
                }
                break;
            
        }
        vTaskDelay(0);
        return result;
    }
};
template<i2s_internal_format InputFormat, size_t OutputSampleRate, i2s_channels OutputChannelConfiguration, bool UseApll, size_t DmaSamples, size_t DmaBufferCount>
uint8_t i2s_internal<InputFormat,OutputSampleRate,OutputChannelConfiguration,UseApll,DmaSamples,DmaBufferCount>::m_out_buffer[DmaSamples*(1+(OutputChannelConfiguration==i2s_channels::both))]={0};
}

