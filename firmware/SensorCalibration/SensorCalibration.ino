/********************************************************************
 * PaintingSpinner - sensor calibration
 * Built by Jason Webb for artist Nadia Shinkunas
 * 
 * DESCRIPTION:
 * (1) Samples and averages data from the distance sensor, as
 * well as (2) records the maximum and minimum values observed
 * over the runtime of the sketch.
 * 
 * Live data is outputted over serial for use with the Arduino
 * IDE's Serial Plotter tool, which graphs data in the following
 * format:
 * {current average} {highest value seen so far} {lowest value seeo so far}
 * 
 * Github repo: https://github.com/jasonwebb/PaintingSpinner
 * Author website: http://jason-webb.info
 ********************************************************************/

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

int sensorUpdateInterval = 10;      // in milliseconds
unsigned int sensorLastUpdate = 0;

int sensorLowestValue = 1023;  // represents furthest distance from sensor
int sensorHighestValue = 0;    // represents closest distance to sensor


/***************************************
* General state flags and variables
****************************************/
boolean DEBUG = true;
unsigned int currentTime = 0;


void setup() {
  // Initialize debug mode ---------------------------------------
  if (DEBUG) {
    Serial.begin(9600);
    while(!Serial);
  }

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

  updateSensor();
}

/**************************************************
* Update sensor data
***************************************************/
void updateSensor() {  
  if(currentTime > sensorLastUpdate + sensorUpdateInterval) {
    // Check and update enable switch
    updateSensorEnabledSwitch();
    
    // Update rolling average of samples ---------------------------    
    // Toss out latest sample
    sensorTotal -= sensorSamples[sensorSampleIndex];
  
    // Get new sample from the sensor and store it in buffer
    sensorSamples[sensorSampleIndex] = analogRead(sensorPin);
  
    // Add latest sample to total
    sensorTotal += sensorSamples[sensorSampleIndex];
  
    // Advance or roll over buffer index
    if(sensorSampleIndex < sensorNumSamples - 1)
      sensorSampleIndex++;
    else
      sensorSampleIndex = 0;
  
    // Calculate new average
    sensorAverage = sensorTotal / sensorNumSamples;

    // Update max and min values ---------------------------
    if(sensorAverage > sensorHighestValue)
      sensorHighestValue = sensorAverage;

    if(sensorHighestValue < sensorLowestValue)
      sensorLowestValue = sensorAverage;

    if(DEBUG && sensorEnabled) {
      Serial.print(sensorAverage);
      Serial.print(" ");
      Serial.print(sensorLowestValue);
      Serial.print(" ");
      Serial.println(sensorHighestValue);
    }

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

