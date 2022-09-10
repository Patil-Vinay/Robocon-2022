// R1 - Robocon 2022 Lagori - Arduino Mega
// seeker round: throws up to 3 balls from R1SZ to break the lagori
// hitter round: gets 3 balls from R2 in the hopper and throws them at the
// ball on head of the other team's R2
// the ball rolls down the pipe and the pusher sends it into two rollers
//
// left stick = drive (strafe and forward)
// right stick X = turn, or turns the table in aim mode
// B1 hold = lift up        B2 hold = lift down
// B3 hold = pusher out     B4 hold = pusher back
// B5 rollers on/off        B6 seeker speed     B7 hitter speed
// B8 drop the next side ball from the hopper
// left click = aim mode on/off     right click = close both hopper gates
//
// HC-05 on Serial1

#include <AccelStepper.h>

// mecanum motors on MDD10A: front left, front right, rear left, rear right
int pwmPin[4] = {5, 6, 7, 8};
int dirPin[4] = {22, 24, 26, 28};

// two roller motors, they spin opposite ways
int rollerPwm[2] = {9, 10};
int rollerDir[2] = {30, 31};

// ball must leave at 30 km/h or less (rulebook)
// set ROLLER_MAX with a speed gun at 4 m on a full battery
#define ROLLER_MAX 200
#define SEEKER_SPEED 180  // breaking the lagori
#define HITTER_SPEED 150  // hitting the ball on head

// relays turn on with LOW
#define LIFT_UP 32  // both lift actuators are on the same relays
#define LIFT_DOWN 33
#define PUSH_OUT 34
#define PUSH_BACK 35

// table stepper, keep it pointing straight ahead at power up
#define TABLE_STEP 36
#define TABLE_DIR 37
#define TABLE_EN 38
#define TABLE_STEPS_PER_DEG 17.8  // 1600 steps per rev and 4:1 gear
#define TABLE_LIMIT 90            // degrees each side

// hopper gate steppers for the two side balls
#define GATE_L_STEP 39
#define GATE_L_DIR 40
#define GATE_L_EN 41
#define GATE_R_STEP 42
#define GATE_R_DIR 43
#define GATE_R_EN 44
#define GATE_OPEN 800  // steps to let one ball through

AccelStepper table(AccelStepper::DRIVER, TABLE_STEP, TABLE_DIR);
AccelStepper gateL(AccelStepper::DRIVER, GATE_L_STEP, GATE_L_DIR);
AccelStepper gateR(AccelStepper::DRIVER, GATE_R_STEP, GATE_R_DIR);

// remote
int lx, ly, rx, ry;
int buttons = 0;
int oldButtons = 0;
unsigned long lastMsg = 0;

int motorSpeed[4] = {0, 0, 0, 0};

bool aimMode = false;
bool rollersOn = false;
int rollerSet = SEEKER_SPEED;
int rollerSpeed = 0;

int liftDir = 0;
int pushDir = 0;

unsigned long lastTime = 0;

void readRemote() {
  oldButtons = buttons;

  if (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');
    int a, b, c, d, e;
    if (sscanf(msg.c_str(), "%d,%d,%d,%d,%d", &a, &b, &c, &d, &e) == 5) {
      lx = a;
      ly = b;
      rx = c;
      ry = d;
      buttons = e;
      lastMsg = millis();
    }
  }

  // nothing from the remote for 300 ms, stop everything
  if (millis() - lastMsg > 300) {
    lx = 0;
    ly = 0;
    rx = 0;
    ry = 0;
    buttons = 0;
    rollersOn = false;
  }
}

bool held(int b) {
  return bitRead(buttons, b - 1);
}

// true only when the button was just pressed
bool pressed(int b) {
  return bitRead(buttons, b - 1) && !bitRead(oldButtons, b - 1);
}

void motor(int i, int speed) {
  if (i == 1 || i == 3) speed = -speed;  // right side is mirrored
  if (speed < 0) digitalWrite(dirPin[i], HIGH);
  else digitalWrite(dirPin[i], LOW);
  analogWrite(pwmPin[i], abs(speed));
}

