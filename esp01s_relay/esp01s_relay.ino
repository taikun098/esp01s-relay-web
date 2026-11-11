/*
 * ESP01S 继电器控制器（通用版 · 常运行轮询 · 稳定版）
 * - 两块板子烧同一份固件，上电用芯片 ID 自动注册到 NAS，网页改名
 * - kj00 → GPIO0 拉高 2 秒（开机）
 * - gj00 → GPIO0 拉高 10 秒（强制关机）
 * - ota  → 按指定 URL 升级固件
 * - 持续运行，每 2 秒轮询一次 NAS
 *
 * 开发板：Generic ESP8266 Module / Flash 1MB / DOUT / 40MHz
 */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>

// ==================== 配置区 ====================
const char* WIFI_SSID = "******";
const char* WIFI_PASS = "******";
const char* NAS_HOST  = "192.168.0.x";
const uint16_t NAS_PORT = 8090;
const uint32_t POLL_INTERVAL = 2000;
const char* FIRMWARE_VERSION = "1.2";
// ================================================

#define RELAY_PIN 0

String DEVICE_ID;
unsigned long lastPoll = 0;
bool wifiConnected = false;
unsigned long wifiStart = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n===== ESP01S 启动 =====");
  Serial.println("固件版本: " + String(FIRMWARE_VERSION));

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);   // 高电平触发：空闲断开为 LOW

  DEVICE_ID = "esp-" + String(ESP.getChipId(), HEX);
  Serial.println("设备ID: " + DEVICE_ID);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  Serial.println("连接WiFi: " + String(WIFI_SSID));
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  wifiStart = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    static unsigned long tick = 0;
    if (millis() - tick > 3000) {
      tick = millis();
      Serial.printf("WiFi未连接 status=%d\n", WiFi.status());
      // 掉线超过 30 秒才重新发起一次连接，平时交给 SDK 自动重试，避免反复 reconnect/扫描打断关联
      if (millis() - wifiStart > 30000) {
        Serial.println("重新发起连接...");
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        wifiStart = millis();
      }
    }
    delay(200);
    return;
  }

  if (!wifiConnected) {
    wifiConnected = true;
    wifiStart = millis();
    Serial.println("WiFi已连接, IP=" + WiFi.localIP().toString());
  }

  if (millis() - lastPoll < POLL_INTERVAL) return;
  lastPoll = millis();
  pollOnce();
}

void pollOnce() {
  char url[128];
  snprintf(url, sizeof(url), "http://%s:%d/api/poll/%s", NAS_HOST, NAS_PORT, DEVICE_ID.c_str());

  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);
  http.setTimeout(5000);
  int code = http.GET();
  if (code != 200) { http.end(); return; }
  String body = http.getString();
  http.end();
  Serial.println("收到: " + body);

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, body)) return;

  JsonVariant cmdNode = doc["cmd"];
  String cmdStr;
  String otaUrl;
  if (cmdNode.is<const char*>()) {
    cmdStr = cmdNode.as<const char*>();
    otaUrl = doc["url"].as<const char*>();   // 扁平结构 {"cmd":"ota","url":...}
  } else if (cmdNode.is<JsonObject>()) {
    cmdStr = cmdNode["cmd"].as<const char*>();
    otaUrl = cmdNode["url"].as<const char*>(); // 嵌套结构 {"cmd":{"cmd":"ota","url":...}}
  }
  if (cmdStr.length() == 0) return;

  if (cmdStr == "kj00") {
    pulseRelay(2000);
  } else if (cmdStr == "gj00") {
    pulseRelay(10000);
  } else if (cmdStr == "ota") {
    if (otaUrl.length()) {
      Serial.println("OTA升级中: " + otaUrl);
      Serial.printf("剩余OTA空间: %d 字节\n", ESP.getFreeSketchSpace());
      ESPhttpUpdate.rebootOnUpdate(true);
      WiFiClient oc;
      t_httpUpdate_return r = ESPhttpUpdate.update(oc, otaUrl);
      Serial.printf("OTA结果: %d, %s\n", (int)r, ESPhttpUpdate.getLastErrorString().c_str());
      Serial.println("OTA结束，继续运行");
    }
  }
}

void pulseRelay(unsigned long ms) {
  Serial.println("继电器动作 " + String(ms) + "ms");
  digitalWrite(RELAY_PIN, HIGH);   // 高电平触发：吸合
  delay(ms);
  digitalWrite(RELAY_PIN, LOW);     // 断开
}
