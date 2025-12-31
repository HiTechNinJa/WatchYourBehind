#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <WiFi.h>
#include <Arduino.h>

// ================= WiFi 配置 =================
// 🔧 如何修改WiFi设置：
// 1. 打开 src/wifi/wifi_config.cpp 文件
// 2. 修改 WIFI_SSID 为您的WiFi名称
// 3. 修改 WIFI_PASS 为您的WiFi密码
// 4. 保存文件并重新编译上传
//
// 示例：
// const char* WIFI_SSID = "MyHomeWiFi";
// const char* WIFI_PASS = "mysecurepassword123";
extern const char* WIFI_SSID;        // WiFi名称/SSID
extern const char* WIFI_PASS;        // WiFi密码

// ================= 服务器配置 =================
extern const char* SERVER_URL;       // 服务器URL（通常不需要修改）

// ================= 全局变量 =================
extern String deviceMac;                    // 设备MAC地址
extern unsigned long lastUploadTime;        // 上次上传时间
extern unsigned long uploadInterval;        // 上传间隔(毫秒)

// ================= WiFi 相关函数 =================

// WiFi连接状态检测与自动重连
void checkWiFiAndReconnect();

// 初始化WiFi连接
bool initWiFi();

// 获取WiFi连接状态信息
String getWiFiStatusInfo();

#endif // WIFI_CONFIG_H