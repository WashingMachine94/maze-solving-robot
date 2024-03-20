int infraPins[] = {1,2,4,5,6};
int pwmPins[] = {11,3};
int dirPins[] = {12,13};

int stopPattern[] = {0,0,0,0,0};
int forwardPattern[] = {1,1,0,1,1};
int deadEndPattern[] = {1,1,1,1,1};
int leftPattern[] = {0,0,0,1,1};
int softLeftPattern[] {1,0,0,1,1};
int rightPattern[] = {1,1,0,0,0};
int softRightPattern[] {1,1,0,0,1};

bool forward = true;

const int speed = 45;
const int turnSpeed = 45;
const int reverseSpeed = 35;

const bool favorLeft = true;

const int forwardsDelay = 300;
const int retryDuration = 40;

const float turnSpeedMultiplier = 0.6;

void setup() {
  //TCCR2B = TCCR2B & B11111000 | B00000110
  TCCR2B = TCCR2B & B11111000 | B00000111; // for PWM frequency of 122.55 Hz

  for(int dirPin : dirPins)
    pinMode(dirPin, OUTPUT);
  for(int pwmPin : pwmPins)
    pinMode(pwmPin, OUTPUT);

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
}
void Stop() {
  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], 0);
}
void TurnUntilForward() {
  // turn until forward pattern is broken
  bool keepSteering = true;
  while(keepSteering) {
    keepSteering = true;

    for(int i = 0; i < 5; i++)
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = false;
  }
  delay(5);
  while(keepSteering) {
    keepSteering = true;

    for(int i = 0; i < 5; i++)
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = false;
  }

  // turn until new forward pattern is found
  keepSteering = true;
  while(keepSteering) {
    keepSteering = false;

    for(int i = 0; i < 5; i++)
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
  }
}
void ReverseUntilForward() {
  analogWrite(pwmPins[0], reverseSpeed);
  analogWrite(pwmPins[1], reverseSpeed);
  forward = false;
  SetDirection();
  for(int i=0; i < 2; i++) {
    bool goBackwards = true;
    while(goBackwards) {
      goBackwards = false;
      for(int i=0; i < 5; i++)
        if(digitalRead(infraPins[i]) != forwardPattern[i])
          goBackwards = true;
    }
  }
  
  forward = true;
  SetDirection();
  delay(retryDuration);
}
void SteerLeft() {
  delay(retryDuration);
  bool patternPersisted = true;
  for(int i=0; i<5; i++)
    if(digitalRead(infraPins[i]) != leftPattern[i])
      patternPersisted = false;

  if(!patternPersisted)
    return;

  if(!favorLeft) {
    // TEST IF THERE'S FORWARD
    bool forwardFound = true;
    bool deadEndFound = true;
    Drive();
    delay(forwardsDelay);

    for(int i=0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        forwardFound = false;
      if(digitalRead(infraPins[i]) != deadEndPattern[i])
        deadEndFound = false;
    }

    if(forwardFound)
      return;
    if(deadEndFound)
      ReverseUntilForward();
  }
  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], turnSpeed);

  TurnUntilForward();
}
void SoftLeft() {
  analogWrite(pwmPins[0], turnSpeed * turnSpeedMultiplier);
  analogWrite(pwmPins[1], turnSpeed);
}
void AbsoluteLeft() {
  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], turnSpeed);
  TurnUntilForward();
}
void SteerRight() {
  delay(retryDuration);
  bool patternPersisted = true;
  for(int i=0; i<5; i++)
    if(digitalRead(infraPins[i]) != rightPattern[i])
      patternPersisted = false;

  if(!patternPersisted)
    return;

  if(favorLeft) {
    // TEST IF THERE'S FORWARD
    bool forwardFound = true;
    bool deadEndFound = true;
    Drive();
    delay(forwardsDelay);

    for(int i=0; i < 5; i++) {
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        forwardFound = false;
      if(digitalRead(infraPins[i]) != deadEndPattern[i])
        deadEndFound = false;
    }

    if(forwardFound)
      return;
    if(deadEndFound)
      ReverseUntilForward();
  }
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], 0);

  TurnUntilForward();
}
void SoftRight() {
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], turnSpeed * turnSpeedMultiplier);
}
void AbsoluteRight() {
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], 0);
  TurnUntilForward();
}

void TestEnd() {
  bool stopFound = true;
  Drive();

  delay(forwardsDelay);

  for(int i=0; i < 5; i++)
    if(digitalRead(infraPins[i]) != stopPattern[i])
      stopFound = false;

  if(!stopFound) {
    ReverseUntilForward();
    
    if(favorLeft) {
      delay(30);
      AbsoluteLeft();
    } else {
      AbsoluteRight();
    }
    return;
  }

  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], 0);

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
    for(int i = 0; i < 5; i++)
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepSteering = true;
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