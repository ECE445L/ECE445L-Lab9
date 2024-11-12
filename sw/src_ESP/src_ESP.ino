// -----------------------------------------------------------------------------
//
// This code runs on the 8266 and is used to communicate to an MQTT broker. It is
// an mashup of various 8266-MQTT client routines found on the web. It was previously
// used in Lab 4E in 2023.
//
//  This code provide the communications between the TM4C and a Web Application 
//  that controls the Motor in Lab 8 
//
// Author:    Mark McDermott
//
// Rev 10:    9/12/24   New architecture for the payload to be sent to the Web App
//
// ----------------------------------------------------------------
// ----------------    DEFINES   ----------------------------------
// ----------------------------------------------------------------
//
// NOTE: Comment out the #defines to disable DEBUG level
// 
// #define     DEBUG1                  // First level of Debug
// #define     DEBUG2                  // Second level of Debug
// #define     DEBUG3                  // Third level of Debug
// #define     DEBUG4                  // Fourth level of Debug
// #define     DEBUG5

#define       RDY 2


#define sckTimeout      4000

// ----------------------------------------------------------------
// ---------------   INCLUDES    ----------------------------------
// ----------------------------------------------------------------
//
#include <ESP8266WiFi.h>            // WiFi drivers
#include <PubSubClient.h>           // MQTT drivers
#include <BlynkSimpleEsp8266.h>     // Blynk timer -- best timer for this appllication
#include <ArduinoJson.h>

#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------
// ----------------  VARIABLES    --------------------------------- 
// ----------------------------------------------------------------        
//
char    eid[20]               = "";
char    ssid[64]              = "";                      
char    password[64]          = "";   


char    U_left[10]           = "0";
char    U_right[10]          = "0";
char    error_left[10]       = "0";
char    error_right[10]      = "0";
char    rpm_left[10]         = "0";
char    rpm_right[10]        = "0";

char    Kp1[10]              = "0";
char    Kp2[10]              = "0";
char    Ki1[10]              = "0";
char    Ki2[10]              = "0";
char    Xstar[10]            = "0";

char    cmd[20];                                       
char    ser_buf[128];            

const uint  keepAlive       = 4000;

// ----------------------------------------------------------------
// -------------   Your Lab 9 MQTT Broker    -------------------
//

 const char *mqtt_username       = "";                   
 const char *mqtt_password       = "";
 char        mqtt_broker[20]     = "";
 char        port[5]             = "";
 int         mqtt_port;

// ----------------------------------------------------------------
// -------------     UT Server MQTT Broker    ---------------------
//
// const char *mqtt_broker         = "10.159.177.113";
// const char *mqtt_username       = "your_eid";                   
// const char *mqtt_password       = "";
// const int   mqtt_port           = 1883;

// ----------------------------------------------------------------
// --------------     Publish topic     --------------------------
//
const char   *pub_b2w            =   "/b2w";
char topic_publish[64];

// ----------------------------------------------------------------
// --------------     Subscribe topic     ------------------------
//
char  topic_w2b[64]             = "/w2b";   
char  topic_subscribe[64];

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

  char        ser_buf[256];
  static int  bufpos = 0;               // starts the buffer back at the first position in the incoming serial.read

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

      char inchar = Serial.read();    // assigns one byte (as serial.read()'s only input one byte at a time

      if (inchar != '\n') {           // if the incoming character is not a newline then process byte
        ser_buf[bufpos] = inchar;     // the buffer position in the array get assigned to the current read
        bufpos++;                     // once that has happend the buffer advances, doing this over and over again until the end of package marker is read.
        delay(10);
      }
  }

  // Rip out the info needed for the MQTT broker
  if (bufpos  > 0) {
    strcpy(eid,         strtok(ser_buf, ","));
    strcpy(ssid,        strtok(NULL,    ","));  
    strcpy(password,    strtok(NULL,    ","));
    strcpy(mqtt_broker, strtok(NULL,    ","));
    strcpy(port,        strtok(NULL,    ","));
  }

  mqtt_port = atoi(port);
  
  // Serial.println();
  // Serial.print("ESP Board MAC Address:  ");
  // Serial.println(WiFi.macAddress());
  // Serial.flush();
  
  // Connect to a WiFi network
  //
  WiFi.begin(ssid, password);
  
  #ifdef DEBUG2
    Serial.print("\nConnecting to WiFi..");
    Serial.flush();
  #endif
  
  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
       
       //Serial.print(".");                 // Feedback that we are still connecting
       Serial.flush();
  }
  #ifdef DEBUG2
    Serial.println("\nConnected to the WiFi network");
    Serial.flush();

  Serial.println();
  Serial.print("ESP Board MAC Address:  ");    // You will need to know the MAC address in order to ues utexas-iot
  Serial.println(WiFi.macAddress());
  Serial.flush();

  #endif
  Serial.flush();

}   //  END OF WiFi Setup


