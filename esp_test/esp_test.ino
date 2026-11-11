/*
 * 最小 WiFi 连接测试：只连 WiFi、打印 IP，不扫描/不重连/不做业务。
 * 串口每 3 秒打印一次 status；连上后打印 IP。
 */
#include <ESP8266WiFi.h>

const char* WIFI_SSID = "******";
const char* WIFI_PASS = "******";

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== MINI WIFI TEST ===");
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void loop() {
  static bool gotIp = false;
  static unsigned long t = 0;
  if (WiFi.status() == WL_CONNECTED) {
    if (!gotIp) {
      gotIp = true;
      Serial.println("CONNECTED IP=" + WiFi.localIP().toString());
    }
  } else {
    gotIp = false;
    if (millis() - t > 3000) {
      t = millis();
      Serial.printf("status=%d\n", WiFi.status());
    }
  }
  delay(200);
}
