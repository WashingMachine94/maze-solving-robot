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

const int speed = 50;
const int turnSpeed = 50;
const int reverseSpeed = 50;

const bool favorLeft = true;

const int forwardsDelay = 300;
const int retryDuration = 40;

const float turnSpeedMultiplier = 0.6;

void setup() {
  TCCR2B = TCCR2B & B11111000 | B00000111;

  for(int dirPin : dirPins)
    pinMode(dirPin, OUTPUT);
  for(int pwmPin : pwmPins)
    pinMode(pwmPin, OUTPUT);
}
void loop() {
  SetDirection();
  UpdateState();
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
void WaitUntilForward() {
  // wait until forward pattern is found
  bool keepWaiting = true;
  while(keepWaiting) {
    keepWaiting = false;

    for(int i = 0; i < 5; i++)
      if(digitalRead(infraPins[i]) != forwardPattern[i])
        keepWaiting = true;
  }
}
void SteerLeftInPlace() {
  digitalWrite(dirPins[0], LOW);
  digitalWrite(dirPins[1], HIGH);
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], turnSpeed);
  WaitUntilForward();
  forward = true;
  SetDirection();
}
void SteerRightInPlace() {
  digitalWrite(dirPins[0], HIGH);
  digitalWrite(dirPins[1], LOW);
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], turnSpeed);
  WaitUntilForward();
  forward = true;
  SetDirection();
}
void SteerLeft() {
  delay(retryDuration);
  bool patternPersisted = true;
  for(int i=0; i<5; i++)
    if(digitalRead(infraPins[i]) != leftPattern[i])
      patternPersisted = false;

  if(!patternPersisted) {
    bool stopFound = true;
    for(int i=0; i<5; i++)
      if(digitalRead(infraPins[i]) != stopPattern[i])
        stopFound = false;

    if(stopFound)
      TestEnd();
    return;
  }

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
    SteerLeftInPlace();
    return;
  }
  analogWrite(pwmPins[0], 0);
  analogWrite(pwmPins[1], turnSpeed);

  TurnUntilForward();
}
void SoftLeft() {
  analogWrite(pwmPins[0], turnSpeed * turnSpeedMultiplier);
  analogWrite(pwmPins[1], turnSpeed);
}
void SteerRight() {
  delay(retryDuration);
  bool patternPersisted = true;
  for(int i=0; i<5; i++)
    if(digitalRead(infraPins[i]) != rightPattern[i])
      patternPersisted = false;

  if(!patternPersisted) {
    bool stopFound = true;
    for(int i=0; i<5; i++)
      if(digitalRead(infraPins[i]) != stopPattern[i])
        stopFound = false;

    if(stopFound)
      TestEnd();
    return;
  }

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
    SteerRightInPlace();
    return;
  }
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], 0);

  TurnUntilForward();
}
void SoftRight() {
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], turnSpeed * turnSpeedMultiplier);
}

void TestEnd() {
  Drive();
  delay(forwardsDelay);

  bool stopFound = true;
  for(int i=0; i < 5; i++)
    if(digitalRead(infraPins[i]) != stopPattern[i])
      stopFound = false;

  if(!stopFound) {
    if(favorLeft)
      SteerLeftInPlace();
    if(!favorLeft)
      SteerRightInPlace();
    return;
  }

  // stop until pattern breaks
  Stop();
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