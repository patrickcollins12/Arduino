#define R_PWM 18 // Green
#define L_PWM 19 // Blue
#define R_EN 18
#define L_EN 19
//#define L_IS 35 // Purple
//#define R_IS 32 // Gray

void setup() {
  Serial.begin(115200);

  pinMode(L_EN, OUTPUT);
  pinMode(R_EN, OUTPUT);

  pinMode(L_PWM, OUTPUT);
  pinMode(R_PWM, OUTPUT);

}

void pull() {
  digitalWrite(R_EN, HIGH);
  digitalWrite(R_PWM, HIGH);

  digitalWrite(L_EN, LOW);
  digitalWrite(L_PWM, LOW);

  Serial.println("pull");
}

void push() {
  digitalWrite(R_EN, LOW);
  digitalWrite(R_PWM, LOW);

  digitalWrite(L_EN, HIGH);
  digitalWrite(L_PWM, HIGH);

  Serial.println("push");
}


void stop() {
  digitalWrite(R_EN, LOW);
  digitalWrite(R_PWM, LOW);

  digitalWrite(L_EN, LOW);
  digitalWrite(L_PWM, LOW);

  Serial.println("stop");
}


void loop() {

  push();
  delay(200);

  stop();
  delay(1000);

  pull();
  delay(200);

  stop();
  delay(1000);

}
