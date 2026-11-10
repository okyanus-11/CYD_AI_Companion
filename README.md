# CYD AI Companion

Recovered ESP32-2432S028 touchscreen chat firmware using Wi-Fi and the Groq
chat-completions API.

Build from `firmware/` with PlatformIO:

```bash
python -m platformio run
```

Copy `firmware/CYD_AI_Companion/config.example.h` to `config.h` for local
Wi-Fi and API settings. The example file documents the fields.

Groq HTTPS uses bundled GlobalSign and Google Trust Services root certificates.
The firmware synchronizes its clock with NTP before making HTTPS requests so
certificate validity dates can be checked. The current GlobalSign cross-sign
expires in January 2028; refresh the bundled trust anchors against Groq's
certificate chain before then.
