#include <RunningAverage.h>

const int FSR_PIN = 2; // Pin connected to FSR/resistor divider
//const float VCC = 3.22; // Measured voltage of Ardunio 5V line
//const float R_DIV = 3000.0; // Measured resistance of 3.3k resistor

RunningAverage ppRA(20);

void setup() 
{
  Serial.begin(115200);
  pinMode(FSR_PIN, INPUT_PULLDOWN);
  ppRA.clear();
}

long map_section(long x, long elsey, long in_min, long in_max, long out_min, long out_max) {
  if (x<= in_max && x>= in_min) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  } else {
    return elsey;
  }
}

void loop() 
{
  int fsrADC = analogRead(FSR_PIN);

  // If the FSR has no pressure, the resistance will be
  // near infinite. So the voltage should be near 0.
  if (fsrADC != 0) // If the analog reading is non-zero
  {
    int fsrADC2 = map(fsrADC, 600, 4095, 0, 4095);
    fsrADC2 = fsrADC; //constrain(fsrADC2, 0, 4095);

    // 0-1200 is super sensitive. 0-10%
    // 1200-2500 is quite firm. 10-50%
    // 2500-3600 takes a lots of force. 50-95%
    // 3600-4095 -- 3600 feels like the max so only leave 5% more after that. 95-100%
    int fsrADC3;
    fsrADC3 = map_section( fsrADC2, fsrADC3,    0, 600,    0,   1);
    fsrADC3 = map_section( fsrADC2, fsrADC3,  601,1200,    2, 200);
    fsrADC3 = map_section( fsrADC2, fsrADC3, 1201,2500,  201, 400);
    fsrADC3 = map_section( fsrADC2, fsrADC3, 2501,3600,  501, 950);
    fsrADC3 = map_section( fsrADC2, fsrADC3, 3601,4095,  951,1000);

    ppRA.add(fsrADC3);
    fsrADC3 = ppRA.getAverage();
//    
//    // Use ADC reading to calculate voltage:
//    float fsrV = fsrADC * VCC / 4095.0;
//

      Serial.printf("pressure sensor: %d remapped and averaged\n", fsrADC3);
//    Serial.printf("pressure sensor: %d, %d clamped, %d remapped (%0.2f volts)\n", fsrADC, fsrADC2, fsrADC3, fsrV);

    delay(20);
    
  }

}
