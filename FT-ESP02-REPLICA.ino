/*************************************************************

  ESP02 code as REPLICA
  Break of Gauge exhibition, film transport mechanism
  © 2020 Paul Kolling

  ESP01-primary   {0x24, 0x6F, 0x28, 0xA4, 0x44, 0x34};
  ESP02-replica   {0x3C, 0x71, 0xBF, 0x7D, 0x67, 0x58};
  ESP03-power     {0x24, 0x6F, 0x28, 0xB1, 0xA3, 0xF4};

*************************************************************/

#include "WiFi.h"
#include <esp_now.h>
#include <timer.h>

#define PIN_DIR   26
#define PIN_STEP  25
#define PIN_CFG0   4
#define PIN_CFG3   5
#define PIN_CFG2  18
#define PIN_CFG1  19
#define PIN_EN    16

#define PIN_F_STEP 21
#define PIN_F_EN   13

#define PIN_PD    34
#define PIN_LED   17

uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0xA4, 0x44, 0x34};

bool globalPower          = false;
bool globalDirection      = true;
bool localDirection       = true;
bool sendSuccess          = false;

bool safety               = false;
int safetyCount           = 0;

int lightSensor           = 100;
int lightSensorCount      = 0;
int thresholdSensor       = 0;
int lightSensorCountLimit = 100;

typedef struct struct_message {
  int msgCode;
  bool sendPower;
  bool sendDirection;
} struct_message;

struct_message msgSend;
struct_message msgReceive;

String msgSendNoti = "noo";
String msgReceiveNoti = "noo";

float cmM = 5 ;
float rpm = 15.2 / cmM;
int stepInterval = rpm / 12800 * 1000000 * 60;


Timer<1, micros> timerOneStepM2S;
Timer<1, micros> timerOneStepS2M;
Timer<1, micros> timerOneFStep;
Timer<1, micros> timerReadSensor;
Timer<1, micros> timerPower;
Timer<1, micros> DEBUG;



/* SEND & RECEIVE */
/* ---------------------------------------------------------------------------------- */

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == 0) {
    sendSuccess = true;
  }
  else {
    sendSuccess = false;
  }

  msgSendNoti = "yes";
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&msgReceive, incomingData, sizeof(msgReceive));

  if (msgReceive.msgCode == 1) {
    globalPower = msgReceive.sendPower;
  }

  if (msgReceive.msgCode == 2) {
    globalDirection = msgReceive.sendDirection;
  }

  if (msgReceive.msgCode == 3) {
    globalPower = msgReceive.sendPower;
    globalDirection = msgReceive.sendDirection;
    localDirection = globalDirection;
  }

  msgReceiveNoti = "yes";
}



/* SETUP */
/* ---------------------------------------------------------------------------------- */

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_peer_info_t peerInfo;

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_CFG0, OUTPUT);
  pinMode(PIN_CFG1, OUTPUT);
  pinMode(PIN_EN, OUTPUT);

  pinMode(PIN_F_STEP, OUTPUT);
  pinMode(PIN_F_EN, OUTPUT);

  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_DIR, HIGH);
  digitalWrite(PIN_STEP, HIGH);
  digitalWrite(PIN_CFG0, HIGH);
  digitalWrite(PIN_CFG1, HIGH);
  digitalWrite(PIN_EN, LOW);

  digitalWrite(PIN_F_STEP, HIGH);
  digitalWrite(PIN_F_EN, LOW);

  digitalWrite(PIN_LED, HIGH);


  timerPower.every(100000, powerESP);
  timerOneStepM2S.every(stepInterval, oneStepM2S);
  timerOneStepS2M.every(stepInterval, oneStepS2M);
  timerOneFStep.every(20000, oneStepFurl);
  timerReadSensor.every(250000, readSensor);
  DEBUG.every(500000, printDEBUG);

  msgSend.msgCode = 3;
  esp_err_t safetyResult = esp_now_send(broadcastAddress, (uint8_t *) &msgSend, sizeof(msgSend));
  msgSend.msgCode = 0;
}



/* LOOP */
/* ---------------------------------------------------------------------------------- */

