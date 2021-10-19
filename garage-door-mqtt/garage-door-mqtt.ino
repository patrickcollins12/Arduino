#include <WiFi.h>
#include "EspMQTTClient.h"
#include "config.h"
#include <neotimer.h>

#define R_PWM 23 // Relay
#define R_PWM_CH 0
#define PWM_Res   8
#define PWM_Freq  10000

// AH1815 Non latching Hall Effect Sensors
#define HALL_SENSOR_OPEN 33 
#define HALL_SENSOR_CLSD 32

Neotimer hallReadTimer = Neotimer(1000/10);      // measure hall timer every 10th second

EspMQTTClient client(
  ssid,
  wifipass,
  mqtt_host,       // MQTT Broker server ip
  mqtt_username,   
  mqtt_password,   
  "esp32-garage-door",          // Client name that uniquely identify your device
  mqtt_port        // The MQTT port, default to 1883. this line can be omitted
);

void setup()
{
  Serial.begin(115200);

  // Optionnal functionnalities of EspMQTTClient : 
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
  client.enableLastWillMessage(MQTT_PREFIX"/available", "offline", true);
  client.enableMQTTPersistence();
  client.setMaxPacketSize(1024);
  
  // Setup the relay activator
  ledcSetup(R_PWM_CH, PWM_Freq, PWM_Res);
  ledcAttachPin(R_PWM, R_PWM_CH);
  ledcWrite(R_PWM_CH, 255); 

  // setup the sensors
  pinMode(HALL_SENSOR_CLSD,INPUT_PULLUP);
  pinMode(HALL_SENSOR_OPEN,INPUT_PULLUP);

}

// This function is called once everything is connected (Wifi and MQTT)
void onConnectionEstablished()
{
  
//  publishWifiStrength();

  // Subscribe to actions
  client.subscribe(MQTT_PREFIX"/action", [](const String & payload) {
    if ( payload == "open" )  openDoor();
    if ( payload == "close" ) closeDoor();
    if ( payload == "click" ) clickDoor();
  });

  // I am now online. (note that the last will message is what puts it offline)
  client.publish(MQTT_PREFIX"/available", "online", true);

  // publish the config
  const char *configr = R""""( 
    { 
       "name": "Garage Door", 
       "~": "homeassistant/cover/garagedoor", 
       "device_class":"garage", 
       "unique_id":"esp32_1234",
       
       "state_topic":"~/state", 
       "command_topic":"~/action",
       "availability_topic": "~/available", 

       "qos":1, 
       "retain":false, 
       "optimistic": false,
       
       "payload_available":"online", 
       "payload_not_available":"offline", 
       
       "payload_open": "open",
       "payload_close": "close", 
       "payload_stop": "click", 
       
       "state_open": "open", 
       "state_opening": "opening", 
       "state_closed": "closed", 
       "state_closing": "closing"
  
  } )"""";
  
  client.publish(MQTT_PREFIX"/config", configr, true);

  // once connections are established we can start reading sensors
  hallReadTimer.start();
}


void pressGarageDoorRelay() {
//    Serial.print("Garage Door Relay Activated");
//    mqtt_client.publish(MQTT_PREFIX"/log", "Relay activated" );

    ledcWrite(R_PWM_CH, 0);  
    delay(1000);
    ledcWrite(R_PWM_CH, 255);    
//    Serial.println("... done ");

}

// open, closed, opening, closing, unknown, error
#define GD_OPEN 1
#define GD_CLOSED 2
#define GD_CLOSING 3
#define GD_OPENING 4
#define GD_UNKNOWN 5
#define GD_ERROR 6

char *getStatusString(int st) {
  if (st == GD_OPEN)    return "open";
  if (st == GD_CLOSED)  return "closed"; 
  if (st == GD_OPENING) return "opening"; 
  if (st == GD_CLOSING) return "closing"; 
  if (st == GD_UNKNOWN) return "unknown"; 
  if (st == GD_ERROR)   return "error"; 
}

int status = GD_UNKNOWN;
int lastStatus = GD_UNKNOWN;

void openDoor() {
  if ( status == GD_OPEN ) {
    // no op
    client.publish(MQTT_PREFIX"/log", "Open requested, but garage is already open. Aborting" );  
  } 
  else {
    pressGarageDoorRelay();
    status = GD_OPENING;

    //  in 20 seconds and if status is still opening, then set status=unknown 
    client.executeDelayed(20 * 1000, []() {
        if (status == GD_OPENING) {
          status = GD_UNKNOWN;
          client.publish(MQTT_PREFIX"/log", "Open 20 second timeout. Status unknown");          
        }
    });
  }
}


void closeDoor() {
  if ( status == GD_CLOSED ) {
    // no op
    client.publish(MQTT_PREFIX"/log", "Close requested, but garage is already closed. Aborting" );  
  } 
  else {
    pressGarageDoorRelay();
    status = GD_CLOSING;

    //  in 20 seconds and if status is still opening, then set status=unknown 
    client.executeDelayed(20 * 1000, []() {
        if (status == GD_CLOSING) {
          status = GD_UNKNOWN;
          client.publish(MQTT_PREFIX"/log", "Close 20 second timeout. Status unknown");          
        }
    });
  }
}

void clickDoor() {
  if ( status == GD_OPEN )   closeDoor();
  if ( status == GD_CLOSED ) openDoor();
  else {
    pressGarageDoorRelay();
    status == GD_UNKNOWN;
  }
}


void publishDoorStatusChanges() {
  int sensorStatus = readSensorStatus();

  // set the status based on sensor status
  // an unknown sensor status is ignored when garage is opening or closing
  if ( sensorStatus == GD_UNKNOWN && (status == GD_OPENING || status == GD_CLOSING) ) {
     // no op
  }
  else {
    if (sensorStatus != status ) {
       status = sensorStatus;
    }
  }

  // publish only changed status
  if (status != lastStatus) {
     client.publish(MQTT_PREFIX"/state", getStatusString(status), true);
  }
  lastStatus = status;

}

// read the AH1815 hall effect sensor 
// 0 is with magnet close, 1 is when no magnet is close.
int readSensorStatus() {
   // CLSD  OPEN  CODE STATE
   // 1     1      2    Door is moving, stopped or no sensors
   // 1     0      1    Door open
   // 0     1      0    Door closed
   // 0     0     -1    Error. Both magnets cannot be activated concurrently or a sensor is offline

   int hclsd = digitalRead (HALL_SENSOR_CLSD);
   int hopen = digitalRead (HALL_SENSOR_OPEN);

   if (  hclsd &&  hopen) { return  GD_UNKNOWN; }
   if ( !hclsd && !hopen) { return  GD_ERROR;   }
   if (  hclsd && !hopen) { return  GD_OPEN;    }
   if ( !hclsd &&  hopen) { return  GD_CLOSED;  }
}



void loop()
{
  client.loop();
  
  if (hallReadTimer.repeat() && client.isConnected()) {
    publishDoorStatusChanges();
  }
}