// ----------------------------------------------------------------
// -----------------   MAIN SETUP  --------------------------------
// ----------------------------------------------------------------

void setup() {

  Serial.begin(115200);                       // Set baud rate to 115200;
  Serial.flush();                             // Flush the serial port buffer

  pinMode(0, INPUT);                          // Set GPIO_0 to an input (not used)
  pinMode(2, OUTPUT);                         // Set GPIO_2 to an output - RDY signal to the TM4C
  
  digitalWrite(RDY, LOW);                     // Set the RDY pin LOW

  Serial.println("\n\rIn setup");

  delay(500);                                 // Delay before starting up the WiFi
  Setup_Wifi(); 

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  client.setKeepAlive(keepAlive);
  client.setSocketTimeout (sckTimeout);

  
  // ---   Connect to a mqtt broker    ---

  while (!client.connected()) {
      String client_id = "ee445l-mqtt-ESP8266";
      //client_id += eid;
      
      // #ifdef DEBUG4
      //   Serial.print("The client is connecting to the mqtt broker using client ID:  "); 
      //   Serial.println(client_id.c_str());
      //   Serial.flush();
      // #endif

      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) 
      {
          #ifdef DEBUG4
          Serial.println("EE445L MQTT broker connected");
          Serial.flush();
          #endif
      } 
      else 
      {
          #ifdef DEBUG4
          Serial.print("Connection failed with state = ");
          Serial.print(client.state());
          Serial.flush();
          #endif
      }
  }
  // MQTT publish and subscribe
  snprintf(topic_subscribe, sizeof(topic_subscribe), "%s%s", eid, topic_w2b);   // Prepend EID to topic
  
  if (client.subscribe(topic_subscribe))
  {
    #ifdef DEBUG2
    Serial.print("Subscribed to: ");
    Serial.println(topic_subscribe);
    Serial.flush();
    #endif
  } 
  else 
  {
    #ifdef DEBUG2
    Serial.print("Subscribe failed with state = ");
    Serial.print(client.state());
    Serial.flush();
    #endif
  }                 
  
  
  // #ifdef DEBUG2
  //   Serial.print("Subscribe Topic: ");
  //   Serial.println(topic_subscribe);
  //   Serial.print("EID: ");
  //   Serial.println(eid);
  // #endif
  

  Serial.flush();
  timer.setInterval(1000L, tm4c2mqtt);            // Run the TM4C to MQTT interface once per second
  digitalWrite(RDY, LOW);                         // Set the RDY pin LOW
  
  #ifdef DEBUG2
  Serial.println("Leaving setup");
  #endif


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

  // -------------------------------------------------------------------
  // Retreive W2B command from received data
  //
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
//            111,222,333,444,555,666,
// ------------------------------------------------------------------------
//  This routine sends Lab 9 data to the Web page
//
void tm4c2mqtt(void) 
{
  
  static uint bufpos = 0;             // starts the buffer back at the first position in the incoming serial.read

  while (Serial.available() > 0)      // Wait for date from the TM4C
  {
    char inchar = Serial.read();      // Assigns one byte (as serial.read()'s only input one byte at a time
        
    if (inchar != '\n') 
    {                                 // if the incoming character is not a newline then process byte the buffer position
      ser_buf[bufpos] = inchar;       // in the array get assigned to the current read once that has happend the buffer advances,
      bufpos++;                       // doing this over and over again until the end of package marker is read.
      delay(10);
    }
    else if (inchar == '\n')
    {
       #ifdef DEBUG4
         Serial.println();
         Serial.print("bufpos= ");    // Print out bufpos to determine how many characters were rcvd.....
         Serial.print(bufpos); 
         Serial.println();
         Serial.flush();
       #endif    
        
      // Rip out the data from the CSV transmission rcvd from the TM4C
      //
       if (bufpos  > 0) 
       {
        strcpy(U_left,            strtok(ser_buf, ","));  
        strcpy(U_right,           strtok(NULL,    ","));
        strcpy(error_left,        strtok(NULL,    ","));
        strcpy(error_right,       strtok(NULL,    ","));
        strcpy(rpm_left,          strtok(NULL,    ","));
        strcpy(rpm_right,         strtok(NULL,    ","));  
        strcpy(Kp1,               strtok(NULL,    ","));
        strcpy(Kp2,               strtok(NULL,    ","));
        strcpy(Ki1,               strtok(NULL,    ","));
        strcpy(Ki2,               strtok(NULL,    ","));
        strcpy(Xstar,             strtok(NULL,    ","));


        // Define the JSON payload
        //
        JsonDocument doc;

        // Select data to send to Web App.
        
        doc["A"]    =   U_left;
        doc["B"]    =   U_right;
        doc["C"]    =   error_left;
        doc["D"]    =   error_right;
        doc["E"]    =   rpm_left;
        doc["F"]    =   rpm_right;
        doc["G"]    =   Kp1;
        doc["H"]    =   Kp2;
        doc["I"]    =   Ki1;
        doc["J"]    =   Ki2;
        doc["K"]    =   Xstar;

        // This builds the Json packet for publication.
        //
        serializeJson(doc, ser_buf);

        snprintf(topic_publish, sizeof(topic_publish), "%s%s", eid, pub_b2w);
        client.publish(topic_publish,   ser_buf,    1); 

      }
    }
  } 

      #ifdef DEBUG6
          bufpos = 40; 
          for (int i = 0; i < bufpos; i++) (Serial.print(ser_buf[i]));      
          bufpos = 0;     // Reset buffer pointer
      #endif

      bufpos = 127; 
      for (int i = 0; i < bufpos; i++)  (ser_buf[i]) = 0;
      bufpos = 0;     // Reset buffer pointer
}