void loop() {
  timerPower.tick();

  if (globalPower == true && safety == true) {
    if (globalDirection == true) {
      timerOneStepM2S.tick();
    }
    else {
      timerOneStepS2M.tick();
    }
  }
  else {
    digitalWrite(PIN_F_EN, HIGH);
  }

  DEBUG.tick();
}



/* ONE STEP */
/* ---------------------------------------------------------------------------------- */

bool oneStepM2S(void *) {
  digitalWrite(PIN_DIR, LOW);
  digitalWrite(PIN_LED, HIGH);

  timerOneFStep.tick();

  digitalWrite(PIN_STEP, LOW);
  delay(1);
  digitalWrite(PIN_STEP, HIGH);
  return true;
}


bool oneStepS2M(void *) {
  digitalWrite(PIN_DIR, HIGH);
  digitalWrite(PIN_LED, HIGH);

  timerReadSensor.tick();
  digitalWrite(PIN_F_EN, HIGH);

  digitalWrite(PIN_STEP, LOW);
  delay(1);
  digitalWrite(PIN_STEP, HIGH);
  return true;
}


bool oneStepFurl(void *) {
  digitalWrite(PIN_F_EN, LOW);

  digitalWrite(PIN_F_STEP, LOW);
  delay(1);
  digitalWrite(PIN_F_STEP, HIGH);
  return true;
}


/* DIRECTION MANAGEMENT */
/* ---------------------------------------------------------------------------------- */

bool readSensor(void *) {
  lightSensor = map(analogRead(PIN_PD), 0, 4095, 0, 100);

  if (lightSensor <= thresholdSensor) {
    lightSensorCount++;
    if (lightSensorCount >= lightSensorCountLimit) {
      localDirection = true;
      switchDirection();
    }
  }
  else {
    lightSensorCount = 0;
  }

  return true;
}



void switchDirection() {
  Serial.println("msgSend.sendDirection");

  msgSend.msgCode = 2;
  msgSend.sendDirection = localDirection;
  esp_err_t switchPowerResult = esp_now_send(broadcastAddress, (uint8_t *) &msgSend, sizeof(msgSend));
  msgSend.msgCode = 0;

  if (sendSuccess == true) {
    globalDirection = true;
    lightSensorCount = 0;
    sendSuccess = false;
  }
}



/* POWER MANAGEMENT */
/* ---------------------------------------------------------------------------------- */

bool powerESP(void *) {
  esp_err_t safetyResult = esp_now_send(broadcastAddress, (uint8_t *) &msgSend, sizeof(msgSend));

  if (sendSuccess == false) {
    safetyCount++;
    if (safetyCount >= 10) {
      safety = false;
      Serial.println("EMERGENCY SHUTDOWN!");
    }
  }
  else {
    safetyCount = 0;
    safety = true;
  }
  return true;
}




/* DEBUGGING */
/* ---------------------------------------------------------------------------------- */

bool printDEBUG(void *) {
  Serial.print("safety: ");
  Serial.print(safety);
  Serial.print(" | globalPower: ");
  Serial.print(globalPower);
  Serial.print(" | localDirection: ");
  Serial.print(localDirection);
  Serial.print(" | globalDirection: ");
  Serial.print(globalDirection);
  Serial.print(" | lightSensor: ");
  Serial.print(lightSensor);
  Serial.print("% | lightSensorCount: ");
  Serial.print(lightSensorCount);
  Serial.print(" | msgSent?: ");
  Serial.print(msgSendNoti);
  Serial.print(" | sendSuccess: ");
  Serial.print(sendSuccess);
  Serial.print(" | msgReceived?: ");
  Serial.print(msgReceiveNoti);
  Serial.print(" | msgReceive.msgCode: ");
  Serial.print(msgReceive.msgCode);
  Serial.print(" | msgReceive.sendPower: ");
  Serial.print(msgReceive.sendPower);
  Serial.print(" | msgReceive.sendDirection: ");
  Serial.println(msgReceive.sendDirection);

  msgSendNoti = "noo";
  msgReceiveNoti = "noo";

  return true;
}
