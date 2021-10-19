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
#define SOLENOID_PUSH           19 // comment
#define SOLENOID_PULL           18 // comment
//
//IO5 - brown - H1A
//IO19 - black - H1B
//IO33 - orange - H1C
//IO18 - blue - H1D
#define H1A 5
#define H1B 19
#define H1C 33
#define H1D 18

// SMOOTHERS
//RunningAverage battRA(20);   // battery reading

/////////////////////////////
// Setup the timers
Neotimer refreshScreenTimer = Neotimer(1000/10);  // refresh the screen only 10/second

// call at the end of setup()
void startTimers() {
  refreshScreenTimer.start();
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

  
  // pin work
//    pinMode(SOLENOID_PUSH,OUTPUT);
//    pinMode(SOLENOID_PULL,OUTPUT);

//  #define H1A 5
//#define H1B 19
//#define H1C 33
//#define H1D 18
//
    pinMode(H1A,OUTPUT);
    pinMode(H1B,OUTPUT);
    pinMode(H1C,OUTPUT);
    pinMode(H1D,OUTPUT);

  // setup the smoothers.
  //  battRA.clear();

  startTimers();
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
    amps = (int)(15.0/100.0*i); // 0 .. ~15
    int speed = i;
//    int speed = (int)(10.0/100.0*i); // 0 .. ~15
    battery = 100-i; // 100 .. 0 
    
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
int loopsPerSample=100; // seed number. start off with a low number, then it gets adjusted
int loopCnt=loopsPerSample;
int samplesPerSecond=1;  // how many times we want to actually read the fps
float hz; // where we store the global result
void readHz() {
    
    if (--loopCnt == 0) {
      unsigned long nowMicros = micros();
      unsigned long elapsedMicros = nowMicros - lastLoopMicros;
      hz = loopsPerSample/(float)elapsedMicros*1000000.0;
      
      // set the loops per sample to every x times per second
      loopsPerSample = hz / samplesPerSecond;
      lastLoopMicros = nowMicros;

      // reset the loop counter
      loopCnt=loopsPerSample;

      // Print the results
      if      (hz > 1000000)   Serial.printf("hertz: %0.1f MHz\n",hz/1000000.0);  
      else if (hz > 1000)      Serial.printf("hertz: %0.1f KHz\n",hz/1000.0);
      else                     Serial.printf("hertz: %0.1f Hz\n", hz);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN LOOP
//////////////////////////////////////////////////////////////////////////////////////////////////////

void push() {
  Serial.println("push");
  digitalWrite(H1A,HIGH);  
  digitalWrite(H1B,HIGH);  
  digitalWrite(H1C,LOW);  
  digitalWrite(H1D,LOW);  
}

void pull() {
  Serial.println("pull");
  digitalWrite(H1A,LOW);  
  digitalWrite(H1B,LOW);  
  digitalWrite(H1C,HIGH);  
  digitalWrite(H1D,HIGH);  
}

void stop() {
  Serial.println("stop");
  digitalWrite(H1A,LOW);  
  digitalWrite(H1B,LOW);  
  digitalWrite(H1C,LOW);  
  digitalWrite(H1D,LOW);  
}

void loop() {

  // DEBUG check how fast the script is running
  readHz();

  push();
  delay(1000);

  stop();
  delay(2000);

  pull();
  delay(1000);

  stop();
  delay(2000);

//  Serial.println("Push");
//  digitalWrite(SOLENOID_PUSH,HIGH);  
//  digitalWrite(SOLENOID_PULL,LOW);
//  delay(1000);
//  
//  Serial.println("Off");
//  digitalWrite(SOLENOID_PUSH,LOW);  
//  digitalWrite(SOLENOID_PULL,LOW);
//  delay(200);
//
//  Serial.println("Pull");  
//  digitalWrite(SOLENOID_PUSH,LOW);  
//  digitalWrite(SOLENOID_PULL,HIGH);
//  delay(1000);
//  
//  Serial.println("Off");
//  digitalWrite(SOLENOID_PUSH,LOW);  
//  digitalWrite(SOLENOID_PULL,LOW);
//  delay(200);
  
  if (refreshScreenTimer.repeat()) {
//    drawScreen(battLevel, voltLevel, current);
//    drawScreen(0,0,0);
  }

//  delay(40);
}
