/***************************************************************
 * PaintingSpinner - audio shield test
 * Built by Jason Webb for artist Nadia Shinkunas
 * 
 * Github repo: https://github.com/jasonwebb/PaintingSpinner
 * Author website: http://jason-webb.info
 ***************************************************************/

#include <CytronEZMP3.h>

/*******************************
* Audio shield parameters
********************************/
CytronEZMP3 audio;

byte audioRxPin = 8;
byte audioTxPin = 9;

byte audioTrackNumber = 1;
byte audioVolume = 0;     // maximum is 255
byte audioMaxVolume = 30;
byte audioVolumeStep = 1;

unsigned int audioLastUpdate = 0;
unsigned int audioUpdateInterval = 500;

const boolean UP = 0;
const boolean DOWN = 1;
boolean rampDirection = UP;

/**************************************
* General state flags and variables
***************************************/
boolean DEBUG = true;
unsigned int currentTime = 0;


void setup() {
  // Initialize debug mode ----------------------------------------
  if (DEBUG) {
    Serial.begin(9600);
    while(!Serial);
  }

  // Initialize audio shield --------------------------------------
  if (!audio.begin(audioRxPin, audioTxPin)) {
    if (DEBUG)
      Serial.println("Failed to initialize audio shield");

    while (1);
  }
  audio.setVolume(audioVolume);

  if(DEBUG)
    Serial.println("Audio shield ready");
}

void loop() {
  currentTime = millis();

  // Continuously trigger audio file ----------------------------
  if(!audio.isPlaying())
    audio.playTrack(audioTrackNumber);

  // Ramp up and down volume -------------------------------------
  if(currentTime > audioLastUpdate + audioUpdateInterval) {
    if(rampDirection == UP) {
      if(audioVolume <= audioMaxVolume - audioVolumeStep)
        audioVolume += audioVolumeStep;
      else
        rampDirection = DOWN;    
    } else {
      if(audioVolume >= audioVolumeStep)
        audioVolume -= audioVolumeStep;
      else
        rampDirection = UP;
    }

    audio.setVolume(audioVolume);

    if(DEBUG)
      Serial.println(audioVolume);
    
    audioLastUpdate = millis();
  }
}
