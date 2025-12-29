# 快速开始指南 (Quick Start Guide)

## 简介 (Introduction)

这是一个ESP32人体存在检测雷达项目的快速开始指南。按照以下步骤快速上手。

This is a quick start guide for the ESP32 Human Presence Detection Radar project. Follow these steps to get started quickly.

## 前置要求 (Prerequisites)

✅ ESP32开发板 (ESP32 Development Board)
✅ 人体存在检测雷达模块 (如 LD2410) (Human Presence Detection Radar Module like LD2410)
✅ USB数据线 (USB Cable)
✅ 杜邦线若干 (Jumper Wires)

## 5分钟快速设置 (5-Minute Quick Setup)

### 1️⃣ 安装软件 (Install Software)

1. 下载并安装 [VSCode](https://code.visualstudio.com/)
2. 在VSCode中安装 PlatformIO IDE 扩展

### 2️⃣ 克隆项目 (Clone Project)

```bash
git clone https://github.com/HiTechNinJa/WatchYourBehind.git
cd WatchYourBehind
code .
```

### 3️⃣ 连接硬件 (Connect Hardware)

```
雷达模块 → ESP32
-----------------
TX    → GPIO 16 (RX)
RX    → GPIO 17 (TX)
VCC   → 5V 或 3.3V
GND   → GND
```

### 4️⃣ 编译上传 (Build & Upload)

1. 连接ESP32到电脑 (Connect ESP32 to PC)
2. 在VSCode底部点击 ✓ 编译 (Click ✓ to build)
3. 点击 → 上传 (Click → to upload)
4. 点击 🔌 打开串口监视器 (Click 🔌 to open serial monitor)

### 5️⃣ 测试 (Test)

- 在雷达前挥手，LED应该亮起 (Wave hand in front of radar, LED should light up)
- 串口监视器显示检测信息 (Serial monitor shows detection info)

## 常见雷达模块设置 (Common Radar Module Settings)

### LD2410 系列 (LD2410 Series)
- 波特率: 256000 (默认) 或 115200
- 接线: 直连，TX→RX, RX→TX

### LD2450 系列 (LD2450 Series)  
- 波特率: 256000
- 接线: 直连，TX→RX, RX→TX

### HLK-LD 系列 (HLK-LD Series)
- 波特率: 115200 或 9600
- 接线: 直连，TX→RX, RX→TX

## 修改配置 (Modify Configuration)

编辑 `src/main.cpp`:

```cpp
#define RADAR_RX_PIN 16      // 改为你的接收引脚
#define RADAR_TX_PIN 17      // 改为你的发送引脚
#define RADAR_BAUDRATE 115200 // 改为你的雷达波特率
```

## 下一步 (Next Steps)

📖 阅读完整的 [README.md](README.md) 了解更多功能
🔧 根据你的雷达模块调整数据解析逻辑
🚀 添加更多功能，如WiFi上报、MQTT等

## 需要帮助？(Need Help?)

- 查看 [故障排除](README.md#故障排除-troubleshooting) 部分
- 提交 [Issue](https://github.com/HiTechNinJa/WatchYourBehind/issues)

## 许可证 (License)

本项目仅供学习研究使用 (For learning and research purposes only)
