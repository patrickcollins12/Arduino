#define R_PWM 23 // Green
#define R_PWM_CH 0
#define PWM_Res   8
#define PWM_Freq  10000

#define BUTTON 22

void setup() {
  Serial.begin(115200);

  ledcSetup(R_PWM_CH, PWM_Freq, PWM_Res);
  ledcAttachPin(R_PWM, R_PWM_CH);

  pinMode(BUTTON, INPUT_PULLUP);
  Serial.println("Hi");
  ledcWrite(R_PWM_CH, 255);  
}

void loop() {

  if (!digitalRead(BUTTON)) {

    Serial.println("Button pressed");
    
    ledcWrite(R_PWM_CH, 0);  
    delay(1000);
    ledcWrite(R_PWM_CH, 255);  
    esp_light_sleep_start();
  }
  
}
