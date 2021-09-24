#define R_PWM 27 // Green
#define L_PWM 14 // Blue
#define L_EN 0
#define R_EN 0
#define L_IS 35 // Purple
#define R_IS 32 // Gray

#define CURRENT_SENSOR 36 // 

#define R_PWM_CH 0
#define L_PWM_CH 1
#define PWM_Res   8
#define PWM_Freq  10000

void setup() {
  Serial.begin(115200);q

  //  pinMode(R_PWM, OUTPUT);
  ledcSetup(R_PWM_CH, PWM_Freq, PWM_Res);
  ledcAttachPin(R_PWM, R_PWM_CH);
  

  //  pinMode(L_PWM, OUTPUT);
  ledcSetup(L_PWM_CH, PWM_Freq, PWM_Res);
  ledcAttachPin(L_PWM, L_PWM_CH);

//  pinMode(L_EN, OUTPUT);
//  pinMode(R_EN, OUTPUT);

  pinMode(L_IS, INPUT);
  pinMode(R_IS, INPUT);

  pinMode(CURRENT_SENSOR, INPUT_PULLDOWN);

//  digitalWrite(L_EN, LOW);
//  digitalWrite(R_EN, LOW);
  //  digitalWrite(L_PWM, LOW);
  //  digitalWrite(R_PWM, LOW);
}

int i_ss = 0;
int direction = 0;
void loop() {

//  digitalWrite(R_EN, HIGH);
//  digitalWrite(L_EN, LOW);

  if (direction == 0) {
    ledcWrite(L_PWM_CH, 0);
    ledcWrite(R_PWM_CH, i_ss);  
  }
  if (direction == 1) {
    ledcWrite(R_PWM_CH, 0);
    ledcWrite(L_PWM_CH, i_ss);  
  }

  i_ss++;
  if (i_ss == 255) {
    i_ss = 0;

    direction = !direction;

  }
  
  
  delay(20);

  // 3.3v input
  // current sensor goes up to 5v though
  // 185mV / A.
  // The range for the motor seems to go from 0 amp to about 1amp.
  // It centers on 2.5v
  // So center point is 2.5/3.3v * 4096 = 3103
  // It max (1A) it will go 3288.
  // So let's scale from 3103 to 3288.
  
  float cr = analogRead(CURRENT_SENSOR) ;
  cr = map(cr, 3103,3288, 0, 10000);
//  Serial.printf("i %3d direction %3d direction (Current sensor: %0.2f) (L Sense: %4d R Sense: %4d)\n", i_ss, direction, cr, analogRead(L_IS), analogRead(R_IS) );
  Serial.printf("%0.2f\n", cr);
}
