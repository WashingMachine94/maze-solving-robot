const int maxDistance = 12;
unsigned long startTime = 0;
unsigned long endTime = 0;
bool started = false;

//display code
const int segmentPins[7] = {14, 15, 16, 17, 18, 19, 0}; // A-G
const int displayControlPins[2] = {8, 9}; // Display 1 and Display 2 control pins

bool letters[4][7] = {
  {HIGH, LOW, LOW, LOW, HIGH, HIGH, HIGH},    // F
  {LOW, LOW, LOW, LOW, HIGH, HIGH, LOW},      // I
  {HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH},   // S
  {LOW, LOW, LOW, HIGH, HIGH, HIGH, HIGH}     // T
};
bool numbers[10][7] = {
  {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW},  // 0
  {LOW, HIGH, HIGH, LOW, LOW, LOW, LOW},      // 1
  {HIGH, HIGH, LOW, HIGH, HIGH, LOW, HIGH},   // 2
  {HIGH, HIGH, HIGH, HIGH, LOW, LOW, HIGH},   // 3
  {LOW, HIGH, HIGH, LOW, LOW, HIGH, HIGH},    // 4
  {HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH},   // 5
  {HIGH, LOW, HIGH, HIGH, HIGH, HIGH, HIGH},  // 6
  {HIGH, HIGH, HIGH, LOW, LOW, LOW, LOW},     // 7
  {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}, // 8
  {HIGH, HIGH, HIGH, HIGH, LOW, HIGH, HIGH}   // 9
};

const int infraPins[] = {1,2,4,5,6};
const int pwmPins[] = {11,3};
const int dirPins[] = {12,13};
const int ultraPins[] = {7,10}; // (trig pin , echo pin)

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

const int speed = 70;
const int turnSpeed = 70;
const int reverseSpeed = 70;
const bool favorLeft = false;
const int retryDuration = 40;
const int forwardsDelay = 300;
const float turnSpeedMultiplier = 0.6;

void setup() {
  TCCR2B = TCCR2B & B11111000 | B00000111;

  for(int i = 0; i < 7; i++)
    pinMode(segmentPins[i], OUTPUT);
  for(int i = 0; i < 2; i++) {
    pinMode(displayControlPins[i], OUTPUT);
    digitalWrite(displayControlPins[i], HIGH);
  }
  pinMode(ultraPins[0], OUTPUT);
  pinMode(ultraPins[1], INPUT);

  for(int dirPin : dirPins)
    pinMode(dirPin, OUTPUT);
  for(int pwmPin : pwmPins)
    pinMode(pwmPin, OUTPUT);
  startTime = millis();
}
void loop() {
  if(!started) {
    if(millis() - startTime > 10000) {
      // display ST
      for(int i=0; i < 100; i++)
        showLetters(2,3);

      started = true;
      startTime = millis();
      return;
    }
    showNumber(10 - (millis() - startTime) / 1000);
    return;
  }
  CheckForObstacle();
  SetDirection();
  ReadPattern();
  PatternToAction();
}

void ReadPattern() {
  UpdateTimer();
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
      TestFinish();
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
void CheckForObstacle() {
  digitalWrite(ultraPins[0], HIGH);
  delayMicroseconds(10);
  digitalWrite(ultraPins[0], LOW);
  float duration = pulseIn(ultraPins[1], HIGH);
  float distance = duration / 58;

  if(distance <= maxDistance)
    RotateBack();
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

  WaitUntilForward();
}
void WaitUntilForward() {
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
      TestFinish();
    return;
  }

  if(!favorLeft) {
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
      TestFinish();
    return;
  }

  if(favorLeft) {
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

void TestFinish() {
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

  Stop();
  endTime = (millis() - startTime) / 1000;

  // blink endTime;
  for(int i=0; i < 3; i++) {
    delay(500);
    for(int j=0; j < 100; j++)
      showNumber(endTime);
  }
      
  // stop until pattern breaks
  bool stopped = true;
  while(stopped) {
    showLetters(0,1);
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

void UpdateTimer() {
  showNumber((millis() - startTime) / 1000);
}
void showNumber(int number) {
  int tens = number / 10; // Calculate tens place
  int ones = number % 10; // Calculate ones place

  // Display the tens digit on the first display
  for(int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], numbers[tens][i]);
  }
  digitalWrite(displayControlPins[0], LOW); // Enable the first display (assuming common cathode)
  delay(5); // Short delay for the human eye to catch the display
  digitalWrite(displayControlPins[0], HIGH); // Disable the first display

  // Display the ones digit on the second display
  for(int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], numbers[ones][i]);
  }
  digitalWrite(displayControlPins[1], LOW); // Enable the second display
  delay(5); // Short delay
  digitalWrite(displayControlPins[1], HIGH); // Disable the second display
}
void showLetters(int index, int index1) {
  for(int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], letters[index][i]);
  }
  digitalWrite(displayControlPins[0], LOW); // Enable the first display (assuming common cathode)
  delay(5); // Short delay for the human eye to catch the display
  digitalWrite(displayControlPins[0], HIGH); // Disable the first display

  // Display the ones digit on the second display
  for(int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], letters[index1][i]);
  }
  digitalWrite(displayControlPins[1], LOW); // Enable the second display
  delay(5); // Short delay
  digitalWrite(displayControlPins[1], HIGH); // Disable the second display
}