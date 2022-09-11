// R2 - Robocon 2022 Lagori - Arduino Mega
// seeker round: carries the ball on head and piles the lagori discs back
// up with suction cups on a platform that moves on two lead screws
// hitter round: picks the 3 hitter balls from the rack and drops them
// into the R1 hopper
//
// left stick = drive (strafe and forward)
// right stick X = turn       right stick Y = platform up / down
// B1 disc cups on/off   B2 side cup on/off   B3 ball cups on/off
// B4 hold = disc arm out     B5 hold = disc arm in
// B7 hold = ball arm out     B8 hold = ball arm in
// B6 platform to the next pile height (disc 1 to 5)
// left click = platform to ball rack height, again for R1 hopper height
// right click = platform to disc pick height
//
// HC-05 on Serial1

#include <AccelStepper.h>

// omni motors on MDD10A: front left, front right, rear left, rear right
int pwmPin[4] = {5, 6, 7, 8};
int dirPin[4] = {22, 24, 26, 28};

// platform lift: two lead screw steppers on two A4988s with the same
// STEP, DIR and EN pins so they always move together
#define LIFT_STEP 30
#define LIFT_DIR 31
#define LIFT_EN 32
#define LIFT_HOME 33           // limit switch at the bottom
#define STEPS_PER_MM 50.0      // 400 steps per rev, 8 mm lead screw
#define LIFT_SPEED 3000

// heights of the suction cups above the floor in mm
#define HOME_MM 150   // platform sitting on the switch
#define MAX_MM 1150   // nothing on R2 can go above the ball on head (1200 mm)
#define DISC_PICK_MM 200
#define BALL_PICK_MM 240  // balls in the ball rack
#define BALL_PASS_MM 950  // just above the R1 hopper

// every disc is 200 mm tall, the top disc is held by the side cup
int pileMm[5] = {420, 620, 820, 1020, 1120};

// relays turn on with LOW
#define DISC_ARM_OUT 34
#define DISC_ARM_IN 35
#define BALL_ARM_OUT 36
#define BALL_ARM_IN 37
#define PUMP 38

// solenoid valves on MOSFETs, on = cups hold, off = part drops
#define VALVE_DISC 39  // two cups facing down
#define VALVE_SIDE 40  // one cup facing sideways for the top disc
#define VALVE_BALL 41  // three cups for the balls

AccelStepper lift(AccelStepper::DRIVER, LIFT_STEP, LIFT_DIR);

// remote
int lx, ly, rx, ry;
int buttons = 0;
int oldButtons = 0;
unsigned long lastMsg = 0;

int motorSpeed[4] = {0, 0, 0, 0};

bool discCups = false;
bool sideCup = false;
bool ballCups = false;

int pileLevel = 0;
bool atRack = false;

int discArmDir = 0;
int ballArmDir = 0;

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

// omni wheels at 45 degrees mix the same way as mecanum
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

// two relays per actuator, dir 1 = out, -1 = in, 0 = stop
void actuator(int outPin, int inPin, int dir) {
  digitalWrite(outPin, HIGH);  // both off first
  digitalWrite(inPin, HIGH);
  if (dir == 0) return;
  delay(100);  // give the relay time before switching the other way
  if (dir == 1) digitalWrite(outPin, LOW);
  else digitalWrite(inPin, LOW);
}

void liftTo(float mm) {
  mm = constrain(mm, HOME_MM, MAX_MM);
  lift.moveTo(mm * STEPS_PER_MM);
}

void homeLift() {
  lift.setSpeed(-LIFT_SPEED / 3);
  unsigned long start = millis();
  while (digitalRead(LIFT_HOME) == HIGH) {
    lift.runSpeed();
    if (millis() - start > 30000) {
      Serial.println("lift home switch not found");
      return;
    }
  }
  lift.setCurrentPosition(HOME_MM * STEPS_PER_MM);
}

void updateValves() {
  digitalWrite(VALVE_DISC, discCups);
  digitalWrite(VALVE_SIDE, sideCup);
  digitalWrite(VALVE_BALL, ballCups);

  // pump only runs when some cups are holding
  if (discCups || sideCup || ballCups) digitalWrite(PUMP, LOW);
  else digitalWrite(PUMP, HIGH);
}

void nextPile() {
  liftTo(pileMm[pileLevel]);
  Serial.print("pile disc ");
  Serial.println(pileLevel + 1);
  pileLevel++;
  if (pileLevel == 5) pileLevel = 0;
}

void sticks() {
  drive(lx, ly, rx * 3 / 5);

  // platform by hand, about 60 mm/s at full stick
  if (ry != 0) {
    float mm = lift.targetPosition() / STEPS_PER_MM;
    liftTo(mm + ry / 210.0);
  }
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

  // write HIGH before pinMode so the relays don't click at power up
  int relays[5] = {DISC_ARM_OUT, DISC_ARM_IN, BALL_ARM_OUT, BALL_ARM_IN, PUMP};
  for (int i = 0; i < 5; i++) {
    digitalWrite(relays[i], HIGH);
    pinMode(relays[i], OUTPUT);
  }

  pinMode(VALVE_DISC, OUTPUT);
  pinMode(VALVE_SIDE, OUTPUT);
  pinMode(VALVE_BALL, OUTPUT);
  updateValves();

  pinMode(LIFT_HOME, INPUT_PULLUP);
  lift.setEnablePin(LIFT_EN);
  lift.setPinsInverted(false, false, true);  // A4988 enable is active low
  lift.setMaxSpeed(LIFT_SPEED);
  lift.setAcceleration(LIFT_SPEED * 2);
  lift.enableOutputs();
  homeLift();

  Serial.println("R2 ready");
}

void loop() {
  readRemote();

  // arms only move while the button is held
  int d = 0;
  if (held(4)) d = 1;
  else if (held(5)) d = -1;
  if (d != discArmDir) {
    actuator(DISC_ARM_OUT, DISC_ARM_IN, d);
    discArmDir = d;
  }

  d = 0;
  if (held(7)) d = 1;
  else if (held(8)) d = -1;
  if (d != ballArmDir) {
    actuator(BALL_ARM_OUT, BALL_ARM_IN, d);
    ballArmDir = d;
  }

  if (pressed(1)) {
    discCups = !discCups;
    updateValves();
  }
  if (pressed(2)) {
    sideCup = !sideCup;
    updateValves();
  }
  if (pressed(3)) {
    ballCups = !ballCups;
    updateValves();
  }
  if (pressed(6)) nextPile();
  if (pressed(9)) {
    atRack = !atRack;
    if (atRack) liftTo(BALL_PICK_MM);
    else liftTo(BALL_PASS_MM);
  }
  if (pressed(10)) liftTo(DISC_PICK_MM);

  // remote lost: stop the platform but keep holding the parts
  if (millis() - lastMsg > 300 && lift.distanceToGo() != 0) lift.stop();

  // drive every 20 ms
  if (millis() - lastTime >= 20) {
    lastTime = millis();
    sticks();
  }

  lift.run();
}
