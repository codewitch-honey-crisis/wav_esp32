#ifndef M5STACK
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
    //SD.begin(5,spi);
    
    if(!sound.initialize()) {
        Serial.println("Unable to initialize I2S audio");
        while(1);
    }
    wav_file_source wav;
    waveform_source<> wform;
    File file = SPIFFS.open("/demo.wav","rb");
    file_stream fs(file);
    sfx_result r = wav_file_source::open(fs,&wav);
        if(r!=sfx_result::success) {
        Serial.print("Error loading wav: ");
        Serial.println((int)r);
        while(1);
    }
    wav.loop(true);
    
    mixer_source<2> mixer;
    r=mixer_source<2>::create(&mixer);
    if(r!=sfx_result::success) {
        Serial.print("Error creating mixer: ");
        Serial.println((int)r);
        while(1);
    }
    mixer.voice(0,&wform);
    mixer.level(0,.25);
    mixer.voice(1,&wav);
    mixer.level(1,.5);
    transport trans;
    r=transport::create(sound,mixer,&trans);
    if(r!=sfx_result::success) {
        Serial.print("Error initializing transport: ");
        Serial.println((int)r);
        while(1);
    }
    size_t written;
    float fq = 200;
    float fd=1;
    // make frequency go up and down in a loop
    while( r==sfx_result::success) {
        r=trans.update();
        wform.frequency(fq);
        if(fq+fd>2600) {
            fd=-fd;
        }
        if(fq+fd<200)  {
            fd=-fd;
        }
        fq+=fd;
    }
    file.close();
}
void loop() {
    
}
#endif