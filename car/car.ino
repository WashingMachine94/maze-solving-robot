int infraPins[] = {1,2,4,5,6};
int pwmPins[] = {11,3};
int brakePins[] = {9,8};
int dirPins[] = {12,13};

int stopPattern[] = {0,0,0,0,0};
int forwardPattern[] = {1,1,0,1,1};
int deadEndPattern[] = {1,1,1,1,1};
int leftPattern[] = {0,0,0,1,1};
int softLeftPattern[] {1,0,0,1,1};
int rightPattern[] = {1,1,0,0,0};
int softRightPattern[] {1,1,0,0,1};

bool favorLeft = true;

bool forward = true;
bool isDriving = false;
bool steerLeft = false;
int leftCount = 0;
bool steerRight = false;
int rightCount  = 0;

//int speed = 35;
//int turnSpeed = 35;
int speed = 45;
int turnSpeed = 45;

void setup() {
  //TCCR2B = TCCR2B & B11111000 | B00000110
  TCCR2B = TCCR2B & B11111000 | B00000111; // for PWM frequency of 122.55 Hz

  for(int dirPin : dirPins)
    pinMode(dirPin, OUTPUT);
  for(int pwmPin : pwmPins)
    pinMode(pwmPin, OUTPUT);
  for(int brakePin : brakePins)
    pinMode(brakePin, OUTPUT);

    Serial.begin(9600);
}
void loop() {
  UpdateState();
  SetDirection();
  UpdateDriving();
}

int UpdateState() {

  bool rotateAround = true;

  bool leftFound = favorLeft;
  
  bool rightFound = !favorLeft;
  bool stopFound = true;
  bool softLeftFound = true;
  bool softRightFound = true;

  for(int i = 0; i < 5; i++) {
    if(digitalRead(infraPins[i]) != stopPattern[i]) {
      isDriving = true;
      stopFound = false;
    }
    if(digitalRead(infraPins[i]) != deadEndPattern[i])
      rotateAround = false;
    if(digitalRead(infraPins[i]) != leftPattern[i])
      leftFound = false;
    if(digitalRead(infraPins[i]) != rightPattern[i])
      rightFound = false;
    if(digitalRead(infraPins[i]) != softLeftPattern[i])
      softLeftFound = false;
    if(digitalRead(infraPins[i]) != softRightPattern[i])
      softRightFound = false;
  }

  if(rotateAround)
    RotateBack();
  if(softLeftFound)
    SoftLeft();
  if(softRightFound)
    SoftRight();
  if(leftFound)
    steerLeft = true;
  if(rightFound)
    steerRight = true;
  if(stopFound)
    return 1;
  return 0;
}

void UpdateDriving() {
  if(isDriving)  {
    if(steerLeft) {
      delay(50);
      UpdateState();
      if(!steerLeft) {
        if(favorLeft) {
          SteerLeft();
        } else {
          SteerRight();
        }
        return;
      }

      SteerLeft();
    } else if(steerRight) {
      delay(50);
      UpdateState();
      if(!steerRight) {
        if(favorLeft) {
          SteerLeft();
        } else {
          SteerRight();
        }
        return;
      }
      SteerRight();
    } else {
      analogWrite(pwmPins[0], speed);
      analogWrite(pwmPins[1], speed);
      digitalWrite(brakePins[0], LOW);
      digitalWrite(brakePins[1], LOW);
    }
  }
  if(!isDriving) {
    analogWrite(pwmPins[0], 0);
    analogWrite(pwmPins[1], 0);
    digitalWrite(brakePins[0], HIGH);
    digitalWrite(brakePins[1], HIGH);
  }
}

void SteerLeft() {
  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], turnSpeed);
  digitalWrite(brakePins[0], HIGH);
  digitalWrite(brakePins[1], LOW);

  bool keepSteering = true;
  while(keepSteering) {
    keepSteering = false;
    for(int i = 0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
    }
  }
  steerLeft = false;
}
void SoftLeft() {
  analogWrite(pwmPins[0], turnSpeed * 0.6);
  analogWrite(pwmPins[1], turnSpeed);

  bool keepSteering = true;
  while(keepSteering) {
    keepSteering = false;
    bool left = true;

    for(int i = 0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
      if(digitalRead(infraPins[i]) != leftPattern[i])
        left = false;
    }
    if(left) {
      if(favorLeft) {
        SteerLeft();
        return;
      }
      keepSteering = false;
    }
  }
}
void SteerRight() {
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], 0);
  digitalWrite(brakePins[0], LOW);
  digitalWrite(brakePins[1], HIGH);

  bool keepSteering = true;
  while(keepSteering) {
    keepSteering = false;
    for(int i = 0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
    }
  }
  steerRight = false;
}
void SoftRight() {
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], turnSpeed * 0.6);

  bool keepSteering = true;
  while(keepSteering) {
    keepSteering = false;
    bool right = true;
    for(int i = 0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
      if(digitalRead(infraPins[i]) != rightPattern[i])
        right = false;
    }

    if(right) {
      if(!favorLeft) {
        SteerRight();
        return;
      }
      keepSteering = false;
    }
  }
}

void TestEnd() {
  if(UpdateState() != 1) return;
  analogWrite(pwmPins[0], speed);
  analogWrite(pwmPins[1], speed);
  digitalWrite(brakePins[0], LOW);
  digitalWrite(brakePins[1], LOW);
  delay(400);
  if(UpdateState() != 1) {
    if(favorLeft) {
      SteerLeft();
    } else {
      SteerRight();
    }
    return;
  }

  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], 0);
  digitalWrite(brakePins[0], HIGH);
  digitalWrite(brakePins[1], HIGH);

  while(true) {}

}
void RotateBack() {
  delay(200);
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], turnSpeed);
  digitalWrite(dirPins[1], HIGH);

  bool keepSteering = true;
  while(keepSteering) {
    keepSteering = false;
    for(int i = 0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
    }
  }
  digitalWrite(dirPins[1], LOW);
}

void SetDirection() {
  for(int dirPin : dirPins) {
    if(!forward)
      digitalWrite(dirPin, HIGH);
    if(forward)
      digitalWrite(dirPin, LOW);
  }
}