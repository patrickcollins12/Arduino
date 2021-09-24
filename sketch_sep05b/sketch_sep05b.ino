#include <neotimer.h>
#include <ArduinoOTA.h>
#include <RunningAverage.h>

#define SPD_PIN        25 // Speedo hall effect pin
RunningAverage speedoRA(5); // speedo`

Neotimer mytimer = Neotimer(1000); // 1 second timer


void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(115200);
  // this is the AH1815 hall effect sensor
  pinMode(SPD_PIN,INPUT_PULLUP);

  mytimer.set(100);

}


////////////////////////////////
// RPM Monitoring DRAFT
int rpm = 0;
int lastPhaseChangeMillis = 0;
int lastPhaseState = 0;
int wheelTurns = 0;

int getSpeedo() {
  
  int speedValue = analogRead(SPD_PIN);

  // smooth the sensor value over the last X readings
//  speedoRA.add(speedValue);
//  speedValue = speedoRA.getAverage();

  int phaseState = (speedValue)?1:0;
//  Serial.printf("speedValue:%d, phaseState:%d\n",speedValue, phaseState*400);

  // on the change from 0 to 1 recalculate rpm
  if (lastPhaseState == 0 && phaseState == 1) {
    wheelTurns++;
    int phaseChangeMillis = millis();
    int phaseDelta = phaseChangeMillis - lastPhaseChangeMillis;
    rpm = (60 * 1000) / phaseDelta;

//    Serial.printf("Hall Effect: %4d phaseState  %4d rpm  %4d wheelturns %4d MicrosElapsed %4d hallValue\n", phaseState*400, rpm, wheelTurns, phaseDelta, speedValue);
    
    lastPhaseChangeMillis = phaseChangeMillis;
  }

  lastPhaseState = phaseState;
  
  return rpm;
}

unsigned long lastLoopMicros=0;
int loopsPerSample=10000; // start off with a low number, then we'll adjust it
int loopCnt=loopsPerSample;
int samplesPerSecond=10;  // how many times we want to actually read the fps
void readHz() {
  
    if (--loopCnt == 0){
      unsigned long nowMicros = micros();
      unsigned long elapsedMicros = nowMicros - lastLoopMicros;
      float hz = loopsPerSample/(float)elapsedMicros*1000000.0;
      
      if (hz > 1000000) {
         Serial.printf("elapsed: %lu, %0.0fhz, %0.1fMHz\n",elapsedMicros,hz, hz/1000000.0);
      }
      else if (hz > 1000) {
         Serial.printf("elapsed: %lu, %0.0fhz, %0.1fKHz\n",elapsedMicros,hz,hz/1000.0);  
      }
      else {
         Serial.printf("elapsed: %lu, %0.0fHz\n",elapsedMicros,hz);
      }    
      
      // reset
      
      // set the loops per sample to every x times per second
      loopsPerSample = hz / samplesPerSecond;
      lastLoopMicros = nowMicros;
      loopCnt=loopsPerSample;
    } 
}

void loop() {
  // put your main code here, to run repeatedly:
//  int y = getSpeedo();
  if(mytimer.repeat()){
    Serial.println("Calling this indefinitely every 2 seconds!");
//    digitalWrite(D13,!digitalRead(D13)); // Let's blink each two seconds
  }


//  readHz();
}
