// -----------------------------------------------------------------------------
//
// This code runs on the 8266 and is used to communicate to an MQTT broker. It is
// an mashup of various 8266-MQTT client routines found on the web.
//
//  This code provide the communications between the TM4C and a Web Application 
//  that controls the Lab 4 clock
//
// Author:    Mark McDermott
// Rev 6:     7/22/23
//
// ----------------------------------------------------------------
// ----------------    DEFINES   ----------------------------------
// 
// 
#define     DEBUG1                  // First level of Debug
//#define     DEBUG2                  // Second level of Debug
//#define     DEBUG3                  // Third level of Debug

#define       RDY 2

// ----------------------------------------------------------------
// ---------------   INCLUDES    ----------------------------------
//
#include <ESP8266WiFi.h>            // WiFi drivers
#include <PubSubClient.h>           // MQTT drivers
#include <BlynkSimpleEsp8266.h>     // Blynk timer -- best timer for this appllication

#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------
// ----------------  VARIABLES    ---------------------------------         
//
char  eid[20]           = "rb679";
char  ssid[64]          = "EE-IOT-Platform-03";           
char  password[64]      = "g!TyA>hR2JTy"; 


char  mode[2]           = "";
char  hour[3]           = "";
char  minute[3]         = "";
char  second[3]         = "";

char  cmd[20];                                       
char  ser_buf[128];            




// ----------------------------------------------------------------
// -------------     UT Server MQTT Broker    ---------------------
//
const char *mqtt_username       = "10.159.177.113";                   // Not needed for this appication 
const char *mqtt_password       = "";
char        mqtt_broker[20]     = "";
char        port[5]             = "";
int         mqtt_port;

// ----------------------------------------------------------------
// --------------     Publish topics     --------------------------
//
const char  *pub_mode           = "/b2w/mode"; 
const char  *pub_hour           = "/b2w/hour";  
const char  *pub_min            = "/b2w/min"; 
const char  *pub_sec            = "/b2w/sec";
const char  *pub_msg            = "/b2w/msg";           // Debug only 
// ----------------------------------------------------------------
// --------------     Subscribe topics     ------------------------
//
char  topic_w2b[20]             = "/w2b";                   
char  topic_subscribe[64];
char  topic_publish[64];


// ----------------------------------------------------------------
//  -------     Start services     --------------------------------
// ----------------------------------------------------------------

WiFiClient          espClient;
PubSubClient client(espClient);
BlynkTimer          timer;                // We will use the Blynk timer for scheduling serial port reads

// ----------------------------------------------------------------------------
// This routine sets up Wifi. First step is receive the SSID, Password and
// student EID code using CSV format. Second step is to parse it and try
// to connect to the WiFi hotspot. Once the WiFI connection is established
// we then connect to the MQTT broker
//

void Setup_Wifi(void) {

  char ser_buf[1024];
  static int bufpos = 0;                // starts the buffer back at the first position in the incoming serial.read

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  //
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();                    // Disconnect the Wifi before re-enabling it

  delay(1000);                          // Wait for system to stabilize
  
  Serial.flush();
  digitalWrite(RDY, HIGH);              // Set RDY to TM4C
  delay (500);                          // Wait before checking if serial data is being sent
   
  while ((Serial.available() == 0)) {}  // Wait for TM4C to start sending data
  
  while (Serial.available() > 0)   {

      char inchar = Serial.read();  // assigns one byte (as serial.read()'s only input one byte at a time

      if (inchar != '\n') {         // if the incoming character is not a newline then process byte
        ser_buf[bufpos] = inchar;   // the buffer position in the array get assigned to the current read
        bufpos++;                   // once that has happend the buffer advances, doing this over and over again until the end of package marker is read.
        delay(10);
      }
  }
  if (bufpos  > 0) {
    strcpy(eid,         strtok(ser_buf, ","));
    strcpy(ssid,        strtok(NULL,    ","));  
    strcpy(password,    strtok(NULL,    ","));
    strcpy(mqtt_broker, strtok(NULL,    ","));
    strcpy(port,        strtok(NULL,    ","));
  }
  mqtt_port = atoi(port);

  // connect to a WiFi network
  WiFi.begin(ssid, password);
  
  #ifdef DEBUG1
    Serial.print("\nConnecting to WiFi..");
  #endif

  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");                 // Feedback that we are still connecting
  }
  #ifdef DEBUG1
    Serial.println("\nConnected to the WiFi network");

  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  #endif
  Serial.flush();

}   //  END OF WiFi Setup


// ----------------------------------------------------------------
// -----------------   MAIN SETUP  --------------------------------
// ----------------------------------------------------------------

