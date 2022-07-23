#include <Arduino.h>
#include <SPIFFS.h>
#include <SD.h>
#include <i2s_external.hpp>
#include <sfx.hpp>

using namespace arduino;
using namespace sfx;
using audio_t = i2s_external<-1,26,25,22,44100,16,i2s_channels::both>;
audio_t sound;
void setup() {
    Serial.begin(115200);
    SPIFFS.begin(false);
    if(!sound.initialize()) {
        Serial.println("Unable to initialize I2S audio");
        while(1);
    }
    wav_file_source wav;
    File file = SPIFFS.open("/demo.wav","rb");
    file_stream fs(file);
    sfx_result r = wav_file_source::open(&fs,&wav);
    if(r!=sfx_result::success) {
        Serial.print("Error loading wav: ");
        Serial.println((int)r);
        while(1);
    }
    transport trans;
    r=transport::create(&sound,&wav,&trans);
    wav.loop(true);
    if(r!=sfx_result::success) {
        Serial.print("Error initializing transport: ");
        Serial.println((int)r);
        while(1);
    }
    uint16_t samp[audio_t::dma_size];
    size_t written;
    while( r==sfx_result::success) {
        size_t i;
        if(audio_t::dma_size<(i=wav.read(samp,audio_t::dma_size))) {
            Serial.print("read ");
            Serial.println(i);
            break;
        }
        i2s_write((i2s_port_t)0,samp,audio_t::dma_size,&written,portMAX_DELAY);
      
    }
    file.close();
}
void loop() {
    
}