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
uint8_t errorPacket[7];
uint8_t movPacket[MAXMOV];

void print_bitsT(unsigned long x) {
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
}

void loop() {
  String packetRec;
  if (!client.connect(host, port)) {
    Serial.println("Ligação com o concentrador falhou");
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

    if (currentMillis - previousMillis >= sampleFreq * 1000) {
      samplesLDR(currentMillis, counter);
      counter += 7;
      nSamples++;
    }
    if (nSamples == (sampleTime / sampleFreq)) {
      client.write(dataPacket, sizeof(dataPacket));
      client.stop();
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
  //digitalWrite(light, HIGH);
  if (client.connected()) {
    packetRec = client.readString();
    if (packetRec[1] == ISS) {
      switch (packetRec[0]) {
        case 0:    // START
          getStart(packetRec);
          printf("START PACKET\n");
          break;
        case 1:    // STOP
          getStop(packetRec);
          printf("STOP PACKET\n");
          break;
        case 4:    // LIGHT
          getLight(packetRec);
          client.write(6);
          client.stop();
          printf("LIGHT PACKET\n");
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

void getStart(String startPacket) {
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

void getStop(String startPacket) {
  time_t curTime = 0;
  char timeStamp[4];
  for (int j = 0; j < 4; j++) {
    timeStamp[j] = startPacket[j + 2];
  }
  memcpy(&curTime, timeStamp, 4);
  startCollecting = false;
}

void getLight(String lightPacket) {
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
  client.write(errorPacket, sizeof(errorPacket));
  Serial.println("ERROR PACKET");
  client.stop();
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
  printf("Tensão LDR: %.2f\n", ldrVoltage);
  printf("Lux: %.2f\n", lux);
  printf("LAMPADA: %d\n", value_light);
}

void createDATA(uint8_t *pacote, uint8_t tipo) {
  pacote[0] = (uint8_t)tipo;
  uint8_t id = ISS;
  memcpy(pacote + 1, &id, 1);
  time_t timeSample = getTime();
  memcpy(pacote + 2, &timeSample, 4);
}
