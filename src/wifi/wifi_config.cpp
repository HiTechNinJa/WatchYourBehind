#include "wifi_config.h"

// ================= 全局变量定义 =================
// 请修改以下配置以连接到您的WiFi网络
const char* WIFI_SSID = "YourWiFiSSID";        // 🔧 修改为您的WiFi名称
const char* WIFI_PASS = "YourWiFiPassword";    // 🔧 修改为您的WiFi密码
const char* SERVER_URL = "http://link2you.top:5000/api/v1/device/sync"; // 🔧 服务器地址（通常不需要修改）
String deviceMac = "";                      // 设备MAC地址
unsigned long lastUploadTime = 0;           // 上次上传时间
unsigned long uploadInterval = 1000;        // 默认1秒上传间隔

// WiFi连接状态检测与自动重连
void checkWiFiAndReconnect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] 连接丢失，正在尝试重连...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        int wifiTry = 0;
        while (WiFi.status() != WL_CONNECTED && wifiTry < 10) {
            delay(500);
            Serial.print(".");
            wifiTry++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[WiFi] 重新连接成功!");
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
            deviceMac = WiFi.macAddress();
        } else {
            Serial.println("\n[WiFi] 重新连接失败。");
        }
    }
}

// 初始化WiFi连接
bool initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    int wifiTry = 0;
    while (WiFi.status() != WL_CONNECTED && wifiTry < 20) {
        delay(500);
        Serial.print(".");
        wifiTry++;
    }

    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    if (wifiConnected) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        deviceMac = WiFi.macAddress();
        Serial.print("Device MAC: ");
        Serial.println(deviceMac);
    } else {
        Serial.println("\nWiFi connect failed, continue serial only.");
    }

    return wifiConnected;
}

// 获取WiFi连接状态信息
String getWiFiStatusInfo() {
    String info = "[WiFi] 状态: ";
    wl_status_t wifiStatus = WiFi.status();

    if (wifiStatus == WL_CONNECTED) {
        info += "已连接  IP: ";
        info += WiFi.localIP().toString();
        info += "  MAC: ";
        info += WiFi.macAddress();
    } else if (wifiStatus == WL_NO_SSID_AVAIL) {
        info += "找不到SSID";
    } else if (wifiStatus == WL_CONNECT_FAILED) {
        info += "连接失败";
    } else if (wifiStatus == WL_IDLE_STATUS) {
        info += "空闲";
    } else if (wifiStatus == WL_DISCONNECTED) {
        info += "未连接";
    } else {
        info += "未知";
    }

    return info;
}