#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <SPI.h>

#define timeSeconds 2.5
#define MAX 512
#define MAXMOV 7
#define ISS 1
#define DATAPACKETEMPTY 6

WiFiClient client;
WiFiUDP udp;
WiFiUDP Udp;
NTPClient timeClient(Udp);

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
uint8_t errorPacket[7];
uint8_t movPacket[MAXMOV];
uint8_t packetACK[3];

void print_bitsT(unsigned char x) {
  int i;
  for (i = 8 * sizeof(x) - 1; i >= 0; i--) {
    (x & (1 << i)) ? putchar('1') : putchar('0');
  }
  printf("\n");
}

void IRAM_ATTR detectsMovement() {
  Serial.println("~~~~~~~~~~~~~~~~~~~~~\nMOVIMENTO DETETADO");
  digitalWrite(led, HIGH);
  startTimer = true;
  lastTrigger = millis();
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.print("A conectar-se a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
  Serial.println("----------------------------------");
  Serial.print("WiFi ligado com IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("----------------------------------");
  timeClient.begin();
  //timeClient.setTimeOffset(3600);
  pinMode(motionSensor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
  pinMode(light, OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(light, LOW);
  digitalWrite(led, LOW);
  udp.begin(8080);
}

void loop() {
   
  // MOTION STOP
  if (startCollecting == true) {
    unsigned long currentMillis = millis();
    unsigned long curMillis = millis();
    current = millis();
    if (startTimer && (current - lastTrigger > (timeSeconds * 1000))) {
      samplesMov();
    }

    if (currentMillis - previousMillis >= sampleFreq * 1000) {
      samplesLDR(currentMillis, counter);
      counter += 7;
      nSamples++;
    }
    if (nSamples == (sampleTime / sampleFreq)) {
      udp.beginPacket(host, port);
      udp.write(dataPacket, sizeof(dataPacket));
      udp.endPacket();
      nSamples = 0;
      prevMillis = curMillis;
      counter = 0;
    }
    if (value_lightSensor < 100) {
      digitalWrite(led, HIGH);
    } else {
      digitalWrite(led, LOW);
    }
  }
  uint8_t packetRecPc[50]; 
  udp.beginPacket(host, port);
  udp.write(packetRecPc,50);
  udp.endPacket();
  memset(packetRecPc, 0, 50);
  udp.parsePacket();
  //digitalWrite(light, HIGH);
  /*if(udp.read(packetRec, 50) > 0){
    Serial.print("Server to client: ");
    Serial.println((char *)packetRec);
  }*/
  char packetRec[8];
  if(udp.read(packetRecPc, 8) > 0){  
    printf("BOAS\n");
    for(int i = 0 ; i<8;i++){
      packetRec[i] = (char)packetRecPc[i];
            //print_bitsT(packetRec[i]);
    }
    printf("%d\n", (int)packetRec[0]);
    if (packetRec[1] == ISS) {
      printf("AQUI\n");
      switch (packetRec[0]) {
        case 0:    // START
          getStart(packetRec);
          printf("START PACKET\n");
          createACK(packetACK,6,1);
          udp.beginPacket(host, port);
          udp.write(packetACK, sizeof(packetACK));
          udp.endPacket();
          Serial.println("Confirmacao Enviada");
          break;
        case 1:    // STOP
          getStop(packetRec);
          printf("STOP PACKET\n");
          createACK(packetACK,6,2);
          udp.beginPacket(host, port);
          udp.write(packetACK, sizeof(packetACK));
          udp.endPacket();
          Serial.println("Confirmacao Enviada");
          break;
        case 4:    // LIGHT
          getLight(packetRec);
          udp.beginPacket(host, port);
          udp.write(6);
          udp.endPacket();
          printf("LIGHT PACKET\n");
          createACK(packetACK,6,3);
          udp.beginPacket(host, port);
          udp.write(packetACK, sizeof(packetACK));
          udp.endPacket();
          Serial.println("Confirmacao Enviada");
          break;
      }
    }
  }
}

void samplesMov() {
  createDATA(movPacket, 3);
  movPacket[6] = 1;
  udp.beginPacket(host, port);
  udp.write(movPacket, sizeof(movPacket));
  udp.endPacket();
  digitalWrite(led, LOW);
  startTimer = false;
}

void samplesLDR(unsigned long currentMillis, int counter) {
  createDATA(dataPacket, 2);
  insertValuesDataPacket(counter);
  previousMillis = currentMillis;
}

long getTime() {
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  time_t epcohTime = timeClient.getEpochTime();
  return epcohTime;
}

void getStart(char startPacket[8]) {
  time_t curTime = 0;
  char timeStamp[4];
  for (int j = 0; j < 4; j++) {
    timeStamp[j] = startPacket[j + 2];
  }
  memcpy(&curTime, timeStamp, 4);
  sampleTime = (int)(startPacket[6]);
  sampleFreq = (int)(startPacket[7]);
  startCollecting = true;
  printf("%d\n", sampleTime/sampleFreq);
  if(sampleTime/sampleFreq > 72){
    sendError(1);
    startCollecting = false;
  } 
  printf("Trama Start recebida\n");
  Serial.println(ctime(&curTime));
  Serial.println(sampleTime);
  Serial.println(sampleFreq);
}

void getStop(char startPacket[8]) {
  time_t curTime = 0;
  char timeStamp[4];
  for (int j = 0; j < 4; j++) {
    timeStamp[j] = startPacket[j + 2];
  }
  memcpy(&curTime, timeStamp, 4);
  startCollecting = false;
}

void getLight(char lightPacket[8]) {
  time_t curTime = 0;
  char timeStamp[4];
  for (int j = 0; j < 4; j++) {
    timeStamp[j] = lightPacket[j + 2];
  }
  memcpy(&curTime, timeStamp, 4);
  Serial.println(ctime(&curTime));
  if (lightPacket[6] == 0) {
    digitalWrite(light, LOW);
  }
  if (lightPacket[6] == 1) {
    digitalWrite(light, HIGH);
  }
}

void sendError(uint8_t errorType){
  errorPacket[0] = (uint8_t)5; 
  uint8_t id = ISS;
  memcpy(errorPacket + 1, &id, 1);
  time_t timeStamp = getTime();
  memcpy(errorPacket + 2, &timeStamp, 4);
  errorPacket[6] = (uint8_t)errorType;
  for (int i = 0; i < 7; i++) {
    Serial.println(errorPacket[i], BIN);
    }
  Serial.println("ERROR PACKET");
  udp.beginPacket(host, port);
  udp.write(errorPacket, sizeof(errorPacket));
  udp.endPacket();
}

void insertValuesDataPacket(int counter) {
  float resistorVoltage, ldrVoltage;
  float ldrResistance;
  String packetRec;
  value_lightSensor = analogRead(lightSensor);
  resistorVoltage = ((float)value_lightSensor * 3.3)/ 4095;
  ldrVoltage = 3.3 - resistorVoltage;
  ldrResistance = ldrVoltage / resistorVoltage * 1000;
  float lux = 9825678.384 * pow(ldrResistance, -1.399789643);
  memcpy(dataPacket + DATAPACKETEMPTY + counter, &value_lightSensor, 2);
  memcpy(dataPacket + DATAPACKETEMPTY + counter + 2, &ldrResistance, 4);
  value_light = digitalRead(led);
  memcpy(dataPacket + DATAPACKETEMPTY + counter + 6, &value_light, 1);
  printf("**********************\nTrama Enviada\n");
  printf("ADC :%d\n", value_lightSensor);
  printf("Resistencia LDR:%.2f\n", ldrResistance);
  printf("TensÃ£o LDR: %.2f\n", ldrVoltage);
  printf("Lux: %.2f\n", lux);
  printf("LAMPADA: %d\n", value_light);
}
void createACK(uint8_t *pacote, uint8_t tipo, uint8_t codigoo) {
  pacote[0] = (uint8_t)tipo;
  uint8_t id = ISS;
  memcpy(pacote + 1, &id, 1);
  /*time_t timeSample = getTime();
  memcpy(pacote + 2, &timeSample, 4);*/
  uint8_t codigo = codigoo;
  memcpy(pacote + 2, &codigo, 1);

}

void createDATA(uint8_t *pacote, uint8_t tipo) {
  pacote[0] = (uint8_t)tipo;
  uint8_t id = ISS;
  memcpy(pacote + 1, &id, 1);
  time_t timeSample = getTime();
  memcpy(pacote + 2, &timeSample, 4);
}
