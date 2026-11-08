#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

// =====================================================
// WIFI AND GROQ CONFIGURATION
// =====================================================

// Local credentials live in config.h (git-ignored). config.example.h documents
// the fields; the repository ships placeholders only.
#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#endif

static const char* const kSsid = WIFI_SSID;
static const char* const kPassword = WIFI_PASSWORD;
static const char* const kGroqApiKey = GROQ_API_KEY;
static const char* const kModel = GROQ_MODEL;

const char* API_URL =
    "https://api.groq.com/openai/v1/chat/completions";

// =====================================================
// DISPLAY AND TOUCHSCREEN
// =====================================================

TFT_eSPI tft = TFT_eSPI();

#define TOUCH_CS   33
#define TOUCH_IRQ  36
#define TOUCH_SCK  25
#define TOUCH_MISO 39
#define TOUCH_MOSI 32

SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3900
#define TOUCH_MIN_Y 200
#define TOUCH_MAX_Y 3900

const int SW = 320;
const int SH = 240;

// =====================================================
// CHAT HISTORY
// =====================================================

struct ChatMessage {
  String role;
  String content;
};

const int MAX_MESSAGES = 10;

ChatMessage history[MAX_MESSAGES];
int messageCount = 0;

String inputText = "";
String statusText = "Starting...";

bool waitingForAI = false;
bool keyboardShift = false;

int scrollOffset = 0;
int g_totalLines = 0;
int g_maxScroll = 0;

// =====================================================
// SCREEN LAYOUT
// =====================================================

const int CHAT_X = 4;
const int CHAT_Y = 24;
const int CHAT_W = 312;
const int CHAT_H = 82;

const int INPUT_Y = 108;
const int INPUT_H = 22;

// Taller keys than the original 20 px rows, with a 23 px pitch so the
// 22 px buttons do not overlap.
const int KEY_Y1 = 132;
const int KEY_Y2 = 155;
const int KEY_Y3 = 178;
const int KEY_H = 22;

const int ACTION_Y = 202;
const int ACTION_H = 18;
const int STATUS_Y = 224;

// =====================================================
// ADD MESSAGE TO HISTORY
// =====================================================

void addMessage(String role, String content) {
  if (messageCount >= MAX_MESSAGES) {
    for (int i = 1; i < MAX_MESSAGES; i++) {
      history[i - 1] = history[i];
    }

    messageCount = MAX_MESSAGES - 1;
  }

  history[messageCount].role = role;
  history[messageCount].content = content;

  messageCount++;
  scrollOffset = 0;
}

// =====================================================
// COUNT WRAPPED LINES
// =====================================================

int getLineCount(String text, int width) {
  if (text.length() == 0) {
    return 1;
  }

  int lines = 0;
  int column = 0;

  for (int i = 0; i < text.length(); i++) {
    if (text[i] == '\n') {
      lines++;
      column = 0;
    } else {
      column++;

      if (column >= width) {
        lines++;
        column = 0;
      }
    }
  }

  if (column > 0) {
    lines++;
  }

  return lines;
}

// =====================================================
// DRAW CHAT HISTORY
// =====================================================

void drawChat() {
  tft.fillRect(
      CHAT_X, CHAT_Y, CHAT_W, CHAT_H, TFT_BLACK);

  tft.drawRect(
      CHAT_X, CHAT_Y, CHAT_W, CHAT_H, TFT_DARKGREY);

  const int charsPerLine = 49;
  const int lineHeight = 10;
  const int maxVisibleLines = (CHAT_H - 6) / lineHeight;

  String lines[100];
  uint16_t colors[100];
  int totalLines = 0;

  for (int i = 0; i < messageCount; i++) {
    if (totalLines >= 98) {
      break;
    }

    bool isUser = history[i].role == "user";

    lines[totalLines] = isUser ? "YOU:" : "AI:";
    colors[totalLines] = isUser ? TFT_CYAN : TFT_GREEN;
    totalLines++;

    String content = history[i].content;
    content.replace("\r", "");

    int start = 0;

    while (start < content.length() && totalLines < 99) {
      int end = min(
          start + charsPerLine,
          (int)content.length());

      if (end < content.length()) {
        int space = content.lastIndexOf(' ', end);

        if (space > start &&
            space - start > charsPerLine / 2) {
          end = space;
        }
      }

      String part = content.substring(start, end);
      part.trim();

      lines[totalLines] = part;
      colors[totalLines] = TFT_WHITE;
      totalLines++;

      start = end;

      while (start < content.length() &&
             content[start] == ' ') {
        start++;
      }
    }

    if (content.length() == 0 && totalLines < 99) {
      lines[totalLines] = "(empty)";
      colors[totalLines] = TFT_RED;
      totalLines++;
    }
  }

  // Keep the view inside the transcript, and remember the clamp so touch
  // scrolling cannot drift past either end.
  g_totalLines = totalLines;
  g_maxScroll = max(0, totalLines - maxVisibleLines);
  scrollOffset = constrain(scrollOffset, 0, g_maxScroll);

  int firstLine = max(
      0,
      totalLines - maxVisibleLines - scrollOffset);

  int lastLine = min(
      totalLines,
      firstLine + maxVisibleLines);

  tft.setTextSize(1);

  for (int i = firstLine; i < lastLine; i++) {
    int y = CHAT_Y + 4 + (i - firstLine) * lineHeight;

    tft.setTextColor(colors[i], TFT_BLACK);
    tft.setCursor(CHAT_X + 4, y);
    tft.print(lines[i]);
  }
}

