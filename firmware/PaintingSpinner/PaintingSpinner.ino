/***************************************************************
 * PaintingSpinner
 * Built by Jason Webb for artist Nadia Shinkunas
 * 
 * Github repo: https://github.com/jasonwebb/PaintingSpinner
 * Author website: http://jason-webb.info
 ***************************************************************/

#include <CytronEZMP3.h>

/*******************************
* Motor parameters
********************************/
const byte CW = HIGH;
const byte CCW = LOW;

byte motorDirection = CCW;

byte motorPwmPin = 3;
byte motorDirPin = 2;

int motorCurrentSpeed = 0;
int motorTargetSpeed = 0;
int motorMinSpeed = 75;
int motorMaxSpeed = 255;
int motorAcceleration = 1;

boolean motorEnabled = true;

long motorUpdateInterval = 175;  // (in ms) faster updates mean faster acceleration, so keep them slow 
                                // (high values) to prevent too much torque on motor gears.

unsigned long motorRandomUpdateLower = 20 * 1000,
              motorRandomUpdateUpper = 50 * 1000;
unsigned long motorRandomUpdateInterval = 0;

unsigned long motorLastUpdate = 0;
unsigned long motorLastRandomUpdate = 0;


/*******************************
* Audio shield parameters
********************************/
CytronEZMP3 audio;

byte audioRxPin = 8;
byte audioTxPin = 9;

byte audioTrackNumber = 1;
byte audioTracksTotal;

byte audioVolume = 30;
byte audioMinVolume = 10;
byte audioMaxVolume = 30; // seems to be a hard limit either of the library, chip or physics

boolean audioEnabled = false;

int audioUpdateInterval = 1000; // Cytron MP3 library/chip is slow and prone to crashing, so keep updates low

unsigned int audioRandomUpdateLower = 10 * 1000,
             audioRandomUpdateUpper = 30 * 1000;
unsigned int audioRandomUpdateInterval = 0;

unsigned int audioLastUpdate = 0;
unsigned int audioLastRandomUpdate = 0;


/*******************************
* Distance sensor parameters
********************************/
byte sensorPin = A0;
byte sensorEnableSwitchPin = A1;
byte sensorEnabledLEDPin = A3;

// Rolling average variables
const int sensorNumSamples = 50;
int sensorSamples[sensorNumSamples];
int sensorSampleIndex = 0;
int sensorTotal = 0;
int sensorAverage = 0;

boolean sensorEnabled, sensorEnabledNextState;

unsigned long sensorUpdateInterval = 10;
unsigned long sensorLastUpdate = 0;

int sensorLowestValue = 50;   // represents furthest distance from sensor
int sensorHighestValue = 516; // represents closest distance to sensor

int sensorMinimumThreshold = 50;  // filter out sensor noise to allow motor to completely stop when people leave


/**************************************
* General state flags and variables
***************************************/
boolean DEBUG = true;
unsigned long currentTime = 0;


void setup() {
  // Initialize debug mode ----------------------------------------
  if (DEBUG) {
    Serial.begin(9600);
    while(!Serial);
  }

  // Randomize random seed ----------------------------------------
  randomSeed(analogRead(A5));

  // Initialize motor driver shield -------------------------------
  pinMode(motorPwmPin, OUTPUT);
  pinMode(motorDirPin, OUTPUT);

  analogWrite(motorPwmPin, motorCurrentSpeed);  // Should be 0
  digitalWrite(motorDirPin, motorDirection);

  if(DEBUG)
    Serial.println("Motor ready");


  // Initialize audio shield --------------------------------------
  if (!audio.begin(audioRxPin, audioTxPin)) {
    if (DEBUG)
      Serial.println("Failed to initialize audio shield");

    while (1);
  }
  
  audio.setVolume(audioVolume);

  audioTracksTotal = audio.getTotalFiles();

  if(DEBUG)
    Serial.println("Audio shield ready");
      

  // Initialize sensor --------------------------------------------
  pinMode(sensorPin, INPUT);
  pinMode(sensorEnableSwitchPin, INPUT);
  pinMode(sensorEnabledLEDPin, OUTPUT);

  // Activate internal pull-up for enable switch
  digitalWrite(sensorEnableSwitchPin, HIGH);

  // Set up initial enable state
  sensorEnabled = digitalRead(sensorEnableSwitchPin);
  digitalWrite(sensorEnabledLEDPin, sensorEnabled);

  // Get initial switch state
  updateSensorEnabledSwitch();

  // Initialize sample buffer to empty values
  for(int i = 0; i < sensorNumSamples; i++)
    sensorSamples[i] = 0;

  if(DEBUG) {
    Serial.println("Distance sensor ready");

    switch(sensorEnabled) {
      case HIGH:
        Serial.println("Distance sensing is enabled - motor speed now mapped to distance");
        break;
      case LOW:
        Serial.println("Distance sensing is disabled - motor speed now randomized at random intervals");
        break;
    }
  }
}

void loop() {
  currentTime = millis();

  // Check for and process new sensor data for use in upcoming logic
  // - Takes readings from the distance sensor (if enabled)
  // - Checks the sensor enable switch for changes
  updateSensor();

  // Update motor speed to chase a target speed
  // - If sensor is enabled, target speed will follow distance sensor
  // - If sensor is NOT enabled, target speed will be randomized at random intervals
  updateMotor();

  // Update audio
  // - If sensor is enabled, trigger samples continuously and map sensor average to volume
  // - If sensor is NOT enabled, trigger samples at random volumes at random intervals
  updateAudio();

}

