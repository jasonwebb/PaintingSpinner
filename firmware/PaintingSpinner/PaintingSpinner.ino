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
const byte CW = LOW;
const byte CCW = HIGH;

byte motorDirection = CCW;

byte motorPwmPin = 3;
byte motorDirPin = 2;

int motorCurrentSpeed = 0;
int motorTargetSpeed = 0;
int motorMinSpeed = 50;
int motorMaxSpeed = 255;
int motorAcceleration = 1;

int motorUpdateInterval = 50;     // in milliseconds
int motorRandomUpdateInterval;
int motorRandomUpdateLower = 5000,  // 5s
    motorRandomUpdateUpper = 20000; // 20s

unsigned int motorLastUpdate = 0; // in milliseconds
unsigned int motorLastRandomUpdate = 0;

/*******************************
* Audio shield parameters
********************************/
CytronEZMP3 audio;

byte audioRxPin = 8;
byte audioTxPin = 9;

byte audioTrackNumber = 1;
byte audioVolume = 0;
byte audioMaxVolume = 30; // seems to be a hard limit either of the library, chip or physics

unsigned int audioLastUpdate = 0;
unsigned int audioUpdateInterval = 500; // Cytron MP3 library/chip is slow and prone to crashing, so keep updates low


/*******************************
* Distance sensor parameters
********************************/
byte sensorPin = A0;
byte sensorEnablePin = A1;
byte sensorEnableSwitchPin = A1;
byte sensorEnabledLEDPin = A3;

// Rolling average variables
const int sensorNumSamples = 50;
int sensorSamples[sensorNumSamples];
int sensorSampleIndex = 0;
int sensorTotal = 0;
int sensorAverage = 0;

boolean sensorEnabled, sensorEnabledNextState;

int sensorUpdateInterval = 10; // in milliseconds
unsigned int sensorLastUpdate = 0;    // in milliseconds

int sensorLowestValue = 50;   // represents furthest distance from sensor
int sensorHighestValue = 516;  // represents closest distance to sensor

int sensorMinimumThreshold = 50;  // filter out sensor noise to allow motor to completely stop when people leave


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
        Serial.println("Distance sensing is enabled");
        break;
      case LOW:
        Serial.println("Distance sensing is disabled");
        break;
    }
  }
}

void loop() {
  currentTime = millis();

  // Check for and process new sensor data for use in upcoming logic
  updateSensor();

  if(currentTime > motorLastUpdate + motorUpdateInterval) {
    // If sensor is enabled, map live sensor data to target motor speed
    if(sensorEnabled) {      
      if(sensorAverage > sensorMinimumThreshold)
        motorTargetSpeed = constrain(map(sensorAverage, sensorLowestValue, sensorHighestValue, motorMinSpeed, motorMaxSpeed), motorMinSpeed, motorMaxSpeed);
      else
        motorTargetSpeed = 0;

    // If sensor is NOT enabled, generate new random target speeds at random intervals
    } else {
      // Choose random target speed and new random update interval
      if(currentTime > motorLastRandomUpdate + motorRandomUpdateInterval) {
        motorTargetSpeed = (int)random(motorMinSpeed, motorMaxSpeed);
        motorRandomUpdateInterval = random(motorRandomUpdateLower, motorRandomUpdateUpper);

        motorLastRandomUpdate = currentTime;
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

/**************************************************
* Update sensor data
***************************************************/
void updateSensor() {
  if(currentTime > sensorLastUpdate + sensorUpdateInterval) {
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

void updateSensorEnabledSwitch() {
  // Check enable switch
  sensorEnabledNextState = digitalRead(sensorEnableSwitchPin);

  if(sensorEnabledNextState != sensorEnabled) {
    if(DEBUG) {
      switch(sensorEnabledNextState) {
        case HIGH:
          digitalWrite(sensorEnabledLEDPin, HIGH);
          motorRandomUpdateInterval = random(motorRandomUpdateLower, motorRandomUpdateUpper);
          Serial.println("Turning on distance sensing");
          break;
        case LOW:
          digitalWrite(sensorEnabledLEDPin, LOW);
          Serial.println("Turning off distance sensing");
          break;
      }
    }
    
    sensorEnabled = sensorEnabledNextState;
  }
}
