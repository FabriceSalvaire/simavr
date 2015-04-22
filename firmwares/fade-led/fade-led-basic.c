/*
  Fade

  This example shows how to fade an LED on pin 9 using the analogWrite() function.

  This example code is in the public domain.
*/

int led = 13;   // the pin that the LED is attached to
int brightness = 128;   // how bright the LED is

// the setup routine runs once when you press reset:
void setup() {
  // declare pin 9 to be an output:
  pinMode(led, OUTPUT);
  // set the brightness of pin 9:
  analogWrite(led, brightness);
}

// the loop routine runs over and over again forever:
void loop() {
}
