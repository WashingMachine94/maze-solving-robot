int infraPins[] = {1,2,4,5,6};
int pwmPins[] = {11,3};
int dirPins[] = {12,13};

const int patterns[][5] = {
  {0, 0, 0, 0, 0},   // Stop
  {1, 1, 0, 1, 1},   // Forward
  {1, 1, 1, 1, 1},   // Dead end
  {0, 0, 0, 1, 1},   // Left
  {1, 0, 0, 1, 1},   // Soft Left
  {1, 1, 0, 0, 0},   // Right
  {1, 1, 0, 0, 1}    // Soft Right
};

enum Pattern {
  STOP,
  FORWARD,
  DEAD_END,
  LEFT,
  SOFT_LEFT,
  RIGHT,
  SOFT_RIGHT
};

Pattern CURRENTPATTERN = FORWARD;
bool forward = true;

const int speed = 57;
const int turnSpeed = 57;
const int reverseSpeed = 57;

const bool favorLeft = false;

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
  ReadPattern();
  PatternToAction();
}

void ReadPattern() {
  for (int i = 0; i < 7; i++) {
    bool patternMatch = true;
    for (int j = 0; j < 5; j++) {
      if (digitalRead(infraPins[j]) != patterns[i][j]) {
        patternMatch = false;
        break;
      }
    }
    if (patternMatch) {
      CURRENTPATTERN = static_cast<Pattern>(i);
      return;
    }
  }
}
void PatternToAction() {
  switch(CURRENTPATTERN) {
    case(STOP):
      TestEnd();
      break;
    case(LEFT):
      SteerLeft();
      break;
    case(SOFT_LEFT):
      SoftLeft();
      break;
    case(RIGHT):
      SteerRight();
      break;
    case(SOFT_RIGHT):
      SoftRight();
      break;
    case(DEAD_END):
      RotateBack();
      break;
    default:
      Drive();
  }
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
  for(int i=0; i < 2; i++) { // needs to be detected twice to be sure
    bool keepSteering = true;
    while(keepSteering) {
      ReadPattern();
      keepSteering = CURRENTPATTERN == FORWARD;
    }
    delay(5);
  }

  // turn until new forward pattern is found
  WaitUntilForward();
}
void WaitUntilForward() {
  // wait until forward pattern is found
  bool keepWaiting = true;
  while(keepWaiting) {
    ReadPattern();
    keepWaiting = CURRENTPATTERN != FORWARD;
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
  ReadPattern();
  bool patternPersisted = CURRENTPATTERN == LEFT;

  if(!patternPersisted) {
    if(CURRENTPATTERN == STOP)
      TestEnd();
    return;
  }

  if(!favorLeft) {
    // TEST IF THERE'S FORWARD
    Drive();
    delay(forwardsDelay);

    ReadPattern();
    if(CURRENTPATTERN == FORWARD)
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

  ReadPattern();
  bool patternPersisted = CURRENTPATTERN == RIGHT;

  if(!patternPersisted) {
    if(CURRENTPATTERN == STOP)
      TestEnd();
    return;
  }

  if(favorLeft) {
    // TEST IF THERE'S FORWARD
    Drive();
    delay(forwardsDelay);

    ReadPattern();
    if(CURRENTPATTERN == FORWARD)
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

  ReadPattern();
  if(CURRENTPATTERN != STOP) {
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
    ReadPattern();
    stopped = CURRENTPATTERN == STOP;
  }
}
void RotateBack() {
  delay(200);
  analogWrite(pwmPins[0], turnSpeed);
  analogWrite(pwmPins[1], turnSpeed);
  digitalWrite(dirPins[1], HIGH);

  bool keepSteering = true;
  while(keepSteering) {
    ReadPattern();
    keepSteering = CURRENTPATTERN != FORWARD;
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