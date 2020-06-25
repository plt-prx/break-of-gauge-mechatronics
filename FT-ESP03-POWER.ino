/*************************************************************

  ESP03 code as POWER switch
  Break of Gauge exhibition, film transport mechanism
  © 2020 Paul Kolling

  ESP01-primary   {0x24, 0x6F, 0x28, 0xA4, 0x44, 0x34};
  ESP02-replica   {0x3C, 0x71, 0xBF, 0x7D, 0x67, 0x58};
  ESP03-power     {0x24, 0x6F, 0x28, 0xB1, 0xA3, 0xF4};
  
*************************************************************/

#include "WiFi.h"
#include <esp_now.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
}

void loop() {
}
