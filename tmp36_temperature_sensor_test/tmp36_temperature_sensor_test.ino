//TMP36 Pin Variables
int sensorPin = 4; //the analog pin the TMP36's Vout (sense) pin is connected to
                        //the resolution is 10 mV / degree centigrade with a
                        //500 mV offset to allow for negative temperatures

#include <RunningAverage.h>

RunningAverage tempRA(50);
RunningAverage tempDropRA(50);

void setup()
{
  Serial.begin(115200);
  pinMode(sensorPin,INPUT_PULLDOWN);
  Serial.printf("temperature\n");

  tempRA.clear();
  tempDropRA.clear();

}

void loop()
{
  
  //getting the voltage reading from the temperature sensor
  int reading = analogRead(sensorPin);
//  delay(3);
//  reading = analogRead(sensorPin);

  // converting that reading to voltage, for 3.3v arduino use 3.3
  float voltage = reading * 3.3 / 4096.0;
  
  float rawC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree with 500 mV offset
                                         //to degrees ((voltage - 500mV) times 100)
  
  float temperatureC = round(rawC*4.0) / 4.0;

  tempRA.add(temperatureC);
  float tempSmooth = tempRA.getAverage();
  float roundedSmooth = round(tempSmooth*4.0) / 4.0;
  
  Serial.printf("degrees: %0.0f\n", roundedSmooth);
  
  delay(20);

}
