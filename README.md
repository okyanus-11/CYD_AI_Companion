# CYD AI Companion

Touchscreen-only ESP32 chat interface using Wi-Fi and the Groq chat-completions API.

## Status

Recovered source prepared for repository use. The original configuration declarations were incomplete; they now contain safe placeholders so the sketch is syntactically complete. It will not connect until local Wi-Fi and Groq values are supplied. No hardware build or API integration test was possible in this environment.

## Hardware and dependencies

- ESP32-2432S028 CYD, 320x240 display
- ESP32 Arduino core (WiFi, WiFiClientSecure, HTTPClient, SPI)
- ArduinoJson 7 (the sketch uses the `JsonDocument` API)
- TFT_eSPI
- XPT2046_Touchscreen

Configure TFT_eSPI for the target CYD board and install the listed libraries through the Arduino IDE or equivalent package manager.

## Local configuration

Edit the three placeholder constants near the top of `CYD_AI_Companion.ino` locally. Keep real credentials out of Git. The sketch intentionally uses insecure TLS certificate handling (`setInsecure()`); do not use it for sensitive traffic without adding certificate verification.

## Verification

Not hardware-tested. Arduino CLI/PlatformIO was unavailable during review, so compilation remains to be verified on the target toolchain and board.