void setup() {

  Serial.begin(9600);                     // Set baud rate to 9600;
  Serial.flush();                         // Flush the serial port buffer

  pinMode(0, INPUT);                      // Set GPIO_0 to an input
  pinMode(2, OUTPUT);                     // Set GPIO_2 to an output - RDY signal to the TM4C
  
  digitalWrite(RDY, LOW);                 // Set the RDY pin LOW

  Setup_Wifi();                           // This routine reads in the  EID, SSID, Password

  delay(100);
  
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  
  // ---   Connect to a mqtt broker    ---

  while (!client.connected()) {
      String client_id = "ee445l-mqtt-herman";
      //client_id += eid;
      
      #ifdef DEBUG1
      Serial.print("The client is connecting to the mqtt broker using client ID:  "); 
      Serial.println(client_id.c_str());
      Serial.flush();
      #endif

      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) 
      {
          Serial.println("EE445L MQTT broker connected");
          Serial.flush();
      } 
      else 
      {
          Serial.print("Connection failed with state = ");
          Serial.print(client.state());
          Serial.flush();
      }
  }
  // MQTT publish and subscribe
  snprintf(topic_subscribe, sizeof(topic_subscribe), "%s%s", eid, topic_w2b);   // Prepend EID to top
  
  if (client.subscribe(topic_subscribe))
  {
    Serial.print("Subscribed to: ");
    Serial.println(topic_subscribe);
    Serial.flush();
  } 
  else 
  {
    Serial.print("Subscribe failed with state = ");
    Serial.print(client.state());
    Serial.flush();
  }                 // Subscribe to the Web based controller 
  
  #ifdef DEBUG1
  //Serial.print("Subscribe Topic: ");
  //Serial.println(topic_subscribe);
  // Serial.print("EID: ");
  // Serial.println(eid);
  #endif
  

  
  timer.setInterval(1000L, tm4c2mqtt);            // Run the TM4C to MQTT interface once per second
  digitalWrite(RDY, LOW);                         // Set the RDY pin LOW
}


// -----------------------------------------------------------------------------------
// ---  This is the callback when messages are received from the Web Contoller  -----
// -----------------------------------------------------------------------------------

void callback(char *topic_subscribe, byte *payload, unsigned int length) {
  
  payload[length] = '\0';
  
  // #ifdef DEBUG5
  // Serial.print("Message arrived in topic:  ");
  // Serial.println(topic_subscribe);
  // Serial.print("Message (char):  ");
  
  // for (int i = 0; i < length; i++) 
  // {
  //      Serial.print((char) payload[i]);
  // }
  
  // Serial.println();
  // Serial.println("-----------------------");
  // #endif

  // Retreive W2B command from received data

  if (length  > 0) {
    strcpy(cmd,    strtok((char *)payload, ""));
    Serial.println(cmd);                  // Send the command to the TM4C

    // #ifdef DEBUG4
    // Serial.print("W2B Command:  ");
    // Serial.println(cmd);
    // Serial.println("-----------------------");
    // #endif
  }
}

// ------------------------------------------------------------------------
//  This routine sends Lab 4E data to the Web page
//
void tm4c2mqtt(void) {
  
  static int bufpos = 0;              // starts the buffer back at the first position in the incoming serial.read

  while (Serial.available() > 0)      // Wait for date from the TM4C
  {
    char inchar = Serial.read();      // Assigns one byte (as serial.read()'s only input one byte at a time
        
    if (inchar != '\n') {             // if the incoming character is not a newline then process byte the buffer position
      ser_buf[bufpos] = inchar;       // in the array get assigned to the current read once that has happend the buffer advances,
      bufpos++;                       //  doing this over and over again until the end of package marker is read.
      delay(10);
    }
    else if (inchar == '\n')
    {
       if (bufpos  > 0) 
       {
        strcpy(mode,      strtok(ser_buf, ","));  
        strcpy(hour,      strtok(NULL,    ","));
        strcpy(minute,    strtok(NULL,    ","));
        strcpy(second,    strtok(NULL,    ","));

        snprintf(topic_publish, sizeof(topic_publish), "%s%s", eid, pub_mode);
        client.publish(topic_publish,   mode,    1); 
        
        snprintf(topic_publish, sizeof(topic_publish), "%s%s", eid, pub_hour);
        client.publish(topic_publish,  hour,     1); 

        snprintf(topic_publish, sizeof(topic_publish), "%s%s", eid, pub_min);
        client.publish(topic_publish,   minute,  1); 
        
        snprintf(topic_publish, sizeof(topic_publish), "%s%s", eid, pub_sec);
        client.publish(topic_publish,   second,  1); 
      }

      bufpos = 127; 
      for (int i = 0; i < bufpos; i++)  (ser_buf[i]) = 0;
      bufpos = 0;     // Reset buffer pointer
    }
  
  } 
}

void loop() {
  timer.run();
  client.loop();

}