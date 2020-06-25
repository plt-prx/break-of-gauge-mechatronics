/*************************************************************

  ESP rewind code
  Break of Gauge exhibition, film transport mechanism
  2020 Paul Kolling

  ESP01-primary   {0x24, 0x6F, 0x28, 0xA4, 0x44, 0x34};
  ESP02-replica   {0x3C, 0x71, 0xBF, 0x7D, 0x67, 0x58};
  ESP03-power     {0x24, 0x6F, 0x28, 0xB1, 0xA3, 0xF4};

*************************************************************/

#include <timer.h>

#define PIN_DIR    26
#define PIN_STEP   25
#define PIN_CFG0    4
#define PIN_CFG3    5
#define PIN_CFG2   18
#define PIN_CFG1   19
#define PIN_EN     16

#define PIN_F_STEP 21
#define PIN_F_EN   13

#define PIN_PD    34
#define PIN_LED   17

float cmM = 500 ;
float rpm = 15.2 / cmM;
int stepInterval = rpm / 1600 * 1000000 * 60;

Timer<1, micros> timerOneStepS2M;
Timer<1, micros> timerOneFStep;



/* SETUP */
/* ---------------------------------------------------------------------------------- */

void setup() {
  Serial.begin(115200);

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
  digitalWrite(PIN_CFG0, LOW);
  digitalWrite(PIN_CFG1, LOW);
  digitalWrite(PIN_EN, LOW);

  digitalWrite(PIN_F_STEP, HIGH);
  digitalWrite(PIN_F_EN, LOW);

  digitalWrite(PIN_LED, HIGH);

  timerOneStepS2M.every(stepInterval, oneStepS2M);
  timerOneFStep.every(300, oneStepFurl);
}



/* LOOP */
/* ---------------------------------------------------------------------------------- */

void loop() {
  if (map(analogRead(PIN_PD), 0, 4095, 0, 100) <= 5) {
    timerOneStepS2M.tick();
  }
}



/* ONE STEP */
/* ---------------------------------------------------------------------------------- */

bool oneStepS2M(void *) {
  digitalWrite(PIN_DIR, LOW);

  timerOneFStep.tick();

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
