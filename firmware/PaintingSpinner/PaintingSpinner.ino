/***************************************************************
 * PaintingSpinner
 * Built by Jason Webb for artist Nadia Shinkunas
 * 
 * Github repo: https://github.com/jasonwebb/PaintingSpinner
 * Author website: http://jason-webb.info
 ***************************************************************/

#include <CytronEZMP3.h>

/** Motor parameters *************/
const byte CW = LOW;
const byte CCW = HIGH;

byte motorDirection = CW;

byte motorPwmPin = 3;
byte motorDirPin = 2;

int motorSpeed = 0;
int motorMaxSpeed = 255;
int motorAcceleration = 1;

boolean motorAtSpeed = false;

/** MP3 shield parameters ********/
CytronEZMP3 audio;

byte audioRxPin = 8;
byte audioTxPin = 9;

byte audioTrackNumber = 1;

/** Sensor parameters ************/
byte sensorPin = A0;

int sensorValue = 0;
byte sensorTriggerThreshold = 100;  // need to refine
boolean sensorTriggered = false;

int sensorCheckInterval = 500;  // in milliseconds
int sensorLastCheck = 0;

/** Debugging mode ***************/
boolean DEBUG = true;


void setup() {
  // Initialize debug mode ---------------------------------------
  if (DEBUG)
    Serial.begin(9600);

  // Initialize audio shield
  if (!audio.begin(audioRxPin, audioTxPin)) {
    if (DEBUG)
      Serial.println("Failed to initialize audio shield");

    while (1);
  }
  audio.setVolume(30);

    if(DEBUG)
      Serial.println("Audio shield ready");
      

  // Initialize sensor --------------------------------------------
  pinMode(sensorPin, INPUT);

    if(DEBUG)
      Serial.println("Distance sensor ready");
      

  // Initialize motor driver shield -------------------------------
  pinMode(motorPwmPin, OUTPUT);
  pinMode(motorDirPin, OUTPUT);

  analogWrite(motorPwmPin, motorSpeed);
  digitalWrite(motorDirPin, motorDirection);

    if(DEBUG)
      Serial.print("Spinning up motor ... ");
}

void loop() {
  // Ramp up motor speed
  if (motorSpeed < motorMaxSpeed) {
    motorSpeed += motorAcceleration;
    analogWrite(motorPwmPin, motorSpeed);
  } else {
    motorAtSpeed = true;

    if(DEBUG)
      Serial.println("done");
  }

  // Look for and process sensor trigger events once motor is up to speed, so long as no audio is currently playing
  if(motorAtSpeed && !audio.isPlaying()) {
    // Check sensor for trigger event
    if(millis() >= sensorLastCheck + sensorCheckInterval) {
      sensorValue = analogRead(sensorPin);

      if(sensorValue >= sensorTriggerThreshold)
        sensorTriggered = true;
    }

    // If triggered, play MP3 file
    if(sensorTriggered) {
      if(DEBUG)
        Serial.println("Sensor triggered - playing audio file");

      audio.playTrack(audioTrackNumber);
      sensorTriggered = false;
    }
  }
}
