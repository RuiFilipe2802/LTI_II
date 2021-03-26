#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

#define timeSeconds 3
#define MAX 512
#define ID 1
#define DATAPACKETEMPTY 6

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const int light = 16;
const int led = 25;
const int motionSensor = 18;
const int lightSensor = 32;

const char* ssid = "MEO-CASA";
const char* password = "10203040";
const uint16_t port = 8080;
const char * host = "192.168.1.119";
int value_light = 0;
int value_lightSensor = 0; 
int value_motionSensor = 0;
int increment = 0;
boolean startCollecting = false;
boolean startTimer = false;

int sampleTime;
int sampleFreq;

unsigned long current = millis();
unsigned long lastTrigger = 0;

uint8_t dataPacket[MAX];

void print_bitsT(unsigned long x)
{
    int i;
    for (i = 8 * sizeof(x) - 1; i >= 0; i--)
    {
        (x & (1 << i)) ? putchar('1') : putchar('0');
    }
    printf("\n");
}


void IRAM_ATTR detectsMovement() {
  Serial.println("MOTION DETECTED!!!");
  digitalWrite(led, HIGH);
  startTimer = true;
  lastTrigger = millis();
}

void setup() {  
  Serial.begin(115200);
  delay(100);
  Serial.print("Connecting to ");
  Serial.println(ssid); 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
  Serial.print("WiFi connected with IP: ");
  Serial.println("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  pinMode(motionSensor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
  pinMode(light, OUTPUT);
  pinMode(led, OUTPUT); 
  digitalWrite(light, LOW);
  digitalWrite(led, LOW);
}  

void loop() {
  current = millis();
  WiFiClient client; 
  if (!client.connect(host, port)){
   Serial.println("Connection to host failed");
   delay(1000);
   return;
  }
  /*
  // MOTION STOP
  if(startTimer && (current - lastTrigger > (timeSeconds*1000))) {
      Serial.println("Motion stopped...");
      digitalWrite(led, LOW);
      startTimer = false;
  }*/
  digitalWrite(light, HIGH);
  //TRAMA START
  String startPacket;
  if(!client.available() && startCollecting == false){
    startPacket = client.readString();
    getStart(startPacket);
  } 
  if(startCollecting == true){
    createDATA(dataPacket);
    //Serial.println(dataPacket, BIN);
    //printf("%d\n", dataPacket);
  } 
  insertValuesDataPacket(dataPacket + DATAPACKETEMPTY);
  for(int t=0;t<MAX;t++){
      Serial.print(dataPacket[t], BIN);
      Serial.print(",");
    }
  delay(2000);
  
  /*
  // START COLLECTING SAMPLES
  if(startCollecting == true){
    int value_lightSensor = analogRead(32);  
    value_motionSensor = digitalRead(motionSensor);  // read input value
    Serial.print("MOTION SENSOR -> ");
    Serial.println(value_motionSensor);  
    Serial.print("LUMINOSIDADE     ");
    Serial.println(value_lightSensor);
    delay(500);  
    if(value_lightSensor == 0){
    digitalWrite(led, HIGH);  // turn LED ON
    }else{
    digitalWrite(led, LOW);  // turn LED OFF
    }
    }
  delay(1000);*/
} 

long getTime(){
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  //formattedDate = timeClient.getFormattedDate();
  //Serial.println(formattedDate);
  time_t epcohTime =  timeClient.getEpochTime();
  Serial.println(ctime(&epcohTime));
  return epcohTime;
  /*// Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
  delay(1000);*/
}

 void getStart(String startPacket){
  time_t curTime = 0;
  char timeStamp[4];
  for(int i=0;i<8;i++){
      Serial.println(startPacket[i], BIN);
      startCollecting = true;
  }
  Serial.println("------------");
  for(int j=0;j<4;j++){
    timeStamp[j] = startPacket[j+2];
    Serial.println(timeStamp[j], BIN);
  }
  memcpy(&curTime, timeStamp, 4);
  Serial.println(ctime(&curTime));
  sampleTime = (int)(startPacket[6]);
  sampleFreq = (int)(startPacket[7]);
  Serial.println(sampleTime);
  Serial.println(sampleFreq);

 } 

 void insertValuesDataPacket(uint8_t *pacote){
  value_lightSensor = analogRead(lightSensor);
  memcpy(pacote,&value_lightSensor, 2);
  Serial.println(value_lightSensor);
  value_motionSensor = digitalRead(motionSensor);
  memcpy(pacote + 2,&value_motionSensor, 1);
  Serial.println(value_motionSensor);
  value_light = digitalRead(light);
  memcpy(pacote + 3,&value_light, 1);
  Serial.println(value_light);
  
 }
 
 void createDATA(uint8_t *pacote){
  pacote[0] = (uint8_t)2;
  uint8_t id = ID;
  memcpy(pacote + 1, &id, 1);
  time_t timeSample = getTime();
  memcpy(pacote + 2, &timeSample, 4);
 }
