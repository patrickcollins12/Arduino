#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
}

int i=0;
float battery = 100;
void loop() {

    // constantly loop i: 0 .. 99
    i = (++i>99)?0:i;

    // inputs
    int amps = (int)(15.0/100.0*i); // 0 .. ~15
//    int speed = i;
    int speed = (int)(10.0/100.0*i); // 0 .. ~15
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
    display.printf("%0.1fv", i*24.0/100.0);

    // bottom right amps
    display.setFont(&FreeSans9pt7b);
    int x3 = (amps > 9 ) ? SCREEN_WIDTH-28-8 : SCREEN_WIDTH-28;
    int y3 = 61;
    display.setCursor(x3, y3);
    display.printf("%2dA", amps);

    display.display();
    
    delay(50);
}
