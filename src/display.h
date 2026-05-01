#pragma once
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include "pins.h"

GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(
  GxEPD2_213_BN(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY)
);

#define SCREEN_W        122
#define SCREEN_H        250
#define MARGIN            4
#define LINE_HEIGHT      14
#define CHARS_PER_LINE   19
#define LINES_PER_PAGE   15

void initDisplay() {
  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
  display.init(115200, true, 2, false);
  display.setRotation(1);  // change to 0/2/3 if text is sideways
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMono9pt7b);
  display.setFullWindow();
}

void displayMessage(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(MARGIN, 20);
    display.print(line1);
    if (line2) { display.setFont(&FreeMono9pt7b); display.setCursor(MARGIN, 40); display.print(line2); }
    if (line3) { display.setCursor(MARGIN, 58); display.print(line3); }
  } while (display.nextPage());
}