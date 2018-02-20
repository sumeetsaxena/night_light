#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "pass.h"
//#define WLAN_SSID "SSID"
//#define WLAN_PASS "PASS"
//#define AIO_SERVER      "SERVER"
//#define AIO_SERVERPORT  1883
//#define AIO_USERNAME    "USERNAME"
//#define AIO_KEY         "KEY"


/************ Global State (you don't need to change this!) ******************/
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'time' for subscribing to current time
Adafruit_MQTT_Subscribe timeFeed = Adafruit_MQTT_Subscribe(&mqtt, "time/seconds");

// Setup a feed called 'planets' for subscribing to which planets needs to turned on
Adafruit_MQTT_Subscribe planetsFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/planets", MQTT_QOS_1);

// Setup a feed called 'sun' for subscribing to changes to the sun-light status
Adafruit_MQTT_Subscribe sunFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/sun", MQTT_QOS_1);

// Setup a feed called 'timeout' for subscribing to changes to the timeout in minutes
Adafruit_MQTT_Subscribe timeoutFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/timeout", MQTT_QOS_1);

//////////////// PLANETS ////////////

#define LED_BUILTIN 16

static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;

int DS_pin = D4;
int STCP_pin = D5;
int SHCP_pin = D6;
int SUN_pin=D8;

byte planets=0;

///////////////////////////////////

//////////
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 1
#define BUILT_IN_LED 2
Adafruit_SSD1306 display(OLED_RESET);


/****************************** Console Display ***********************************/

static uint line=0;
void console_println(String str){
  if (line>=32)
  {
    display.clearDisplay();
    line=0;
  }
  display.setCursor(0,line);
  line+=8;
  
  display.println(str);
  display.display();
}


/****************************** MQTT Connection Management ***********************************/
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  console_println("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 10 seconds...");
       mqtt.disconnect();
       delay(10000);  // wait 10 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  console_println("MQTT Connected!");
}


/****************************** TIME Management ***********************************/
static uint32_t start_time=0;
static uint32_t elapsed_time=0;
static uint32_t timeout_checkpoint=0;
static uint32_t timeout=0;

String elapsedtime(){
  int sec;
  int min;
  int hour;
  uint32_t tsec = elapsed_time;
  
  // calculate current time
  sec = tsec % 60;
  tsec  /= 60;
  min = tsec % 60;
  tsec  /= 60;
  hour = tsec % 24;

  String time="";

  if (hour > 0)
    time+=String(hour)+((hour>=10)?" hrs " : " hr ");

  if (min > 0)
    time+=String(min)+((hour>=10)?" mins " : " min ");

  if (sec > 0)
    time+=String(sec)+((sec>=10)?" secs" : " sec");
          
  return time;
}

//int timeZone = -8; // utc-4 eastern daylight time (nyc)

//Callback for Time_Feed
void onTimeCallback(uint32_t current) {
  
  if (start_time==0)
    start_time=current;

  elapsed_time = current - start_time;

  String time_display=elapsedtime();  

  if (timeout > 0) //Timeout has been set
  {
    time_display=String(timeout - (elapsed_time - timeout_checkpoint)) + " secs ";
    if ((elapsed_time - timeout_checkpoint)>timeout) 
    {
      timeout = 0; // Timeout already called
      timeout_checkpoint=elapsed_time;
      digitalWrite(BUILT_IN_LED, 1);
      console_println("Timeout!");
      delay(1000);
    }
  }

  
  console_println(time_display);
}

/****************************** CALLBACKS - Lighting  ***********************************/
void onPlanetsCallback(uint32 planets) {
  console_println("Planets " + String(planets));
  if (planets>0)
    digitalWrite(BUILT_IN_LED, 1);
  else
    digitalWrite(BUILT_IN_LED, 0);

  writereg(planets);    
  
  delay(500);
 }

void onSunCallback(uint32 sun) {
  console_println("Sun " + String(sun));
  if (sun)
      digitalWrite(SUN_pin, LOW);
    else
      digitalWrite(SUN_pin, HIGH);  
  delay(500);  
}

void onTimeoutCallback(uint32 x) {
  console_println("Timeout :" + String(x) + " secs");
  delay(500);
  timeout=x;
  timeout_checkpoint=elapsed_time;
}

void writereg(byte lights)
{
  byte bits=lights;
  digitalWrite(STCP_pin, LOW);
  for (int i = 7; i>=0; i--)
  {
    digitalWrite(SHCP_pin, LOW);
    digitalWrite(DS_pin, (bits & 0x01)>0 );
    digitalWrite(SHCP_pin, HIGH);
    bits = bits >> 1;
  }
  digitalWrite(STCP_pin, HIGH);
}

void setup() {

  Serial.begin(9600);

  pinMode(DS_pin,OUTPUT);
  pinMode(STCP_pin,OUTPUT);
  pinMode(SHCP_pin,OUTPUT);
  writereg(planets);

  pinMode(SUN_pin,OUTPUT);    
  

  pinMode(BUILT_IN_LED, OUTPUT);
  digitalWrite(BUILT_IN_LED, 1);  //TURN OFF BUILT IN LED

  delay(10);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);  
  console_println("Welcome");
  delay(2000);

  // Connect to WiFi access point.
  console_println("Connecting to ");
  console_println(WLAN_SSID);
//
//WiFi.persistent(false);
//WiFi.mode(WIFI_OFF);   // this is a temporary line, to be removed after SDK update to 1.5.4
//WiFi.mode(WIFI_STA);
//WiFi.begin(ssid, password);

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WLAN_SSID, WLAN_PASS);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    console_println("Connecting...");
  }

  console_println("WiFi connected");
  delay(2000);  
  console_println(String(WiFi.localIP()));
  delay(2000);

  timeFeed.setCallback(onTimeCallback);
  planetsFeed.setCallback(onPlanetsCallback);
  sunFeed.setCallback(onSunCallback);
  timeoutFeed.setCallback(onTimeoutCallback);  
  
  // Setup MQTT subscription for time feed.
  mqtt.subscribe(&timeFeed);
  mqtt.subscribe(&planetsFeed);
  mqtt.subscribe(&sunFeed);
  mqtt.subscribe(&timeoutFeed);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets and callback em' busy subloop
  // try to spend your time here:
  mqtt.processPackets(10000);
  
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
}


