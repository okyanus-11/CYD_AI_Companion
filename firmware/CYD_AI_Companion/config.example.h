// Copy to config.h and fill in locally. config.h is git-ignored.
#pragma once

#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// Create a key at https://console.groq.com/keys - never commit a real key.
#define GROQ_API_KEY    "YOUR_GROQ_API_KEY"

// Check https://console.groq.com/docs/models for currently served models.
// openai/gpt-oss-20b was the last model used by the original project.
#define GROQ_MODEL      "openai/gpt-oss-20b"

// Required for verified HTTPS. Paste the PEM root CA certificate that chains
// api.groq.com to a CA you trust. Leave empty to fail closed (no API requests).
// Use a raw string literal for the multiline PEM:
// #define GROQ_ROOT_CA R"EOF(
// -----BEGIN CERTIFICATE-----
// ... trusted root CA PEM ...
// -----END CERTIFICATE-----
// )EOF"
#define GROQ_ROOT_CA    ""
