# WiFi 配置指南

## 📡 如何配置WiFi连接

### 步骤1：修改WiFi设置
1. 打开 `wifi_config.cpp` 文件
2. 找到以下行：
   ```cpp
   const char* WIFI_SSID = "YourWiFiSSID";     // 🔧 修改为您的WiFi名称
   const char* WIFI_PASS = "YourWiFiPassword"; // 🔧 修改为您的WiFi密码
   ```

3. 将 `"YourWiFiSSID"` 替换为您的WiFi名称
4. 将 `"YourWiFiPassword"` 替换为您的WiFi密码

### 示例配置
```cpp
// 如果您的WiFi名称是 "MyHomeWiFi"，密码是 "password123"
const char* WIFI_SSID = "MyHomeWiFi";
const char* WIFI_PASS = "password123";
```

### 步骤2：重新编译上传
修改配置后，需要重新编译并上传固件到ESP32：
```bash
platformio run --target upload
```

### 步骤3：验证连接
上传成功后，ESP32会显示：
```
Connecting to WiFi
WiFi connected!
IP: 192.168.x.x
Device MAC: XX:XX:XX:XX:XX:XX
```

## ⚠️ 注意事项
- WiFi名称和密码区分大小写
- 确保ESP32在您的WiFi信号覆盖范围内
- 如果连接失败，检查WiFi名称和密码是否正确
- 服务器URL通常不需要修改，除非您要连接到不同的服务器

## 🔧 高级配置
如果需要更复杂的WiFi管理（如WiFiManager自动配网），可以后续扩展此模块。</content>
<parameter name="filePath">h:\Code\WatchYourBehind\src\wifi\README.md