// =====================================================
// DRAW KEYBOARD ROW
// =====================================================

void drawKeyboardRow(String keys, int y) {
  int keyW = SW / keys.length();

  for (int i = 0; i < keys.length(); i++) {
    int x = i * keyW;

    tft.fillRoundRect(
        x + 1, y, keyW - 2, KEY_H - 1,
        3, TFT_NAVY);

    tft.drawRoundRect(
        x + 1, y, keyW - 2, KEY_H - 1,
        3, TFT_LIGHTGREY);

    char c = keys[i];

    if (keyboardShift) {
      c = toupper(c);
    }

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);

    tft.setCursor(
        x + (keyW - 6) / 2,
        y + 6);

    tft.print(c);
  }
}

// =====================================================
// DRAW FULL INTERFACE
// =====================================================

void drawInterface() {
  tft.fillScreen(TFT_BLACK);

  // Header
  tft.fillRect(0, 0, SW, 22, TFT_DARKCYAN);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);

  tft.setCursor(5, 7);
  tft.print("CYD AI");

  tft.setCursor(224, 7);

  if (WiFi.status() == WL_CONNECTED) {
    tft.print("ONLINE");
  } else {
    tft.print("OFFLINE");
  }

  // Chat history
  drawChat();

  // Input box
  tft.fillRoundRect(
      4, INPUT_Y, 312, INPUT_H,
      3, TFT_DARKGREY);

  tft.drawRoundRect(
      4, INPUT_Y, 312, INPUT_H,
      3, TFT_LIGHTGREY);

  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(9, INPUT_Y + 7);

  String visibleInput = inputText;

  if (visibleInput.length() > 49) {
    visibleInput =
        visibleInput.substring(
            visibleInput.length() - 49);
  }

  tft.print(visibleInput);

  // Keyboard
  drawKeyboardRow("qwertyuiop", KEY_Y1);
  drawKeyboardRow("asdfghjkl", KEY_Y2);
  drawKeyboardRow("zxcvbnm,.", KEY_Y3);

  // SHIFT
  tft.fillRoundRect(
      2, ACTION_Y, 43, ACTION_H,
      3, TFT_MAROON);

  tft.setTextColor(TFT_WHITE, TFT_MAROON);
  tft.setCursor(5, ACTION_Y + 6);
  tft.print("SHIFT");

  // SPACE
  tft.fillRoundRect(
      47, ACTION_Y, 52, ACTION_H,
      3, TFT_DARKGREY);

  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setCursor(52, ACTION_Y + 6);
  tft.print("SPACE");

  // DELETE
  tft.fillRoundRect(
      101, ACTION_Y, 40, ACTION_H,
      3, TFT_DARKGREY);

  tft.setCursor(105, ACTION_Y + 6);
  tft.print("DEL");

  // CLEAR
  tft.fillRoundRect(
      143, ACTION_Y, 49, ACTION_H,
      3, TFT_DARKGREY);

  tft.setCursor(146, ACTION_Y + 6);
  tft.print("CLEAR");

  // SEND
  tft.fillRoundRect(
      194, ACTION_Y, 124, ACTION_H,
      3, TFT_GREEN);

  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.setCursor(240, ACTION_Y + 6);

  tft.print(waitingForAI ? "WAIT..." : "SEND");

  // Status
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(4, STATUS_Y);

  tft.print(statusText.substring(0, 52));
}

