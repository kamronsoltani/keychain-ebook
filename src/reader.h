#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <vector>
#include "display.h"

struct BookState { String filename; int pageIndex; int totalPages; };
BookState currentBook;

std::vector<String> getBookList() {
  std::vector<String> books;
  File root = LittleFS.open("/");
  File f = root.openNextFile();
  while (f) {
    String name = String(f.name());
    if (!name.startsWith("/")) name = "/" + name;
    if (name.endsWith(".txt") && name != "/bookmark.txt") books.push_back(name);
    f = root.openNextFile();
  }
  return books;
}

int countPages(const String& path) {
  File f = LittleFS.open(path, "r");
  if (!f) return 0;
  int pages = 0, lineCount = 0;
  String line = "";
  while (f.available()) {
    char c = f.read();
    if (c == '\n' || c == '\r') {
      if (line.length() > 0) { lineCount += (line.length() / CHARS_PER_LINE) + 1; line = ""; }
    } else line += c;
    if (lineCount >= LINES_PER_PAGE) { pages++; lineCount = 0; }
  }
  if (lineCount > 0) pages++;
  f.close();
  return max(pages, 1);
}

void renderPage(const String& path, int targetPage, int batteryPct) {
  File f = LittleFS.open(path, "r");
  if (!f) { displayMessage("File error", path.c_str()); return; }

  int currentPage = 0, lineCount = 0;
  String line = "";
  std::vector<String> pageLines;

  while (f.available() && currentPage <= targetPage) {
    char c = f.read();
    if (c == '\n' || c == '\r') {
      while ((int)line.length() > CHARS_PER_LINE) {
        int breakAt = CHARS_PER_LINE;
        for (int i = CHARS_PER_LINE; i > CHARS_PER_LINE-6 && i > 0; i--)
          if (line[i] == ' ') { breakAt = i; break; }
        if (currentPage == targetPage) pageLines.push_back(line.substring(0, breakAt));
        line = line.substring(breakAt + 1);
        lineCount++;
        if (lineCount >= LINES_PER_PAGE) { currentPage++; lineCount = 0; }
      }
      if (line.length() > 0) {
        if (currentPage == targetPage) pageLines.push_back(line);
        lineCount++;
        if (lineCount >= LINES_PER_PAGE) { currentPage++; lineCount = 0; }
        line = "";
      }
    } else line += c;
  }
  f.close();

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(nullptr);
    display.setCursor(MARGIN, 8);
    String shortName = path.startsWith("/") ? path.substring(1) : path;
    if (shortName.length() > 14) shortName = shortName.substring(0, 14);
    display.print(shortName);
    display.setCursor(SCREEN_W - 28, 8);
    display.printf("%d%%", batteryPct);
    display.drawFastHLine(0, 12, SCREEN_W, GxEPD_BLACK);
    display.setFont(&FreeMono9pt7b);
    int y = 12 + LINE_HEIGHT + 2;
    for (auto& l : pageLines) {
      display.setCursor(MARGIN, y);
      display.print(l);
      y += LINE_HEIGHT;
      if (y > SCREEN_H - LINE_HEIGHT) break;
    }
    display.setFont(nullptr);
    display.drawFastHLine(0, SCREEN_H - 12, SCREEN_W, GxEPD_BLACK);
    display.setCursor(MARGIN, SCREEN_H - 4);
    display.printf("pg %d/%d", targetPage + 1, currentBook.totalPages);
  } while (display.nextPage());
}

void showBookMenu(const std::vector<String>& books, int selected) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(MARGIN, 16); display.print("Select Book");
    display.drawFastHLine(0, 20, SCREEN_W, GxEPD_BLACK);
    display.setFont(&FreeMono9pt7b);
    int y = 36;
    for (int i = 0; i < (int)books.size() && i < 8; i++) {
      if (i == selected) {
        display.fillRect(0, y-12, SCREEN_W, 14, GxEPD_BLACK);
        display.setTextColor(GxEPD_WHITE);
      } else display.setTextColor(GxEPD_BLACK);
      String name = books[i].startsWith("/") ? books[i].substring(1) : books[i];
      if (name.length() > 17) name = name.substring(0, 17);
      display.setCursor(MARGIN, y); display.print(name);
      y += 16;
    }
    display.setTextColor(GxEPD_BLACK);
    if (books.empty()) {
      display.setCursor(MARGIN, 60);  display.print("No books found.");
      display.setCursor(MARGIN, 80);  display.print("Hold MENU on boot");
      display.setCursor(MARGIN, 96);  display.print("WiFi:keychain-reader");
      display.setCursor(MARGIN, 112); display.print("-> 192.168.4.1");
    }
  } while (display.nextPage());
}

void saveBookmark() {
  File f = LittleFS.open("/bookmark.txt", FILE_WRITE);
  if (f) { f.printf("%s\n%d\n", currentBook.filename.c_str(), currentBook.pageIndex); f.close(); }
}

bool loadBookmark() {
  if (!LittleFS.exists("/bookmark.txt")) return false;
  File f = LittleFS.open("/bookmark.txt", "r");
  if (!f) return false;
  currentBook.filename = f.readStringUntil('\n'); currentBook.filename.trim();
  currentBook.pageIndex = f.readStringUntil('\n').toInt();
  f.close();
  return LittleFS.exists(currentBook.filename);
}