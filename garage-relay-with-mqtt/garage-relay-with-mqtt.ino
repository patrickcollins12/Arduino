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
MQTTClient mqtt_client;

Neotimer hallReadTimer = Neotimer(1000/10);      // measure hall timer every 10th second
//Neotimer connectedStatusTimer = Neotimer(1000*60); // ping every minute



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


char mqtt_client_name[300];
void setupMqttClientName() {
    // Get the MAC Address of the Wifi and make a unique mqtt client.
    // mqtt broker can only have one client connected at the same time with the same name.
    String mqtt = String("esp32-") + getMacAddress();
    mqtt.toCharArray(mqtt_client_name,300);
}


bool reconnectedWifi=false;
void connectWifi() {

    Serial.printf("connecting wifi %s ...", ssid);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
    }
    Serial.print(" done\n");
    reconnectedWifi=true;

}

void connectMQTT() {
  
  if (!mqtt_client.connected() ) {
    
    Serial.printf("connecting to mqtt://%s...", mqtt_host);

    while (!mqtt_client.connect(mqtt_client_name, mqtt_username, mqtt_password)) {
      Serial.print(".");
      delay(100);
    }
    Serial.print(" done\n");
  
    // mpub -t "homeassistant/cover/garagedoor/config" -m '{ 

//    const char *configr = R""""( { "name": "Garage Door", "~": "homeassistant/cover/garagedoor", "device_class":"garage", "state_topic":"~/state", "command_topic":"/GD/Command", "availability_topic": "~/available", "payload_available":"online", "payload_not_available":"offline", "qos":1, "retain":false, "optimistic": false, "payload_open": "open","payload_close": "close", "payload_stop": "click", "state_open": "open", "state_opening": "opening", "state_closed": "closed", "state_closing": "closing", "unique_id":"esp32_1234" } )"""";
//    const char *configr = R""""( { "name": "Garage Door", "~": "homeassistant/cover/garagedoor" } )"""";
//    const char *configr = R""""( abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz )"""";
    String configr = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";

    Serial.print(configr);
    // make the broker send and offline message retainedwhen i go offline
//    mqtt_client.setWill(MQTT_PREFIX"/available", "offline", true, 1);
//    mqtt_client.publish(MQTT_PREFIX"/available", "online",true,0);
    if (!mqtt_client.publish(MQTT_PREFIX"/config", configr, true, 0)) {
      lwmqtt_err_t err = mqtt_client.lastError();
      Serial.printf("Failed config publish: %d",err);
    }
    mqtt_client.subscribe(mqtt_topic);
  }
  
}

void pressGarageDoorRelay() {
//    Serial.print("Garage Door Relay Activated");
//    mqtt_client.publish(MQTT_PREFIX"/log", "Relay activated" );

    ledcWrite(R_PWM_CH, 0);  
    delay(1000);
    ledcWrite(R_PWM_CH, 255);    
    Serial.println("... done ");

}

// read the AH1815 hall effect sensor 
// 0 is when magnet is close. That is when door is closed.
// 1 is when no magnet is present. That is when door is undefined.
// implement: open, opening, closed or closing
int isGarageOpen() {
   // CLSD  OPEN  CODE STATE
   // 0     0     -1    Error. Both magnets cannot be activated concurrently or a sensor is offline
   // 1     0     1    Door open
   // 0     1     0    Door closed
   // 1     1     2   Door is moving or stuck

   int hclsd = digitalRead (HALL_SENSOR_CLSD);
   int hopen = digitalRead (HALL_SENSOR_OPEN);
//   Serial.printf("CLOSED: %d, OPEN: %d\n",hclsd, hopen);
   
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

// implement: open, opening, closed or closing
int lastStatus=-0;
void reportDoorStatus () {
    int status = isGarageOpen();
    if (status != lastStatus) {
      char *statusstr;
      if (status ==  0 ) statusstr = "closed";
      if (status ==  1 ) statusstr = "open";
      if (status ==  2 ) {
        if (lastStatus = 0) {
           statusstr = "opening";  
        }
        if (lastStatus = 1) {
          statusstr = "closing";  
        }
      }
      if (status == -1 ) {
          statusstr = "error";
          mqtt_client.publish(MQTT_PREFIX"/log", "Error: sensor offline or both activated" );
      }
      mqtt_client.publish(MQTT_PREFIX"/state", statusstr, true, 1);
    }
    lastStatus = status;
}

// receive an open or close command and act accordingly
void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  if ( payload == "open" ) {
    if (isGarageOpen() ) {
       mqtt_client.publish(MQTT_PREFIX"/log", "Open requested, but garage is already open. Aborting" );

    } else {
      mqtt_client.publish(MQTT_PREFIX"/log", "Opening door" );
      mqtt_client.publish(MQTT_PREFIX"/state", "opening", true, 1);

      pressGarageDoorRelay();
    }
  }

  else if ( payload == "close" ) {
    if (isGarageOpen() ) {
       mqtt_client.publish(MQTT_PREFIX"/log", "Closing door" );
       mqtt_client.publish(MQTT_PREFIX"/state", "closing", true, 1);
       pressGarageDoorRelay();
    } else {
      // garage is already closed
      mqtt_client.publish(MQTT_PREFIX"/log", "Close requested, but garage is already closed. Aborting" );
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
  mqtt_client.begin(mqtt_host, net);
  mqtt_client.onMessage(messageReceived);

  // calls our utility connect function
  connectWifi();
  connectMQTT();

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

  setupMqttClientName();

}


//////////////////////////
// MAIN LOOP
void loop() {
  
  mqtt_client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }
  
  if (!mqtt_client.connected()) {
    Serial.println("Not connected, trying again");
    connectMQTT();
  }

  if ( reconnectedWifi && mqtt_client.connected() ) {
     publishWifiStrength();
     reconnectedWifi=false;
  }
  
  if (!digitalRead(BUTTON)) {
    Serial.println("Button pressed\n");    
    mqtt_client.publish(MQTT_PREFIX"/log", "button pressed");
    pressGarageDoorRelay();
  }

  if (hallReadTimer.repeat()) {
    reportDoorStatus();
  }

//  if (connectedStatusTimer.repeat()) {
//     mqtt_client.publish(MQTT_PREFIX"/log", "online");
//  }
}


// UTILITY


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
  mqtt_client.publish(MQTT_PREFIX"/log", buf );
  
}
