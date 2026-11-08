// Copy to config.h and fill in locally. config.h is git-ignored.
#pragma once

#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// Create a key at https://console.groq.com/keys - never commit a real key.
#define GROQ_API_KEY    "YOUR_GROQ_API_KEY"

// Check https://console.groq.com/docs/models for currently served models.
// openai/gpt-oss-20b was the last model used by the original project.
#define GROQ_MODEL      "openai/gpt-oss-20b"
