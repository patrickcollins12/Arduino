#include <RunningAverage.h>
#include <SPI.h>
#include <Wire.h>
#include <neotimer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

// PIN SETUP
#define RXD2           16 // Needed for Serial2 setup. Why won't you talk to us Syren?
#define TXD2           17 // Transmit out to Syren
#define THROTTLE       15 // Hall Effect Sensor Throttle
#define PP_PIN          4 // Pressure Pad
#define SPD_PIN        25 // Speedo hall effect pin
#define FORWARD_PIN    23 // Direction pin: Forward
#define REVERSE_PIN    19 // Direction pin: Reverse
#define BATT_LEVEL     36 // 24V VOLTAGE DIVIDER INPUT
#define CURRENT_FAULT  3 // 31A Current Fault
#define CURRENT_SENSOR 39 // 31A Current Sensor

// DIRECTION STATES
#define DIR_UNKNOWN -1
#define DIR_STOPPED  0
#define DIR_FORWARD  1
#define DIR_REVERSE  2

// SMOOTHERS
#define THROTTLE_RA_NUM 10
RunningAverage thottleRunningAverage(THROTTLE_RA_NUM);
RunningAverage directionRunningAverage(100);
RunningAverage ppRA(20); // pressure pad
RunningAverage speedoRA(3); // speedo
RunningAverage battRA(20);   // battery reading

/////////////////////////////
// Setup the timers

Neotimer serialWriteTimer        = Neotimer(1000/100); // sample out to motor driver 100/second

Neotimer readThrottleTimer       = Neotimer(1000/100); // read the Throttle 100/second
Neotimer refreshScreenTimer      = Neotimer(1000/10);  // refresh the screen only 10/second
Neotimer readCurrentLevelTimer   = Neotimer(1000/10);  // sample the current only 10/second
Neotimer readBatteryLevelTimer   = Neotimer(1000/10);  // sample the current only 10/second
Neotimer readMotorDirectionTimer = Neotimer(1000/10);  // sample the current only 10/second

// call at the end of setup()
void startTimers() {
  serialWriteTimer.start();
  readThrottleTimer.start();
  refreshScreenTimer.start();
  readCurrentLevelTimer.start();
  readBatteryLevelTimer.start();
  readMotorDirectionTimer.start();
}


/////////////////////////////
// SCREEN SETUP

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


///////////////////////
// MAIN SETUP()

void setup() {
  
  Serial.begin(115200);
  
  // Setup the screen
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  
  // expects a 3 legged pin pulled down onto two separate inputs
  pinMode(FORWARD_PIN,INPUT_PULLDOWN);
  pinMode(REVERSE_PIN,INPUT_PULLDOWN);

  // this is the AH1815 hall effect sensor
  pinMode(SPD_PIN,INPUT_PULLUP);
  
  pinMode(PP_PIN, INPUT_PULLDOWN);
  pinMode(THROTTLE, INPUT_PULLDOWN);

  pinMode(CURRENT_SENSOR, INPUT);

  pinMode(BATT_LEVEL, INPUT);
  
  // setup the smoothers.
  thottleRunningAverage.clear();
  thottleRunningAverage.fillValue(127);
  
  directionRunningAverage.clear();
  ppRA.clear();
  speedoRA.clear();
  battRA.clear();
    
  // Setup RS232 the way the Syren expects but wait 2 seconds first.
  //
  //Until the bauding character is sent, the driver will accept no
  //commands and the green status light will stay lit. Please note that the SyRen 50 may take up to a
  //second to start up after power is applied, depending on the power source being used. Sending the
  //bauding character during this time period may cause undesirable results. When using
  //Packetized Serial mode, please allow a two second delay between applying power and
  //sending the bauding character to the drivers.
  delay(1500);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.write(127); // if we don't set this early and constantly the wheel spins at byte 0 for 2 seconds

  startTimers();

}


//////////////////////////////////////////////////////////////////////////////////////////////////////
// DIRECTION: FWD/OFF/REV
//////////////////////////////////////////////////////////////////////////////////////////////////////

