#pragma once
#include <Arduino.h>
#include "pins.h"

#define VDIV_RATIO  2.0f
#define ADC_REF     3.3f
#define ADC_MAX     4095.0f
#define BATT_MAX_V  4.2f
#define BATT_MIN_V  3.0f

int getBatteryPercent() {
  int sum = 0;
  for (int i = 0; i < 10; i++) { sum += analogRead(BAT_ADC); delay(2); }
  float voltage = (sum / 10.0f / ADC_MAX) * ADC_REF * VDIV_RATIO;
  return constrain((int)(((voltage - BATT_MIN_V) / (BATT_MAX_V - BATT_MIN_V)) * 100.0f), 0, 100);
}