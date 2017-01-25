/***************************************************************
 * PaintingSpinner - Motor test
 * Built by Jason Webb for artist Nadia Shinkunas
 * 
 * DESCRIPTION:
 * Revs up and down motor speed to (1) verify operation and
 * (2) allow for quick tuning of program variables.
 * 
 * Github repo: https://github.com/jasonwebb/PaintingSpinner
 * Author website: http://jason-webb.info
 ***************************************************************/

/** Motor parameters *************/
const boolean CW = LOW;
const boolean CCW = HIGH;

boolean motorDirection = CW;

byte motorPwmPin = 3;
byte motorDirPin = 2;

int motorSpeed = 0;
int motorMaxSpeed = 255;
int motorAcceleration = 1;

boolean motorAtSpeed = false;

const boolean UP = 0;
const boolean DOWN = 1;

boolean rampDirection = UP;

/** Debugging mode ***************/
boolean DEBUG = true;


void setup() {
  // Initialize debug mode ---------------------------------------
  if (DEBUG)
    Serial.begin(9600);

  // Initialize motor driver shield -------------------------------
  pinMode(motorPwmPin, OUTPUT);
  pinMode(motorDirPin, OUTPUT);

  analogWrite(motorPwmPin, motorSpeed);
  digitalWrite(motorDirPin, motorDirection);

  if(DEBUG)
    Serial.println("Ramping up (CW) ...");
}

void loop() {
  switch(rampDirection) {
    // Ramp up in current direction, then flip to ramp down mode
    case UP:
      if(motorSpeed < motorMaxSpeed) {
        motorSpeed += motorAcceleration;
      } else {
        rampDirection = DOWN;

        if(DEBUG) {
          Serial.println("done");
    
          if(motorDirection == CW)
            Serial.print("Ramping down (CW) ... ");
          else
            Serial.print("Ramping down (CCW) ... ");
        }
      }

      break;

    // Ramp down in current direction, then flip to ramp up mode and flip direction
    case DOWN:
      if(motorSpeed > 0) {
        motorSpeed -= motorAcceleration;
      } else {
        rampDirection = UP;

        if(DEBUG)
          Serial.println("done");          

        if(motorDirection == CW) {
          motorDirection = CCW;

          if(DEBUG)
            Serial.print("Ramping up (CCW) ... ");
        } else {
          motorDirection = CW;

          if(DEBUG)
            Serial.print("Ramping up (CW) ... ");
        }
      }

      break;
  }

  digitalWrite(motorDirPin, motorDirection);
  analogWrite(motorPwmPin, motorSpeed);
}