void drive(int x, int y, int turn) {
  int v[4];
  v[0] = y + x + turn;  // front left
  v[1] = y - x - turn;  // front right
  v[2] = y - x + turn;  // rear left
  v[3] = y + x - turn;  // rear right

  // if one wheel goes over 255 scale all of them down
  int biggest = 255;
  for (int i = 0; i < 4; i++) {
    if (abs(v[i]) > biggest) biggest = abs(v[i]);
  }

  for (int i = 0; i < 4; i++) {
    int target = (long)v[i] * 255 / biggest;
    // change speed slowly so the wheels don't slip
    motorSpeed[i] += constrain(target - motorSpeed[i], -30, 30);
    motor(i, motorSpeed[i]);
  }
}

void roller(int i, int speed) {
  if (i == 1) speed = -speed;
  if (speed < 0) digitalWrite(rollerDir[i], HIGH);
  else digitalWrite(rollerDir[i], LOW);
  analogWrite(rollerPwm[i], abs(speed));
}

// speed up the rollers slowly so they don't pull the battery down
void updateRollers() {
  int target = 0;
  if (rollersOn) target = min(rollerSet, ROLLER_MAX);
  rollerSpeed += constrain(target - rollerSpeed, -10, 10);
  roller(0, rollerSpeed);
  roller(1, rollerSpeed);
}

// two relays per actuator, dir 1 = out, -1 = in, 0 = stop
void actuator(int outPin, int inPin, int dir) {
  digitalWrite(outPin, HIGH);  // both off first
  digitalWrite(inPin, HIGH);
  if (dir == 0) return;
  delay(100);  // give the relay time before switching the other way
  if (dir == 1) digitalWrite(outPin, LOW);
  else digitalWrite(inPin, LOW);
}

void nextBall() {
  if (gateL.targetPosition() == 0) gateL.moveTo(GATE_OPEN);
  else if (gateR.targetPosition() == 0) gateR.moveTo(GATE_OPEN);
}

void sticks() {
  int turn = rx;

  if (aimMode) {
    // turn the table instead of the robot, about 45 deg/s at full stick
    long pos = table.targetPosition() + rx * TABLE_STEPS_PER_DEG / 280;
    long limit = TABLE_LIMIT * TABLE_STEPS_PER_DEG;
    table.moveTo(constrain(pos, -limit, limit));
    turn = 0;
  }

  drive(lx, ly, turn * 3 / 5);
  updateRollers();
}

void setupStepper(AccelStepper &s, int en, int maxSpeed) {
  s.setEnablePin(en);
  s.setPinsInverted(false, false, true);  // A4988 enable is active low
  s.setMaxSpeed(maxSpeed);
  s.setAcceleration(maxSpeed * 2);
  s.enableOutputs();
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial1.setTimeout(50);

  for (int i = 0; i < 4; i++) {
    pinMode(pwmPin[i], OUTPUT);
    pinMode(dirPin[i], OUTPUT);
    motor(i, 0);
  }
  for (int i = 0; i < 2; i++) {
    pinMode(rollerPwm[i], OUTPUT);
    pinMode(rollerDir[i], OUTPUT);
    roller(i, 0);
  }

  // write HIGH before pinMode so the relays don't click at power up
  int relays[4] = {LIFT_UP, LIFT_DOWN, PUSH_OUT, PUSH_BACK};
  for (int i = 0; i < 4; i++) {
    digitalWrite(relays[i], HIGH);
    pinMode(relays[i], OUTPUT);
  }

  setupStepper(table, TABLE_EN, 1500);
  setupStepper(gateL, GATE_L_EN, 1000);
  setupStepper(gateR, GATE_R_EN, 1000);
  Serial.println("R1 ready");
}

void loop() {
  readRemote();

  // actuators only move while the button is held
  int d = 0;
  if (held(1)) d = 1;
  else if (held(2)) d = -1;
  if (d != liftDir) {
    actuator(LIFT_UP, LIFT_DOWN, d);
    liftDir = d;
  }

  d = 0;
  if (held(3)) d = 1;
  else if (held(4)) d = -1;
  if (d != pushDir) {
    actuator(PUSH_OUT, PUSH_BACK, d);
    pushDir = d;
  }

  if (pressed(5)) rollersOn = !rollersOn;
  if (pressed(6)) rollerSet = SEEKER_SPEED;
  if (pressed(7)) rollerSet = HITTER_SPEED;
  if (pressed(8)) nextBall();
  if (pressed(9)) aimMode = !aimMode;
  if (pressed(10)) {
    gateL.moveTo(0);
    gateR.moveTo(0);
  }

  // drive and rollers every 20 ms
  if (millis() - lastTime >= 20) {
    lastTime = millis();
    sticks();
  }

  table.run();
  gateL.run();
  gateR.run();
}
