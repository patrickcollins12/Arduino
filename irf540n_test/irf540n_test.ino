#define R_PWM 19 // Green
#define R_PWM_CH 0
#define PWM_Res   8
#define PWM_Freq  10000

void setup() {
  Serial.begin(115200);

  ledcSetup(R_PWM_CH, PWM_Freq, PWM_Res);
  ledcAttachPin(R_PWM, R_PWM_CH);

}

int i=0;
void loop() {
  // put your main code here, to run repeatedly:

  ledcWrite(R_PWM_CH, i);  
  i++;
  if (i==256) i=0;
  Serial.println(i);
  delay(15);
}