int getMotorDirection() {
  
    int d0 = digitalRead(REVERSE_PIN); //23
    int d1 = digitalRead(FORWARD_PIN); //3

    int direction = DIR_STOPPED;
    
    if ( d0 ) {
        direction = DIR_REVERSE;
    } 
    if ( d1 ) {
        direction = DIR_FORWARD;
    }

    return direction;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////
// SPEED CONTROL
//////////////////////////////////////////////////////////////////////////////////////////////////////


// SPEED CONTROL: Pressure Pad / Finger

// pressure pad remapper
long map_section(long x, long elsey, long in_min, long in_max, long out_min, long out_max) {
  if (x<= in_max && x>= in_min) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  } else {
    return elsey;
  }
}

int getPressurePad() {
  int fsrADC = analogRead(PP_PIN);

  // 2850
  // If the FSR has no pressure, the resistance will be
  // near infinite. So the voltage should be near 0.
//  if (fsrADC != 0) // If the analog reading is non-zero
//  {

    // sensor min 0, sensor max 2900
    int fsrADC2;
    fsrADC2 = constrain(fsrADC, 0, 2900);
    fsrADC2 = map(fsrADC2, 0, 2900, 0, 1000);
    

    // 0-1200 is super sensitive. 0-10%
    // 1200-2500 is quite firm. 10-50%
    // 2500-3600 takes a lots of force. 50-95%
    // 3600-4095 -- 3600 feels like the max so only leave 5% more after that. 95-100%

    // 4950 --> 1000 = 4.95
    int fsrADC3;
    fsrADC3 = map_section( fsrADC2, fsrADC3,    0, 121,    0,   1);
    fsrADC3 = map_section( fsrADC2, fsrADC3,  122, 242,    2, 200);
    fsrADC3 = map_section( fsrADC2, fsrADC3,  243, 505,  201, 400);
    fsrADC3 = map_section( fsrADC2, fsrADC3,  506, 727,  501, 950);
    fsrADC3 = map_section( fsrADC2, fsrADC3,  728,1000,  951,1000);

    ppRA.add(fsrADC3);
    fsrADC3 = ppRA.getAverage();
//    
//    // Use ADC reading to calculate voltage:
//    float fsrV = fsrADC * VCC / 4095.0;
//

//     Serial.printf("pressure sensor: %d remapped and averaged\n", fsrADC3);
    if (fsrADC != 0 )
      Serial.printf("pressure sensor: %4d, %4d clamped, %4d remapped\n", fsrADC, fsrADC2, fsrADC3);

    fsrADC3 = map( fsrADC3, 0,1000, 0,127);
    return fsrADC3;
}


////////////////////////////////
// SPEED CONTROL: Black Throttle - hall effect sensor
int getHallThrottle () {
  const int hallMin=915;
  const int hallMax=3071; 

  // read the throttle hall sensor val
  int thtmp = analogRead(THROTTLE);
//  Serial.println(thtmp);

  // use the average of the last X reads
  thottleRunningAverage.add(thtmp);
  int avg = thottleRunningAverage.getAverage();

  // remap the averaged hall effect sensor
  // to the values needed by the motor driver
  int thcon = constrain(avg, hallMin, hallMax);
  int thby1 = map(thcon, hallMin, hallMax, 0, 127);

  Serial.printf("Throttle level %4d --> rs232 %3d\n", thtmp, thby1);
  return thby1;
}


////////////////////////////////
// SPEED CONTROL: Debug / No input
float thDebug = 0;
// how long do you want it to do a full speed up and slow down cycle? in milliseconds
float debugCycleSpeed = 1000;
// what is the max speed you want it to get to?
float maxDebugSpeed = 30; // max is 127
float debugMS = 2000.0/maxDebugSpeed*2.0;
Neotimer debugSpeedupTimer = Neotimer(debugMS); // increment every 50ms

