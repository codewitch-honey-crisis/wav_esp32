#ifdef M5STACK
#include <Arduino.h>
#include <SPIFFS.h>
#include <i2s_internal.hpp>
#include <htcw_button.hpp>
#include <sfx.hpp>

using namespace arduino;
using namespace sfx;

constexpr static const int8_t button_a_pin = 39;
constexpr static const int8_t button_b_pin = 38;
constexpr static const int8_t button_c_pin = 37;

using audio_t = i2s_internal<i2s_channels::left>;

button<button_a_pin,10,true> button_a;
button<button_b_pin,10,true> button_b;
button<button_c_pin,10,true> button_c;

audio_t sound;

performer<5> perform(sound);
File file;
file_stream file_stm(file);
int button_handles[3];
uint32_t second_ts;
int sound_state;
void setup() {
    Serial.begin(115200);
    SPIFFS.begin(false);
    button_a.initialize();
    button_b.initialize();
    button_c.initialize();
    button_a.callback([](bool pressed,void*state){
        if(pressed) {
            button_handles[0]=perform.shape(440,.3);
        } else {
            perform.stop(button_handles[0]);
        }
    });
    button_b.callback([](bool pressed,void*state){
        if(pressed) {
            button_handles[1]=perform.shape(880,.3,waveform_shape::triangle);
        } else {
            perform.stop(button_handles[1]);
        }
    });
    button_c.callback([](bool pressed,void*state){
        if(pressed) {
            button_handles[2]=perform.wav(file_stm,.3);
        } else {
            perform.stop(button_handles[2]);
        }
    });
    if(!sound.initialize()) {
        Serial.println("Could not initialize sound");
        while(true);
    }
    file = SPIFFS.open("/demo.wav","rb");
    if(!file) {
        Serial.println("Could not open demo.wav file");
        while(true);
    }
    // reinitialize the file_stream with the now valid object
    file_stm.set(file);
    
}
void loop() {
    perform.update();
    button_a.update();
    button_b.update();
    button_c.update();
}
#endif