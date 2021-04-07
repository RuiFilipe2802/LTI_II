#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <SPI.h>

#define timeSeconds 1
#define MAX 512
#define MAXMOV 7
#define ISS 1
#define DATAPACKETEMPTY 6

WiFiClient client;
WiFiUDP udp;
NTPClient timeClient(udp);

const int light = 16;
const int led = 25;
const int motionSensor = 18;
const int lightSensor = 32;

const char *ssid = "MEO-CASA";
const char *password = "10203040";
const uint16_t port = 8080;
const char *host = "192.168.1.119";
int value_light = 0;
int value_lightSensor = 0;
int value_motionSensor = 0;
int increment = 0;
boolean startCollecting = false;
boolean startTimer = false;
unsigned long previousMillis = 0;
unsigned long prevMillis = 0;

int sampleTime;
int sampleFreq;
int counter = 0;
int nSamples = 0;

unsigned long current = millis();
unsigned long lastTrigger = 0;

uint8_t dataPacket[MAX];
uint8_t movPacket[MAXMOV];

void print_bitsT(unsigned long x) {
  int i;
  for (i = 8 * sizeof(x) - 1; i >= 0; i--) {
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
  Serial.println(WiFi.localIP());
  Serial.println("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
  timeClient.begin();
  //timeClient.setTimeOffset(3600);
  pinMode(motionSensor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
  pinMode(light, OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(light, LOW);
  digitalWrite(led, LOW);
}

void loop() {
  String packetRec;
  if (!client.connect(host, port)) {
    Serial.println("Connection to host failed");
    delay(1000);
    return;
  }

  // MOTION STOP
  if (startCollecting == true) {
    unsigned long currentMillis = millis();
    unsigned long curMillis = millis();
    current = millis();
    if (startTimer && (current - lastTrigger > (timeSeconds * 1000))) {
      samplesMov();
    }

    if (currentMillis - previousMillis >= (sampleTime / sampleFreq) * 1000) {
      samplesLDR(currentMillis, counter);
      counter += 3;
      nSamples++;
    }
    printf("%d\n", curMillis - prevMillis);
    if (nSamples == 5) {
      client.write(dataPacket, sizeof(dataPacket));
      client.stop();
      nSamples = 0;
      prevMillis = curMillis;
      counter = 0;
    }
  }
  //digitalWrite(light, HIGH);
  if (client.connected()) {
    packetRec = client.readString();
    if (packetRec[1] == ISS) {
      switch (packetRec[0]) {
        case 0:    // START
          getStart(packetRec);
          printf("START PACKET");
          break;
        case 1:    // STOP
          getStop(packetRec);
          printf("STOP PACKET");
          break;
        case 4:    // LIGHT
          getLight(packetRec);
          printf("LIGHT PACKET");
          break;
      }
    }
  }
}

void samplesMov() {
  createDATA(movPacket, 3);
  movPacket[6] = 1;
  client.write(movPacket, sizeof(movPacket));
  client.stop();
  Serial.println("Motion stopped...");
  digitalWrite(led, LOW);
  startTimer = false;
}

void samplesLDR(unsigned long currentMillis, int counter) {
  createDATA(dataPacket, 2);
  insertValuesDataPacket(counter);
  // for(int t=0;t<MAX;t++){
  for (int t = 0; t < 25; t++) {
    Serial.print(dataPacket[t], BIN);
    Serial.print(",");
  }
  printf("\n");
  previousMillis = currentMillis;
}

long getTime() {
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  // formattedDate = timeClient.getFormattedDate();
  // Serial.println(formattedDate);
  time_t epcohTime = timeClient.getEpochTime();
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

void getStart(String startPacket) {
  time_t curTime = 0;
  char timeStamp[4];
  for (int i = 0; i < 8; i++) {
    Serial.println(startPacket[i], BIN);
  }
  for (int j = 0; j < 4; j++) {
    timeStamp[j] = startPacket[j + 2];
    Serial.println(timeStamp[j], BIN);
  }
  memcpy(&curTime, timeStamp, 4);
  Serial.println(ctime(&curTime));
  sampleFreq = (int)(startPacket[6]);
  sampleTime = (int)(startPacket[7]);
  Serial.println(sampleFreq);
  Serial.println(sampleTime);
  startCollecting = true;
}

void getStop(String startPacket) {
  time_t curTime = 0;
  char timeStamp[4];
  for (int i = 0; i < 8; i++) {
    Serial.println(startPacket[i], BIN);
  }
  for (int j = 0; j < 4; j++) {
    timeStamp[j] = startPacket[j + 2];
    Serial.println(timeStamp[j], BIN);
  }
  memcpy(&curTime, timeStamp, 4);
  Serial.println(ctime(&curTime));
  startCollecting = false;
}

void getLight(String lightPacket){
  time_t curTime = 0;
  char timeStamp[4];
  printf("^^^^");
  for (int i = 0; i < 7; i++) {
    Serial.println(lightPacket[i], BIN);
  }
  for (int j = 0; j < 4; j++) {
    timeStamp[j] = lightPacket[j + 2];
    Serial.println(timeStamp[j], BIN);
  }
  memcpy(&curTime, timeStamp, 4);
  Serial.println(ctime(&curTime));
  if(lightPacket[6] == 0){
    digitalWrite(light, LOW);
  }
  if(lightPacket[6] == 1){
    digitalWrite(light, HIGH);
  }
  Serial.println(lightPacket[6], BIN);
}

void insertValuesDataPacket(int counter) {
  value_lightSensor = analogRead(lightSensor);
  memcpy(dataPacket + DATAPACKETEMPTY + counter, &value_lightSensor, 2);
  Serial.println(value_lightSensor);
  value_lightSensor = 50;
  value_light = digitalRead(light);
  memcpy(dataPacket + DATAPACKETEMPTY + counter + 2, &value_light, 1);
  Serial.println(value_light);
}

void createDATA(uint8_t *pacote, uint8_t iss) {
  pacote[0] = (uint8_t)iss;
  uint8_t id = ISS;
  memcpy(pacote + 1, &id, 1);
  time_t timeSample = getTime();
  memcpy(pacote + 2, &timeSample, 4);
}
