# 雷达模块适配指南 (Radar Module Adaptation Guide)

本文档提供常见人体存在检测雷达模块的适配参考。

This document provides adaptation references for common human presence detection radar modules.

## LD2410 / LD2410B / LD2410C

### 基本信息 (Basic Information)
- 制造商: 海凌科 (Hi-Link)
- 通信协议: UART
- 默认波特率: 256000 (可配置为 9600, 19200, 38400, 57600, 115200, 230400, 256000, 460800)
- 工作电压: 5V DC

### 数据格式 (Data Format)

目标状态上报帧 (Target Status Report Frame):
```
帧头: FD FC FB FA
数据长度: 0D 00
数据类型: 02
目标状态: 00 (无目标) / 01 (运动目标) / 02 (静止目标) / 03 (运动+静止)
运动距离: XX
运动能量: XX
静止距离: XX
静止能量: XX
检测距离: XX XX
帧尾: 04 03 02 01
```

### 示例代码 (Example Code)

```cpp
// LD2410 配置
#define RADAR_BAUDRATE 256000
#define RADAR_RX_PIN 16
#define RADAR_TX_PIN 17

// 数据帧头
const uint8_t FRAME_HEADER[] = {0xFD, 0xFC, 0xFB, 0xFA};
const uint8_t FRAME_FOOTER[] = {0x04, 0x03, 0x02, 0x01};

void parseLD2410Data() {
  if (radarSerial.available() >= 14) {
    uint8_t buffer[14];
    radarSerial.readBytes(buffer, 14);
    
    // 验证帧头
    if (memcmp(buffer, FRAME_HEADER, 4) == 0) {
      uint8_t targetState = buffer[6];
      
      if (targetState > 0) {
        // 检测到目标
        presenceDetected = true;
        Serial.println("LD2410: 检测到人体");
      } else {
        presenceDetected = false;
        Serial.println("LD2410: 无人体");
      }
    }
  }
}
```

## LD2450

### 基本信息 (Basic Information)
- 制造商: 海凌科 (Hi-Link)
- 通信协议: UART
- 默认波特率: 256000
- 工作电压: 5V DC
- 特点: 多目标跟踪，最多支持3个目标

### 数据格式 (Data Format)

```
帧头: AA FF 03 00
目标1 X坐标: XX XX (2字节，有符号)
目标1 Y坐标: XX XX (2字节，有符号)
目标1 速度: XX XX (2字节，有符号)
目标1 分辨率: XX XX (2字节)
... (目标2、目标3数据)
帧尾: 55 CC
```

## HLK-LD2410

### 基本信息 (Basic Information)
- 制造商: 海凌科 (Hi-Link)
- 与LD2410相同，但封装不同

## 通用UART雷达适配模板 (Generic UART Radar Adaptation Template)

```cpp
// 在 src/main.cpp 的 loop() 中替换数据解析部分

void loop() {
  if (radarSerial.available()) {
    // 方法1: 基于文本协议 (Text-based Protocol)
    String radarData = radarSerial.readStringUntil('\n');
    
    if (radarData.indexOf("TARGET") >= 0) {
      presenceDetected = true;
      digitalWrite(LED_PIN, HIGH);
    } else if (radarData.indexOf("CLEAR") >= 0) {
      presenceDetected = false;
      digitalWrite(LED_PIN, LOW);
    }
    
    // 方法2: 基于二进制协议 (Binary Protocol)
    // 根据雷达文档定义帧格式
    /*
    if (radarSerial.available() >= FRAME_SIZE) {
      uint8_t buffer[FRAME_SIZE];
      radarSerial.readBytes(buffer, FRAME_SIZE);
      
      // 验证帧头和帧尾
      if (buffer[0] == FRAME_HEADER && buffer[FRAME_SIZE-1] == FRAME_FOOTER) {
        uint8_t status = buffer[STATUS_BYTE_INDEX];
        presenceDetected = (status == PRESENCE_VALUE);
      }
    }
    */
  }
}
```

## 调试技巧 (Debugging Tips)

### 1. 查看原始数据 (View Raw Data)

```cpp
void loop() {
  if (radarSerial.available()) {
    while (radarSerial.available()) {
      uint8_t b = radarSerial.read();
      Serial.printf("%02X ", b);
    }
    Serial.println();
  }
}
```

### 2. 十六进制转储 (Hex Dump)

```cpp
void hexDump(uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    Serial.printf("%02X ", data[i]);
    if ((i + 1) % 16 == 0) Serial.println();
  }
  Serial.println();
}
```

## 常见问题 (Common Issues)

### 1. 无数据输出 (No Data Output)
- 检查波特率设置
- 检查RX/TX引脚是否接反
- 检查雷达供电是否正常

### 2. 数据乱码 (Garbled Data)
- 波特率不匹配
- 接线质量问题，使用短线缆
- 检查共地

### 3. 检测不灵敏 (Low Sensitivity)
- 调整雷达灵敏度参数（需查阅雷达文档）
- 检查雷达安装位置和角度
- 确认雷达检测范围设置

## 资源链接 (Resource Links)

- LD2410 数据手册: [查阅厂商官网](https://www.hlktech.net/)
- PlatformIO 文档: https://docs.platformio.org/
- Arduino ESP32 文档: https://docs.espressif.com/projects/arduino-esp32/

## 贡献 (Contributing)

如果你成功适配了其他雷达模块，欢迎提交PR添加到本文档！

If you successfully adapted other radar modules, feel free to submit a PR to add them to this document!