void reconnect() 
{
  while (!client.connected()) 

  {  
      // Loop until we're reconnected
      // Serial.print("MQTT connection...");
      // String clientId = "stevensarns";
      String clientId = "ee445l-mqtt-ESP8266";
      clientId += String(random(0xffff), HEX);   // Create a random client ID

    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) 
    {
      
     // client.publish(topic_publish,   U_left,    1); 
      snprintf(topic_subscribe, sizeof(topic_subscribe), "%s%s", eid, topic_w2b);   // Prepend EID to topic
  
      if (client.subscribe(topic_subscribe))
      {
        // #ifdef DEBUG2
        // Serial.print("Subscribed to: ");
        // Serial.println(topic_subscribe);
        // Serial.flush();
        // #endif
      } 
      else 
      { 
        // #ifdef DEBUG2
        // Serial.print("Subscribe failed with state = ");
        // Serial.print(client.state());
        // Serial.flush();
        // #endif
      }
    }                 // Subscribe to the Web based controller 
    else 
    {
      delay(5000);      // Wait 5 seconds before retrying
    }
  }  
}

//   This is a test Json payload that can be used to verify the ESP is working correctly
//   {"A":300, "B":345, "C":4567, "D":12343, "E":9985, "F":75664}


void loop() 
{
  if (!client.connected()) reconnect(); // always executes on first loop
  timer.run();
  client.loop();
  if (!client.connected()) 
  {
    // Serial.print("Lost connection to Broker ");
    // Serial.print(client.state());
    // Serial.flush();
    setup();
  }
}