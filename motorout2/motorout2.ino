/*
  AnalogReadSerial

  Reads an analog input on pin 0, prints the result to the Serial Monitor.
  Graphical representation is available using Serial Plotter (Tools > Serial Plotter menu).
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/AnalogReadSerial
*/

#include "Arduino.h";

#define MOTOR 11

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(MOTOR, OUTPUT);
  Serial.begin(9600);
}

int i=0;
// the loop function runs over and over again forever
void loop() {
  i++;
  digitalWrite(MOTOR, i);
  Serial.println(i);

  if (i>255) {
    i=0;
  }

  delay(100);                       // wait for a second
}