int getDebugSpeed() { 

  // start the time if it isn't already;
  if (!debugSpeedupTimer.started()) {
    debugSpeedupTimer.start();   
  }

  if (debugSpeedupTimer.repeat()) {
    thDebug++;
  }

  if (thDebug > maxDebugSpeed) {
     thDebug=0;
  }

  float rads = thDebug/maxDebugSpeed*M_PI;
  float sined = sin(rads)*maxDebugSpeed;
//  Serial.printf("ms %d thDebug %f rads %f sined %f actual %d\n", debugMS, thDebug, rads, sined, (int)sined);
  return (int)sined;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// SENSING (RPM / CURRENT / BATTERY)
//////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////
// RPM Monitoring DRAFT
int rpm = 0;
int lastPhaseChangeMillis = 0;
int lastPhaseState = 0;
int wheelTurns = 0;

void getSpeedo() {
  
  int speedValueRaw = digitalRead(SPD_PIN);

  // smooth the sensor value over the last X readings
  speedoRA.add(speedValueRaw);
  int speedValue = speedoRA.getAverage();

  int phaseState = (speedValue)?1:0;
//  Serial.printf("speedValue: %d, magnetPhaseState:%d\n", speedValue,phaseState);

  // on the change from 0 to 1 recalculate rpm
  if (lastPhaseState == 0 && phaseState == 1) {
    wheelTurns++;
    int phaseChangeMillis = millis();
    int phaseDelta = phaseChangeMillis - lastPhaseChangeMillis;
    rpm = (60 * 1000) / phaseDelta;

//    Serial.printf("raw: %4d, smoothed: %4d, phaseState: %4d, rpm: %4d, phaseDeltaMs: %4d\n", 
//                      speedValueRaw, speedValue, phaseState, rpm, phaseDelta );
//    Serial.printf("Loop: 2000\n");
    
    lastPhaseChangeMillis = phaseChangeMillis;
  }
//  else if (lastPhaseState == 1 && phaseState == 0) {
//    Serial.printf("Loop: 1000\n");
//  }

  lastPhaseState = phaseState;
  
}


////////////////////////////////
// get Current Level
// 45 mV/A when Vcc is 3.3 V
int getCurrentLevel() {
  float curr = (float)analogRead(CURRENT_SENSOR);
  float mvolts = (curr/4096)*3.3*1000;
  float current = mvolts/45 - 33;
//  Serial.printf("read %0.2f, mV %0.3f, current %0.1fA\n", curr, mvolts,current-33);
  return (int)current;
}


////////////////////////////////
// get Battery Level
float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float voltLevel = 0;
float battLevel = 100;
void getBatteryLevel() {
  float batt = (float)analogRead(BATT_LEVEL);

  battRA.add(batt);
  batt = battRA.getAverage();

  // v1 6.72k ohm
  // v2 67.4k ohm
  // v1 / (v1+v2) * 24v
  // 6720/(6720+67400) = .0906
  // 24v ==> 2.175v
  // 
  // TGheoretical
  // 0 to 2560 to 4096
  // 0 to 2.06 to 3.3v
  // 0 to 24v  to 38.44v

  // Calibration:
  // 2780 2.14v 17v
  // 3260 2.51v 24v
  float battVo = batt/4096.0*3.3;
  float battVi = mapf(battVo,2.14,2.51, 17.0, 24.0);

  float aa = mapf(battVi,20.0, 24.8, 0.0, 100.0);
  battLevel = (float)constrain((int)aa,0,100);
  voltLevel = battVi;

//  Serial.printf("Battery Level: %0.2f %0.2f %0.2f (actual %0.1f)\n",  batt, battVo, battVi, battLevel);
}




//////////////////////////////////////////////////////////////////////////////////////////////////////
// DISPLAY
//////////////////////////////////////////////////////////////////////////////////////////////////////

int i=0;
//float battery = 100;
void drawScreen(float battery, float voltLevel, int amps) {

    // constantly loop i: 0 .. 99
    i = (++i>99)?0:i;

    // inputs
//    int amps = (int)(15.0/100.0*i); // 0 .. ~15
//    int speed = i;
    int speed = (int)(10.0/100.0*i); // 0 .. ~15
//    battery = 100-i; // 100 .. 0 
    
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.clearDisplay();
      
    int x0 =   0;
    int y0 =  35;

    // main speed
    display.setFont(&FreeMonoBold24pt7b);
    display.setCursor(x0,y0);
    display.printf("%2d",speed);

    // "kph"
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(x0+56,y0);
    display.printf("kph");

    // draw the battery
    display.drawRect(0, 48, 30, 16, SSD1306_WHITE);   // outer
    display.fillRect(30, 52,  3,  8, SSD1306_INVERSE); // + pad terminal
    display.fillRect(2, 50, 26*(battery/100), 12, SSD1306_INVERSE); // inside meter

    // bottom right volts
    display.setFont(&FreeSans9pt7b);
    int x2 = 37;
    int y2 = 61;
    display.setCursor(x2,y2);
    display.printf("%0.1fv", voltLevel);

    // bottom right amps
    display.setFont(&FreeSans9pt7b);
    int x3 = (amps > 9 ) ? SCREEN_WIDTH-28-8 : SCREEN_WIDTH-28;
    int y3 = 61;
    display.setCursor(x3, y3);
    display.printf("%2dA", amps);

    display.display();
    
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
// DEBUG DETERMINE FPS (Hz)
//////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned long lastLoopMicros=0;
int loopsPerSample=100; // start off with a low number, then we'll adjust it
int loopCnt=loopsPerSample;
int samplesPerSecond=1;  // how many times we want to actually read the fps
float hz;
void readHz() {
  
//    float hz;
    
    if (--loopCnt == 0) {
      unsigned long nowMicros = micros();
      unsigned long elapsedMicros = nowMicros - lastLoopMicros;
      hz = loopsPerSample/(float)elapsedMicros*1000000.0;
      
//      if (hz > 1000000) {
//         Serial.printf("elapsed per loop: %lums, %0.0fhz, %0.1fMHz\n",elapsedMicros,hz, hz/1000000.0);
//      }
//      else if (hz > 1000) {
//         Serial.printf("elapsed per loop: %lums, %0.0fhz, %0.1fKHz\n",elapsedMicros,hz,hz/1000.0);  
//      }
//      else {
//         Serial.printf("elapsed per loop: %lums, %0.0fHz\n",elapsedMicros,hz);
//      }    

      // reset
      
      // set the loops per sample to every x times per second
      loopsPerSample = hz / samplesPerSecond;
      lastLoopMicros = nowMicros;
      loopCnt=loopsPerSample;
    } 
}



//////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP
//////////////////////////////////////////////////////////////////////////////////////////////////////


int lastDirection=DIR_UNKNOWN;
int current=0;
char lastSPrintf[1000];
char newSPrintf[1000];
int thtmp=127;
int direction=DIR_STOPPED;
//char *dirStr;
char dirStr[20];

void loop() {

  // DEBUG check how fast the script is running
  readHz();

  // READ DIRECTION

  bool debug=false;
  
  if (readMotorDirectionTimer.repeat()) {
    
    if (debug){
      direction = DIR_FORWARD; // DEBUG FORCE FORWARD
    } else {
      direction = getMotorDirection();
    }
     
     if (direction == DIR_FORWARD) { strcpy(dirStr,"FORWARD"); }
     if (direction == DIR_REVERSE) { strcpy(dirStr,"REVERSE");}
     if (direction == DIR_STOPPED) { strcpy(dirStr,"STOPPED"); }
     if (direction == DIR_UNKNOWN) { strcpy(dirStr,"UNKNOWN"); }
  }

  // READ THROTTLE
  if (readThrottleTimer.repeat()) {
    if (debug) {
      thtmp = getDebugSpeed();
    } else{
      thtmp = getHallThrottle();
    }
     //  thtmp = getPressurePad();
  }
  
//  getSpeedo();
    
  // GO! Drive! Set rs232
  if (serialWriteTimer.repeat()) {
    // Setup the rs232 command
    //           stop    full speed
    // forward   127 --> 255
    // backward  127 --> 0
    byte serialVal=127;
    const float speedLimiter = 1; // 1=100%, .8 = 80%
  
    // Forward is 127-255
    if (direction == DIR_FORWARD) {
      serialVal = (thtmp * speedLimiter) + 127;
    } 
  
    // Reverse is 127 to 0
    else if (direction == DIR_REVERSE) {
      serialVal = 127-(thtmp * speedLimiter);
    }

    // debug print the throttle level
    Serial.printf("OOF %s %d (Serial:%d)\n",dirStr, thtmp, serialVal);  
    Serial2.write(serialVal);  
  }

    

  if (readBatteryLevelTimer.repeat()) {
    getBatteryLevel();
  }
  
  if (readCurrentLevelTimer.repeat()) {
     current = getCurrentLevel();
  }

  if (refreshScreenTimer.repeat()) {
    drawScreen(battLevel, voltLevel, current);
  }

   
  sprintf(newSPrintf, "loop hertz: %0.0fHz, rpm: %d, %0.1f kph\n",hz,rpm, rpm*1000.0*0.0001885);
  if (strcmp(newSPrintf,lastSPrintf) != 0) {
     Serial.printf("%s",newSPrintf);
  }
  strcpy(lastSPrintf, newSPrintf);
  
//  delay(40);
}
