#include <WiFi.h>

const char* ssid = "MEO-CASA";
const char* password = "10203040";
const uint16_t port = 8080;
const char * host = "192.168.1.118";
int val = 0;
int increment = 0;

void setup() {  
  Serial.begin(115200); 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }

  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());
  pinMode(25, OUTPUT); 
  pinMode(18, INPUT);
}  

void loop() { 
  WiFiClient client; 
  if (!client.connect(host, port)) {
   Serial.println("Connection to host failed");
   delay(1000);
   return;
}
  client.print(increment);
  client.stop();
  increment++;
  /*int value = analogRead(32);  
  val = digitalRead(18);  // read input value
  //Serial.print("MOTION SENSOR -> ");
  //Serial.println(val);  
  Serial.print("LUMINOSIDADE     ");
  Serial.println(value);
  delay(500);  
  if(value == 0){
   digitalWrite(25, HIGH);  // turn LED ON
  }else{
   digitalWrite(25, LOW);  // turn LED OFF
  }*/
  delay(1000);
}  