// =====================================================
// TOUCHSCREEN
// =====================================================

bool getTouchPoint(int &x, int &y) {
  if (!touch.touched()) {
    return false;
  }

  TS_Point p = touch.getPoint();

  x = map(
      p.x,
      TOUCH_MIN_X,
      TOUCH_MAX_X,
      0,
      SW - 1);

  y = map(
      p.y,
      TOUCH_MIN_Y,
      TOUCH_MAX_Y,
      0,
      SH - 1);

  x = constrain(x, 0, SW - 1);
  y = constrain(y, 0, SH - 1);

  return true;
}

// =====================================================
// GROQ API
// =====================================================

String askGroq(String prompt) {
  WiFiClientSecure client;

  // Prototype only: certificate verification is disabled.
  // Add proper TLS certificate verification before using
  // this device for sensitive conversations.
  client.setInsecure();

  HTTPClient https;

  if (!https.begin(client, API_URL)) {
    return "Could not initialize HTTPS";
  }

  https.setTimeout(45000);

  https.addHeader("Content-Type", "application/json");

  https.addHeader(
      "Authorization",
      String("Bearer ") + kGroqApiKey);

  JsonDocument request;

  request["model"] = kModel;

  JsonArray messages = request["messages"].to<JsonArray>();

  JsonObject systemMessage = messages.add<JsonObject>();
  systemMessage["role"] = "system";
  systemMessage["content"] =
      "You are a friendly AI companion running on a small "
      "device. Reply in 1-3 short sentences. Use plain text. "
      "Do not explain your reasoning. Answer immediately.";

  // Replay recent context. The current prompt is appended exactly once below,
  // so skip the copy that sendMessage() already put at the end of history.
  int contextStart = max(0, messageCount - 8);

  for (int i = contextStart; i < messageCount; i++) {
    if (i == messageCount - 1 &&
        history[i].role == "user" &&
        history[i].content == prompt) {
      continue;
    }

    if (history[i].content == "Thinking...") {
      continue;
    }

    JsonObject msg = messages.add<JsonObject>();

    msg["role"] = history[i].role;
    msg["content"] = history[i].content;
  }

  JsonObject userMessage = messages.add<JsonObject>();
  userMessage["role"] = "user";
  userMessage["content"] = prompt;

  // gpt-oss spends part of the completion budget on hidden reasoning. Ask for
  // concise reasoning and leave enough tokens for the visible answer, which is
  // what previously produced an empty string with finish_reason "length".
  request["reasoning_effort"] = "low";
  request["max_completion_tokens"] = 700;

  String body;
  serializeJson(request, body);

  Serial.println("\n===== GROQ REQUEST =====");
  Serial.println("Model: " + String(kModel));
  Serial.println("Prompt: " + prompt);

  int code = https.POST(body);
  String response = https.getString();

  Serial.printf("HTTP status: %d\n", code);

  https.end();

  if (code != 200) {
    Serial.println("===== API ERROR =====");
    Serial.println(response);

    if (code == 401) {
      return "Invalid API key";
    }

    if (code == 429) {
      return "Groq limit reached; try later";
    }

    if (code < 0) {
      return "Network error: " + String(code);
    }

    return "API error " + String(code);
  }

  JsonDocument result;

  DeserializationError error =
      deserializeJson(result, response);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());

    return "Could not parse API response";
  }

  Serial.println("\n===== GROQ RESPONSE =====");
  serializeJsonPretty(result, Serial);
  Serial.println("\n=========================");

  const char* finishReason =
      result["choices"][0]["finish_reason"] | "unknown";

  JsonVariant content =
      result["choices"][0]["message"]["content"];

  String answer = "";

  if (content.is<const char*>()) {
    answer = content.as<String>();
    answer.trim();
  }

  // A non-empty answer is still useful even if truncated.
  if (answer.length() > 0) {
    if (String(finishReason) == "length") {
      Serial.println("Warning: response reached token limit.");
    }

    Serial.println("===== ANSWER TO DISPLAY =====");
    Serial.println(answer);
    Serial.println("=============================");

    return answer;
  }

  Serial.println("No visible text in response.");
  Serial.print("Finish reason: ");
  Serial.println(finishReason);

  if (String(finishReason) == "length") {
    return "Reply cut short. Please try again.";
  }

  return String("No text returned. Finish: ") + finishReason;
}

// =====================================================
// SEND MESSAGE
// =====================================================

