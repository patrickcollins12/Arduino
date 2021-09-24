

uint8_t led_gpio = 11; // the PWM pin the LED is attached to

// the setup routine runs once when you press reset:
void setup() {
  // declare pin 9 to be an output:
//  pinMode(led, OUTPUT);
  ledcSetup(0, 4000, 8); // 12 kHz PWM, 8-bit resolution
  ledcAttachPin(32, 0);
  
  // Initialize channels
  // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
  // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
  Serial.begin(115200);
}

// the loop routine runs over and over again forever:
int i=0;
void loop() {
  i++;
  ledcWrite(0, i); // set the brightness of the LED
  
  Serial.println(i);

  if (i>255) {
    i=0;
  }
  
  // wait for 30 milliseconds to see the dimming effect
  delay(30);
}
