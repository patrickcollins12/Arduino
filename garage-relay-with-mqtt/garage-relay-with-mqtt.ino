#include <WiFi.h>
#include <MQTT.h>
#include <neotimer.h>
#include "config.h"

#define R_PWM 23 // Relay
#define R_PWM_CH 0
#define PWM_Res   8
#define PWM_Freq  10000

// AH1815 Non latching Hall Effect Sensors
#define HALL_SENSOR_OPEN 33 
#define HALL_SENSOR_CLSD 32

// Onboard reset button
#define BUTTON 22

WiFiClient net;
MQTTClient client;

Neotimer hallReadTimer = Neotimer(1000/10);      // measure hall timer every 10th second
//Neotimer connectedStatusTimer = Neotimer(1000*60); // ping every minute

void connect() {
  
  Serial.printf("connecting wifi %s ...", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.print(" done\n");
  
  Serial.printf("connecting to mqtt://%s...", mqtt_host);

  // Get the MAC Address of the Wifi and make a unique mqtt client.
  // mqtt broker can only have one client connected at the same time with the same name.
  String mqtt = String("esp32-") + getMacAddress();
  char mqtt_client_name[300];
  mqtt.toCharArray(mqtt_client_name,300);


  // make the broker send and offline message retained, when i go offline
  client.setWill("/GD/Status", "offline", true, 1);

  while (!client.connect(mqtt_client_name, mqtt_username, mqtt_password)) {
    Serial.print(".");
    delay(100);
  }

  Serial.print(" done\n");

  client.subscribe(mqtt_topic);

  client.publish("/GD/Status", "online");
  
  publishWifiStrength();
}

void pressGarageDoorRelay() {
    Serial.print("Garage Door Relay Activated");
    client.publish("/GD/Log", "Relay activated" );

    ledcWrite(R_PWM_CH, 0);  
    delay(1000);
    ledcWrite(R_PWM_CH, 255);    
    Serial.println("... done ");

}

// read the AH1815 hall effect sensor 
// 0 is when magnet is close. That is when door is closed.
// 1 is when no magnet is present. That is when door is undefined.
int isGarageOpen() {
   // CLSD  OPEN  CODE STATE
   // 0     0     -1    Error. Both magnets cannot be activated concurrently or a sensor is offline
   // 1     0     1    Door open
   // 0     1     0    Door closed
   // 1     1     2   Door is moving or stuck

   int hclsd = digitalRead (HALL_SENSOR_CLSD);
   int hopen = digitalRead (HALL_SENSOR_OPEN);
   // 0 is activated
   if ( !hclsd && !hopen) {
       return -1;
   }

   else if ( hclsd && !hopen) {
       return 1;
   }

   else if ( !hclsd && hopen) {
       return 0;
   }
   else if ( hclsd && hopen) {
       return 2; 
   }
   else {
     // panic: food fight!
   }
}

int lastStatus=-0;
void reportDoorStatus () {
    int status = isGarageOpen();
    if (status != lastStatus) {
      char *statusstr;
      if (status ==  0 ) statusstr = "closed";
      if (status ==  1 ) statusstr = "open";
      if (status ==  2 ) statusstr = "moving or stuck";
      if (status == -1 ) {
          statusstr = "error";
          client.publish("/GD/Log", "Error: sensor offline or both activated" );
      }
      client.publish("/GD/DoorStatus", statusstr, true, 1);
    }
    lastStatus = status;
}

// receive an open or close command and act accordingly
void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  if ( payload == "open" ) {
    if (isGarageOpen() ) {
       client.publish("/GD/Log", "Open requested, but garage is already open. Aborting" );

    } else {
      client.publish("/GD/Log", "Closing door" );
      pressGarageDoorRelay();
    }
  }

  else if ( payload == "close" ) {
    if (isGarageOpen() ) {
       client.publish("/GD/Log", "Opening door" );
       pressGarageDoorRelay();
    } else {
      // garage is already closed
      client.publish("/GD/Log", "Close requested, but garage is already closed. Aborting" );
    }
  }

  else if ( payload == "click" ) {
    pressGarageDoorRelay();  
  }  
  Serial.println("");
}

int last_hall_state=0;
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin(mqtt_host, net);
  client.onMessage(messageReceived);

  // calls our utility connect function
  connect();

  // Setup the relay activator
  ledcSetup(R_PWM_CH, PWM_Freq, PWM_Res);
  ledcAttachPin(R_PWM, R_PWM_CH);
  ledcWrite(R_PWM_CH, 255); 

  // setup the button
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(HALL_SENSOR_CLSD,INPUT_PULLUP);
  pinMode(HALL_SENSOR_OPEN,INPUT_PULLUP);

  hallReadTimer.start();
//  connectedStatusTimer.start();


}


//////////////////////////
// MAIN LOOP
void loop() {
  
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    Serial.println("Not connected, trying again");
    connect();
  }

  if (!digitalRead(BUTTON)) {
    Serial.println("Button pressed\n");    
    client.publish("/GD/Log", "button pressed");
    pressGarageDoorRelay();
  }

  if (hallReadTimer.repeat()) {
    reportDoorStatus();
  }

//  if (connectedStatusTimer.repeat()) {
//     client.publish("/GD/Log", "online");
//  }
}


// UTILITY

String getMacAddress()
{
   uint8_t baseMac[6];

   // Get MAC address for WiFi station
   esp_read_mac(baseMac, ESP_MAC_WIFI_STA);

   char baseMacChr[18] = {0};
   sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);

   String macAddress = String(baseMacChr);

   return String(baseMacChr);
}


void publishWifiStrength() {
  
  // PUBLISH THE WIFI STRENGTH
  //RSSI level and a signal strength
  //-50 dBm  Excellent
  //-60 dBm Very good
  //-70 dBm Good
  //-80 dBm Low
  //-90 dBm Very low 
  long signalStrength = WiFi.RSSI();
  char *s;
       if (signalStrength <  -80) s = "Very low";
  else if (signalStrength <  -70) s = "Low";
  else if (signalStrength <  -60) s = "Good";
  else if (signalStrength <  -50) s = "Very good";
  else if (signalStrength >= -50) s = "Excellent";
  int percent = map((int)signalStrength, -90, -42, 0, 100);
  percent = constrain(percent, 0, 100);
  
  char buf[300];
  sprintf(buf,"%s is %s (%d%% %ld dBm)", ssid, s, percent, signalStrength);
  client.publish("/GD/Log", buf );
  
}
