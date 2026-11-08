# CYD AI Companion

Recovered ESP32-2432S028 touchscreen chat firmware using Wi-Fi and the Groq
chat-completions API.

Build from `firmware/` with PlatformIO:

```bash
python -m platformio run
```

Copy `firmware/CYD_AI_Companion/config.example.h` to `config.h` for local
Wi-Fi and API settings. The example file documents the fields.
