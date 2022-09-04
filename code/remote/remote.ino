// Remote for R1 and R2 (Arduino Nano)
// two joysticks, 8 buttons and a HC-05 paired with the robot
// sends "lx,ly,rx,ry,buttons" every 40 ms

#include <SoftwareSerial.h>

SoftwareSerial bt(2, 3);  // RX, TX (HC-05 RX through a divider)

int btnPin[10] = {4, 5, 6, 7, 8, 9, 10, 11, A4, A5};  // B1 to B8, left click, right click

int readStick(int pin) {
  int v = map(analogRead(pin), 0, 1023, -255, 255);
  if (abs(v) < 25) v = 0;  // dead zone in the middle
  return v;
}

void setup() {
  for (int i = 0; i < 10; i++) {
    pinMode(btnPin[i], INPUT_PULLUP);
  }
  pinMode(13, OUTPUT);
  bt.begin(9600);
}

void loop() {
  int lx = readStick(A0);
  int ly = -readStick(A1);  // Y is upside down on these joysticks
  int rx = readStick(A2);
  int ry = -readStick(A3);

  int buttons = 0;
  for (int i = 0; i < 10; i++) {
    if (digitalRead(btnPin[i]) == LOW) bitSet(buttons, i);
  }

  bt.print(lx);
  bt.print(",");
  bt.print(ly);
  bt.print(",");
  bt.print(rx);
  bt.print(",");
  bt.print(ry);
  bt.print(",");
  bt.println(buttons);

  // led on when any button is pressed
  if (buttons > 0) digitalWrite(13, HIGH);
  else digitalWrite(13, LOW);

  delay(40);
}