/******************************************************************************
* Update sensor data buffer and calculated values
*   - Takes readings from the distance sensor (if enabled)  
*   - Checks the distance sensor enable switch for changes
******************************************************************************/
void updateSensor() {
  if(currentTime - sensorLastUpdate >= sensorUpdateInterval) {
    // Check and update enable switch
    updateSensorEnabledSwitch();
    
    // Update rolling average of samples ---------------------------    
    // Pop latest sample from total
    sensorTotal -= sensorSamples[sensorSampleIndex];
  
    // Get new sample from the sensor and store it in buffer
    sensorSamples[sensorSampleIndex] = analogRead(sensorPin);
  
    // Push latest sample to total
    sensorTotal += sensorSamples[sensorSampleIndex];
  
    // Advance or roll over buffer index
    if(sensorSampleIndex < sensorNumSamples - 1)
      sensorSampleIndex++;
    else
      sensorSampleIndex = 0;
  
    // Calculate new average
    sensorAverage = sensorTotal / sensorNumSamples;

    sensorLastUpdate = millis();
  }
}

/******************************************************************************
* Check for and process any changes to the "enable" switch
*   - If a change is detected, some prep work needs to happen
******************************************************************************/
void updateSensorEnabledSwitch() {
  // Check enable switch pin
  sensorEnabledNextState = digitalRead(sensorEnableSwitchPin);

  if(sensorEnabledNextState != sensorEnabled) {
    switch(sensorEnabledNextState) {
      case HIGH:
        digitalWrite(sensorEnabledLEDPin, HIGH);
        motorRandomUpdateInterval = (int)random(motorRandomUpdateLower, motorRandomUpdateUpper);

        if(DEBUG)
          Serial.println("Turning on distance sensing");

        break;
        
      case LOW:
        digitalWrite(sensorEnabledLEDPin, LOW);

        if(DEBUG)
          Serial.println("Turning off distance sensing");

        break;
    }
    
    sensorEnabled = sensorEnabledNextState;
  }
}

/*******************************************************************************************
* Update motor target speed and current speed
*   - If sensor is enabled, target speed will follow distance sensor
*   - If sensor is NOT enabled, target speed will be randomized at random intervals
*   - It is very important to keep accelerations low to prevent too much torque on motor
********************************************************************************************/
void updateMotor() {
  if(currentTime - motorLastUpdate >= motorUpdateInterval && motorEnabled) {
    // If sensor is enabled, map live sensor data to target motor speed
    if(sensorEnabled) {      
      if(sensorAverage > sensorMinimumThreshold)
        motorTargetSpeed = constrain(map(sensorAverage, sensorLowestValue, sensorHighestValue, motorMinSpeed, motorMaxSpeed), motorMinSpeed, motorMaxSpeed);
      else
        motorTargetSpeed = 0;

    // If sensor is NOT enabled, generate new random target speeds at random intervals
    } else {
      // Choose random target speed and new random update interval
      if(currentTime - motorLastRandomUpdate >= motorRandomUpdateInterval) {
        motorTargetSpeed = (int)random(motorMinSpeed, motorMaxSpeed);
        motorRandomUpdateInterval = random(motorRandomUpdateLower, motorRandomUpdateUpper);

        if(DEBUG) {
          Serial.print("New target speed - ");
          Serial.print(map(motorTargetSpeed, 0, motorMaxSpeed, 0, 100));
          Serial.println("%");
        }

        motorLastRandomUpdate = millis();
      }
    }

    // Adjust current speed to "chase" target speed, within bounds
    if( (motorCurrentSpeed < motorTargetSpeed) && (motorCurrentSpeed < motorMaxSpeed) )
        motorCurrentSpeed += motorAcceleration;
    else if( (motorCurrentSpeed > motorTargetSpeed) && (motorCurrentSpeed > 0) )
        motorCurrentSpeed -= motorAcceleration;
  
    // Push motor speed to motor shield
    analogWrite(motorPwmPin, motorCurrentSpeed);

    motorLastUpdate = millis();
  }
}

/*****************************************************************
 * Update audio
******************************************************************/
void updateAudio() {
  if(currentTime - audioLastUpdate >= audioUpdateInterval && audioEnabled) {
      if(1) {
//    if(!audio.isPlaying()) {
      // If sensor is enabled, map distance to volume
      if(sensorEnabled) {
        audioVolume = (int)constrain(map(sensorAverage, sensorLowestValue, sensorHighestValue, audioMinVolume, audioMaxVolume), audioMinVolume, audioMaxVolume);

        audioTrackNumber = (int)random(1,audioTracksTotal);
        audio.setVolume(audioVolume);
        audio.playTrack(audioTrackNumber);

        if(DEBUG)
          Serial.println("Triggering audio file");
        
      // If sensor is NOT enabled, just use max volume
      } else {
        if(currentTime - audioLastRandomUpdate >= audioRandomUpdateInterval) {
          audioVolume = audioMaxVolume;

          audioTrackNumber = (int)random(1,audioTracksTotal);
          audio.setVolume(audioVolume);
          delay(100);
          audio.playTrack(audioTrackNumber);
          delay(100);

          if(DEBUG) {
            Serial.print("Triggering audio file - ");
            Serial.println(audioTrackNumber);
          }

          audioRandomUpdateInterval = (int)random(audioRandomUpdateLower, audioRandomUpdateUpper);
          audioLastRandomUpdate = millis();
        }
      }

      audioLastUpdate = millis();
    }
  }
}
