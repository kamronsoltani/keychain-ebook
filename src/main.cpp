#include <Arduino.h>
#include <LittleFS.h>
#include <SPI.h>
#include "pins.h"
#include "display.h"
#include "battery.h"
#include "reader.h"
#include "upload_server.h"

enum AppState { STATE_MENU, STATE_READING, STATE_UPLOAD };
AppState appState = STATE_MENU;

std::vector<String> bookList;
int menuSelection = 0;

unsigned long lastActivityMs = 0;
#define SLEEP_TIMEOUT_MS (30 * 1000)

void resetSleepTimer() { lastActivityMs = millis(); }

void goToSleep() {
  saveBookmark();
  displayMessage("Sleeping...", "Press any button", "to wake");
  delay(1500);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_DOWN, 0);
  esp_deep_sleep_start();
}

#define DEBOUNCE_MS 150
unsigned long lastBtnTime[4] = {0,0,0,0};
bool btnPressed(int pin, int idx) {
  if(digitalRead(pin) == LOW && (millis() - lastBtnTime[idx]) > DEBOUNCE_MS){
    lastBtnTime[idx] = millis(); resetSleepTimer(); return true;
  }
  return false;
}

void handleMenuState() {
  if(bookList.empty()){ showBookMenu(bookList, 0); delay(300); return; }
  if(btnPressed(BTN_DOWN, 1)){
    menuSelection = (menuSelection + 1) % bookList.size();
    showBookMenu(bookList, menuSelection);
  } else if(btnPressed(BTN_UP, 0)){
    menuSelection = (menuSelection - 1 + bookList.size()) % bookList.size();
    showBookMenu(bookList, menuSelection);
  } else if(btnPressed(BTN_SELECT, 2)){
    currentBook.filename  = bookList[menuSelection];
    currentBook.pageIndex = 0;
    displayMessage("Loading...", currentBook.filename.c_str());
    currentBook.totalPages = countPages(currentBook.filename);
    appState = STATE_READING;
    renderPage(currentBook.filename, currentBook.pageIndex, getBatteryPercent());
  }
}

void handleReadingState() {
  if(btnPressed(BTN_DOWN, 1)){
    if(currentBook.pageIndex < currentBook.totalPages - 1){
      currentBook.pageIndex++;
      renderPage(currentBook.filename, currentBook.pageIndex, getBatteryPercent());
      saveBookmark();
    }
  } else if(btnPressed(BTN_UP, 0)){
    if(currentBook.pageIndex > 0){
      currentBook.pageIndex--;
      renderPage(currentBook.filename, currentBook.pageIndex, getBatteryPercent());
      saveBookmark();
    }
  } else if(btnPressed(BTN_MENU, 3)){
    appState = STATE_MENU;
    bookList = getBookList();
    showBookMenu(bookList, menuSelection);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("PSRAM: %d  Flash: %d\n", ESP.getPsramSize(), ESP.getFlashChipSize());

  pinMode(BTN_MENU,   INPUT_PULLUP);
  pinMode(BTN_UP,     INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  if(!LittleFS.begin(true)){ LittleFS.format(); LittleFS.begin(); }

  initDisplay();

  delay(100);
  if(digitalRead(BTN_MENU) == LOW){
    appState = STATE_UPLOAD;
    startUploadServer();
    return;
  }

  if(loadBookmark()){
    currentBook.totalPages = countPages(currentBook.filename);
    appState = STATE_READING;
    renderPage(currentBook.filename, currentBook.pageIndex, getBatteryPercent());
  } else {
    bookList = getBookList();
    appState = STATE_MENU;
    showBookMenu(bookList, 0);
  }
  resetSleepTimer();
}

void loop() {
  if(appState == STATE_UPLOAD){ delay(10); return; }
  if(appState == STATE_MENU)           handleMenuState();
  else if(appState == STATE_READING)   handleReadingState();
  if(millis() - lastActivityMs > SLEEP_TIMEOUT_MS) goToSleep();
  delay(20);
}