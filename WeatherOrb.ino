/*
  Internet weather smartlight v1.0
  The WiFi manager will connect to wireless lan using settings.h
  Will download RSS XML from Yahoo Weather API and parse for a tag to retrieve actual temperature.
  The temperature will be encoded to a hue color, and converted to RGB values for the RGB led.
  The WiFi manager will update the temp value every ten minutes.
  
  Circuit:
  * WiFi Shield
  * RGB Led: 3, 5, 6 and common lead to 3.3V

  Improvements:
  * Retrieve time from inet and implement a night mode, to dim the light or switch off.

*/
#include <Serial.h>
#include <SPI.h>
#include <WiFi.h>

#include "settings.h"

#define ledPinR 3
#define ledPinG 5
#define ledPinB 6

int rgb_colors[3]; 
int hue;
int saturation;
int brightness;


int status = WL_IDLE_STATUS;

// Initialize the Wifi client library
WiFiClient client;

unsigned long lastConnectionTime = 0;           // last time you connected to the server, in milliseconds
boolean lastConnected = false;                  // state of the connection last time through the main loop
const unsigned long postingInterval = 10*60*1000;  // delay between updates, in milliseconds

String clientText;

void setup(){
  
    pinMode(ledPinR, OUTPUT);
    pinMode(ledPinG, OUTPUT);
    pinMode(ledPinB, OUTPUT);
  
  Serial.begin(9600);
  
  setRGBLed(255,255,255);
  
  delay(1500);
  
  setRGBLed(0,0,0);
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 
  
  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) { 
    setRGBLed(200,200,0);
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    status = WiFi.begin(ssid, pass);
    // wait for connection:
    delay(5000);
    setRGBLed(0,0,0);
    delay(1000);
  }
  
  Serial.println("Connected to SSID!");
  
}


void loop() {
  
  
  while (client.available()) {
    char c = client.read();
    
    clientText+=c;
    
    if (c=='>'){      
      parseTags(clientText);      
      clientText="";
    }
    
    //Serial.write(c); //debugging
  }
  
   
  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }

  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    httpRequest();
  }
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

// this method makes a HTTP connection to the server:
void httpRequest() {
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.print("GET ");
    client.print(page);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("User-Agent: arduino-wifi");
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println("disconnecting.");
    client.stop();
  }
}


void parseTags(String clientText){
  
  //Serial.println("PARSING");
  
  
//Serial.println(clientText);  

  int startPos = clientText.lastIndexOf("<yweather:condition");
  

  if(startPos >= 0) {
    
    Serial.print("Start tag found in ");
    Serial.println(startPos);
        
    int endPos = clientText.indexOf("/>",startPos);
    
    
    if (endPos>startPos){
      
      Serial.print("End tag found in ");
      Serial.println(endPos);
        
        Serial.println(clientText);
        
        int startPos = clientText.indexOf("temp=\"");
        int endPos = clientText.indexOf("\"",startPos+6);
        String strTemp=clientText.substring(startPos+6,endPos);
        Serial.println("Temp="+strTemp);
        
        char valueArray[strTemp.length() + 1];
        strTemp.toCharArray(valueArray, sizeof(valueArray));
        
        colorTemperature(atoi(valueArray));
       
    }else{
      Serial.println("End tag not found? ERROR");
    
    } 
     
  }
  
}

void colorTemperature(int temp){

 int minTemp=0;
 int maxTemp=40;
 
 int coldColor=225;
 int hotColor=0;
 
 //temp=15; //debugging colors
 
 getRGB(map(temp, minTemp, maxTemp, coldColor, hotColor), 255, 255, rgb_colors);
 
 setRGBLed(rgb_colors[0],rgb_colors[1],rgb_colors[2]);
 
}

void setRGBLed(int R,int G,int B){
  
  
  //invert values because of the common lead of the led
  R=abs(R-255);
  G=abs(G-255);
  B=abs(B-255);
  
  
  analogWrite(ledPinR, R);            // red value in index 0 of rgb_colors array
  analogWrite(ledPinG, G);            // green value in index 1 of rgb_colors array
  analogWrite(ledPinB, B);            // blue value in index 2 of rgb_colors array
  
}

void getRGB(int hue, int sat, int val, int colors[3]) { 
  /* convert hue, saturation and brightness ( HSB/HSV ) to RGB
     The dim_curve is used only on brightness/value and on saturation (inverted).
     This looks the most natural.      
  */
  
  /*
  val = dim_curve[val];
  sat = 255-dim_curve[255-sat];
  */
  
  int r;
  int g;
  int b;
  int base;
  
  if (sat == 0) { // Acromatic color (gray). Hue doesn't mind.
    colors[0]=val;
    colors[1]=val;
    colors[2]=val;  
  } else  { 
    
    base = ((255 - sat) * val)>>8;
  
    switch(hue/60) {
	case 0:
		r = val;
		g = (((val-base)*hue)/60)+base;
		b = base;
	break;
	
	case 1:
		r = (((val-base)*(60-(hue%60)))/60)+base;
		g = val;
		b = base;
	break;
	
	case 2:
		r = base;
		g = val;
		b = (((val-base)*(hue%60))/60)+base;
	break;
	
	case 3:
		r = base;
		g = (((val-base)*(60-(hue%60)))/60)+base;
		b = val;
	break;
	
	case 4:
		r = (((val-base)*(hue%60))/60)+base;
		g = base;
		b = val;
	break;
	
	case 5:
		r = val;
		g = base;
		b = (((val-base)*(60-(hue%60)))/60)+base;
	break;
    }
      
    colors[0]=r;
    colors[1]=g;
    colors[2]=b; 
  }   

}
