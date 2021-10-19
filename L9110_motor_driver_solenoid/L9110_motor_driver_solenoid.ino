#define FWD 18
#define REV 19
#define BUTTON 5

void setup() {
  Serial.begin(115200);

  pinMode(FWD, OUTPUT);
  pinMode(REV, OUTPUT);
  pinMode(BUTTON, INPUT);
}

void push() {
  digitalWrite(FWD, HIGH);
  digitalWrite(REV, LOW);
}


void pull() {
  digitalWrite(FWD, LOW);
  digitalWrite(REV, HIGH);
}

void stop() {
  digitalWrite(FWD, LOW);
  digitalWrite(REV, LOW);
}


bool push_pull_toggle = 0;

void loop() {
  
  if (digitalRead(BUTTON)) {
    
    if (push_pull_toggle) {
      push();
    }
    else {
      pull();
    }
    delay(300);

    stop();
//    delay(1000);
    
    push_pull_toggle = !push_pull_toggle;
  }

  delay(40);
  
}
