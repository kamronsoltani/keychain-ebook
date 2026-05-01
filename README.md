# Kindle Keychain

A small e-ink reader for the CrowPanel ESP32-S3 2.13" built with PlatformIO.

This project turns the device into a keychain-style Kindle reader with:

- e-ink display rendering using `GxEPD2`
- local text file storage with `LittleFS`
- onboard file upload mode via a Wi-Fi access point
- simple page navigation and bookmark persistence
- battery voltage sensing support

## How it works

- On normal boot, the device loads a bookmark if present and opens the last read book.
- If there is no bookmark, it shows a book selection menu.
- Press `BTN_MENU`, `BTN_UP`, `BTN_DOWN`, and `BTN_SELECT` to navigate and read.
- Hold the `BTN_MENU` button while powering on to enter upload mode.
- In upload mode the device creates a Wi-Fi AP named `keychain-reader`.
- Visit `http://192.168.4.1/` to upload `.txt` files.

## Hardware configuration

The code is configured for the CrowPanel ESP32-S3 2.13" module with the following pins:

- Display SPI:
  - `EPD_MOSI` = 11
  - `EPD_SCK` = 12
  - `EPD_CS` = 10
  - `EPD_DC` = 9
  - `EPD_RST` = 8
  - `EPD_BUSY` = 7
- Buttons:
  - `BTN_MENU` = 0 (also used to enter upload mode)
  - `BTN_UP` = 35
  - `BTN_DOWN` = 36
  - `BTN_SELECT` = 37
- Battery ADC: `BAT_ADC` = 4

## Software structure

- `src/main.cpp` - application state management, button handling, sleep timer, and boot/upload mode logic
- `src/display.h` - display initialization and helper rendering functions
- `src/reader.h` - file listing, pagination, page rendering, bookmark save/load, and menu display
- `src/upload_server.h` - Wi-Fi access point, file upload web UI, and file management routes
- `src/pins.h` - pin definitions for display, buttons, and battery ADC

## Build and flash

Use PlatformIO from the project root:

```bash
pio run
pio run -t upload
pio device monitor
```

## Upload mode

To upload books:

1. Hold the `BTN_MENU` button while powering on the device.
2. Connect to the `keychain-reader` Wi-Fi network.
3. Open `http://192.168.4.1/` in a browser.
4. Drag and drop one or more `.txt` files.
5. The uploaded `.txt` files are stored in LittleFS and appear in the reader menu.

## File format

- Only plain text files with `.txt` extension are supported.
- Bookmark state is saved in `/bookmark.txt`.
- Uploaded books are stored at the filesystem root.

## PlatformIO environment

The project targets the `esp32-s3-devkitc-1` board using Arduino framework.

The required libraries are declared in `platformio.ini`:

- `zinggjm/GxEPD2`
- `mathieucarbou/ESPAsyncWebServer`
- `mathieucarbou/AsyncTCP`
- `bblanchon/ArduinoJson`
- `adafruit/Adafruit GFX Library`

## Notes

- The current implementation uses a simple text pagination algorithm with a fixed character width and line count.
- The display is configured for 122×250 pixel mode and text is wrapped by `CHARS_PER_LINE`.
- The upload UI is provided by `upload_server.h` and supports multiple file uploads, deletion, and storage usage reporting.
