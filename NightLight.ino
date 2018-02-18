//////////////////////////////
#include <ESP8266WiFi.h>


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

const char* ssid = "SSID";
const char* password = "PASSWD";

int DS_pin = D4;
int STCP_pin = D5;
int SHCP_pin = D6;
int SUN_pin=D8;

byte planets=0;

int count=1;

boolean sun=false;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

void setup() {
  Serial.begin(9600);

  pinMode(DS_pin,OUTPUT);
  pinMode(STCP_pin,OUTPUT);
  pinMode(SHCP_pin,OUTPUT);
  writereg(planets);

  pinMode(SUN_pin,OUTPUT);  
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  server.begin();
  
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

void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  int val;
  if (req.indexOf("/byte/") != -1){
    planets = req.substring(req.indexOf("/byte/")+6).toInt();
    writereg(planets);
  }else if (req.indexOf("/planets/") != -1) {
    planets = planets | req.substring(req.indexOf("/planets/")+9).toInt();
    writereg(planets);
  } else if (req.indexOf("/sun/") != -1){
    if (sun)
        digitalWrite(SUN_pin, LOW);
      else
        digitalWrite(SUN_pin, HIGH);
      sun=!sun;
  }
  else {
    Serial.println("invalid request");
    client.stop();
    return;
  }

  // Set GPIO2 according to the request
  digitalWrite(2, val);
  
  client.flush();
  
  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
  s += (val)?"high":"low";
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed

}
