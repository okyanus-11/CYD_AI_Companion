# Installation and flashing

## Hardware
- ESP32-2432S028 / ESP32-2432S028R Cheap Yellow Display (CYD)
- USB data cable
- 2.4 GHz Wi-Fi network
- A Groq API key

## Tools
Install Visual Studio Code with the PlatformIO IDE extension, or install PlatformIO Core:

```bash
python -m pip install --upgrade platformio
```

## Configure

1. Clone the repository and enter the firmware project:

   ```bash
   git clone https://github.com/okyanus-11/CYD_AI_Companion.git
   cd CYD_AI_Companion/firmware
   ```

2. Copy `CYD_AI_Companion/config.example.h` to `CYD_AI_Companion/config.h`.
3. Set `WIFI_SSID`, `WIFI_PASSWORD`, `GROQ_API_KEY`, and the desired `GROQ_MODEL`.
4. Configure `GROQ_ROOT_CA` with the verified PEM root CA that validates the HTTPS certificate chain for `api.groq.com`. The firmware intentionally rejects API requests while this value is empty; do not disable certificate verification or paste a short-lived leaf certificate.

## Build and upload

Connect the CYD by USB, identify its serial port, then from the `firmware` directory:

```bash
pio run
pio run -t upload --upload-port COMx
pio device monitor --port COMx --baud 115200
```

Replace `COMx` with the actual port on Windows (for example, `COM6`). On Linux, use the device path such as `/dev/ttyUSB0`. Close the serial monitor before uploading.

Keep `config.h` and API credentials private. The example configuration is safe to copy; never commit your populated local file.
