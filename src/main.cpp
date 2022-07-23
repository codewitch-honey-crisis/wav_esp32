#include <Arduino.h>
#include <SPIFFS.h>
#include <SD.h>
#include <i2s_external.hpp>
#include <sfx.hpp>

using namespace arduino;
using namespace sfx;
using audio_t = i2s_external<-1,26,25,22,i2s_channels::both,true>;
audio_t sound;
SPIClass spi(VSPI);
void setup() {
    Serial.begin(115200);
    SPIFFS.begin(false);
    SD.begin(5,spi);
    
    if(!sound.initialize()) {
        Serial.println("Unable to initialize I2S audio");
        while(1);
    }
    wav_file_source wav;
    File file = SD.open("/proto5.wav","rb");
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
    trans.volume(.10);
    if(r!=sfx_result::success) {
        Serial.print("Error initializing transport: ");
        Serial.println((int)r);
        while(1);
    }
    uint16_t samp[audio_t::dma_size];
    size_t written;

    while( r==sfx_result::success) {
        r=trans.update();
      
    }
    file.close();
}
void loop() {
    
}