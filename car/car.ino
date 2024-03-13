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

const bool favorLeft = true;

bool forward = true;
bool isDriving = false;

//int speed = 35;
//int turnSpeed = 35;
const int speed = 45;
const int turnSpeed = 45;
const float turnSpeedMultiplier = 0.6;

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
}

int UpdateState() {
  bool rotateAround = true;
  bool leftFound = true;
  bool rightFound = true;

  bool stopFound = true;
  bool forward = true;

  bool softLeftFound = true;
  bool softRightFound = true;

  for(int i = 0; i < 5; i++) {
    if(digitalRead(infraPins[i]) != forwardPattern[i])
      forward = false;
    if(digitalRead(infraPins[i]) != stopPattern[i])
      stopFound = false;
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

  if(forward)
    Drive();
  if(rotateAround)
    RotateBack();
  if(softLeftFound)
    SoftLeft();
  if(softRightFound)
    SoftRight();
  if(leftFound)
    SteerLeft();
  if(rightFound)
    SteerRight();
  if(stopFound)
    TestEnd();
  return 0;
}
void Drive() {
  analogWrite(pwmPins[0], speed);
  analogWrite(pwmPins[1], speed);
  digitalWrite(brakePins[0], LOW);
  digitalWrite(brakePins[1], LOW);
}
void Stop() {
  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], 0);
  digitalWrite(brakePins[0], HIGH);
  digitalWrite(brakePins[1], HIGH);
}
void SteerLeft() {
  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], turnSpeed);
  digitalWrite(brakePins[0], HIGH);
  digitalWrite(brakePins[1], LOW);

  bool keepSteering = true;
  while(keepSteering) {
    keepSteering = false;
    bool stop = true;

    for(int i = 0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
      if(digitalRead(infraPins[i]) != stopPattern[i])
        stop = false;
    }

    if(stop) {
      TestEnd();
      return;
    }
  }
}
void SoftLeft() {
  analogWrite(pwmPins[0], turnSpeed * turnSpeedMultiplier);
  analogWrite(pwmPins[1], turnSpeed);
}
void AbsoluteLeft() {
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
}
void SteerRight() {
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], 0);
  digitalWrite(brakePins[0], LOW);
  digitalWrite(brakePins[1], HIGH);

  bool keepSteering = true;
  while(keepSteering) {
    keepSteering = false;
    bool stop = true;

    for(int i = 0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
      if(digitalRead(infraPins[i]) != stopPattern[i])
        stop = false;
    }

    if(stop) {
      TestEnd();
      return;
    }
  }
}
void SoftRight() {
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], turnSpeed * turnSpeedMultiplier);
}
void AbsoluteRight() {
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], 0);
  digitalWrite(brakePins[0], LOW);
  digitalWrite(brakePins[1], HIGH);

  bool keepSteering = true;
  while(keepSteering) {
    keepSteering = false;
    bool stop = true;

    for(int i = 0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
    }
  }
}

void TestEnd() {
  bool stopFound = true;
  Drive();
  delay(400);

  for(int i=0; i < 5; i++)
    if(digitalRead(infraPins[i]) != stopPattern[i])
      stopFound = false;

  if(!stopFound) {
    forward = false;
    SetDirection();
    delay(750);
    forward = true;
    SetDirection();

    if(favorLeft) {
      AbsoluteLeft();
    } else {
      AbsoluteRight();
    }
    return;
  }

  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], 0);
  digitalWrite(brakePins[0], HIGH);
  digitalWrite(brakePins[1], HIGH);

  bool stopped = true;
  while(stopped) {
    bool temp = true;
    for(int i=0; i < 5; i++)
      if(digitalRead(infraPins[i]) != stopPattern[i])
        temp = false;
    
    if(!temp)
      stopped = false;
  }
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