# CYD AI Companion

Recovered ESP32-2432S028 touchscreen chat firmware using Wi-Fi and the Groq
chat-completions API.

## Appearance

Tap **THEME** in the top-right of the header to cycle Graphite, Midnight and
Light palettes. The selected theme is stored in ESP32 Preferences and survives
reboots.

## Build

Build from `firmware/` with PlatformIO:

```bash
python -m platformio run
```

Latest CI build: 46,264 bytes RAM (14.1%) and 786,665 bytes flash
(25.0% of the 3 MB application partition).

Copy `firmware/CYD_AI_Companion/config.example.h` to `config.h` for local
Wi-Fi and API settings. Keep `config.h` out of Git.

## HTTPS certificate verification

The firmware verifies Groq's HTTPS certificate and deliberately fails closed
if a trusted CA certificate is not configured. Set `GROQ_ROOT_CA` in your
local `config.h` to the PEM root CA that validates the current certificate
chain for `api.groq.com`. Use a raw string literal for the multiline PEM:

```cpp
#define GROQ_ROOT_CA R"EOF(
-----BEGIN CERTIFICATE-----
... trusted root CA certificate ...
-----END CERTIFICATE-----
)EOF"
```

Do not paste the server's short-lived leaf certificate: use the trusted root
CA, and verify the hostname and chain before trusting it. The device also
synchronizes its clock with NTP because certificate validity dates must be
checked. If the CA or clock is unavailable, API requests are rejected rather
than silently disabling TLS verification.

The placeholder `GROQ_ROOT_CA` in the example is empty on purpose. Add the
verified CA to your private `config.h` before expecting API requests to work.

## Installation

For prerequisites, environment setup, configuration, and run/build commands, see the [installation guide](INSTALLATION.md).