void sendMessage() {
  if (inputText.length() == 0 || waitingForAI) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    statusText = "Wi-Fi disconnected";
    drawInterface();
    return;
  }

  String prompt = inputText;
  inputText = "";

  addMessage("user", prompt);
  addMessage("assistant", "Thinking...");

  waitingForAI = true;
  statusText = "Contacting Groq...";
  scrollOffset = 0;

  drawInterface();

  String answer = askGroq(prompt);

  Serial.println("\n===== FINAL ANSWER =====");
  Serial.println(answer);

  // Replace the temporary assistant message.
  if (messageCount > 0 &&
      history[messageCount - 1].role == "assistant") {
    history[messageCount - 1].content = answer;
  }

  waitingForAI = false;
  statusText = "Reply received!";
  scrollOffset = 0;

  drawInterface();

  Serial.println("Display refreshed.");
}

// =====================================================
// TOUCH HANDLER
// =====================================================

void handleTouch(int x, int y) {
  // Chat scrolling
  if (y >= CHAT_Y && y < CHAT_Y + CHAT_H) {
    // Top half scrolls back through older messages; bottom half returns to
    // the newest line. Both are clamped to the transcript length.
    if (y < CHAT_Y + CHAT_H / 2) {
      scrollOffset = min(g_maxScroll, scrollOffset + 2);
    } else {
      scrollOffset = max(0, scrollOffset - 2);
    }

    drawInterface();
    return;
  }

  if (waitingForAI) {
    return;
  }

  // Keyboard row 1
  if (y >= KEY_Y1 && y < KEY_Y1 + KEY_H) {
    int index = x / (SW / 10);

    if (index < 10 && inputText.length() < 180) {
      char c = "qwertyuiop"[index];

      if (keyboardShift) {
        c = toupper(c);
      }

      inputText += c;
      keyboardShift = false;
    }
  }

  // Keyboard row 2
  else if (y >= KEY_Y2 && y < KEY_Y2 + KEY_H) {
    int index = x / (SW / 9);

    if (index < 9 && inputText.length() < 180) {
      char c = "asdfghjkl"[index];

      if (keyboardShift) {
        c = toupper(c);
      }

      inputText += c;
      keyboardShift = false;
    }
  }

  // Keyboard row 3
  else if (y >= KEY_Y3 && y < KEY_Y3 + KEY_H) {
    int index = x / (SW / 9);

    if (index < 9 && inputText.length() < 180) {
      char c = "zxcvbnm,."[index];

      if (keyboardShift) {
        c = toupper(c);
      }

      inputText += c;
      keyboardShift = false;
    }
  }

  // Action buttons
  else if (y >= ACTION_Y &&
           y < ACTION_Y + ACTION_H) {
    if (x < 46) {
      keyboardShift = !keyboardShift;
    } else if (x < 100) {
      if (inputText.length() < 180) {
        inputText += ' ';
      }
    } else if (x < 142) {
      if (inputText.length() > 0) {
        inputText.remove(inputText.length() - 1);
      }
    } else if (x < 193) {
      inputText = "";
    } else {
      sendMessage();
      return;
    }
  }

  drawInterface();
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);

  touchSPI.begin(
      TOUCH_SCK,
      TOUCH_MISO,
      TOUCH_MOSI,
      TOUCH_CS);

  touch.begin(touchSPI);
  touch.setRotation(1);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 20);

  tft.println("CYD AI");
  tft.println();
  tft.println("Connecting Wi-Fi...");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(kSsid, kPassword);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected!");
    Serial.println(WiFi.localIP());

    statusText = "Wi-Fi connected";

    addMessage(
        "assistant",
        "Hello! I'm ready to chat.");
  } else {
    Serial.println("Wi-Fi connection failed!");

    statusText = "Wi-Fi connection failed";

    addMessage(
        "assistant",
        "Check your Wi-Fi credentials.");
  }

  drawInterface();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  int x, y;

  // Keep trying the saved network instead of staying offline until reboot.
  static unsigned long lastWifiAttempt = 0;
  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastWifiAttempt > 15000) {
    lastWifiAttempt = millis();
    statusText = "Reconnecting Wi-Fi...";
    WiFi.disconnect();
    WiFi.begin(kSsid, kPassword);
    drawInterface();
  }

  if (getTouchPoint(x, y)) {
    handleTouch(x, y);

    unsigned long start = millis();

    while (touch.touched() &&
           millis() - start < 500) {
      delay(10);
    }

    delay(100);
  }
}
