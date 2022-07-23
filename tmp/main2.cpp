 
// Include required libraries
#include <Arduino.h>
#include <Audio.h>
#include <SPIFFS.h> 
 
// I2S Connections
#define I2S_DOUT      22
#define I2S_BCLK      26
#define I2S_LRC       25
 
 // Create Audio object
Audio audio;
 
void setup() {
    Serial.begin(115200);
    
    // Start microSD Card
    if(!SPIFFS.begin(false))
    {
      Serial.println("Error accessing SPIFFS!");
      while(true); 
    }
    
    // Setup I2S 
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
    // Set Volume
    audio.setVolume(10);

    // Open music file
    audio.connecttoFS(SPIFFS,"/demo.wav");
 
}
 
void loop()
{
    audio.loop();    
}