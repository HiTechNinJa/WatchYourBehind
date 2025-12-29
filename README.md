# WatchYourBehind - ESP32 人体存在检测雷达

软件工程实训之用。此仓库作为项目源码仓库

## 项目简介 (Project Introduction)

本项目是一个基于ESP32的人体存在检测雷达系统，使用毫米波雷达传感器进行人体存在检测。

This is an ESP32-based human presence detection radar system using mmWave radar sensors for human presence detection.

## 硬件要求 (Hardware Requirements)

- ESP32 开发板 (ESP32 Development Board)
- 人体存在检测雷达模块 (Human Presence Detection Radar Module)
  - 支持UART通信 (UART communication supported)
  - 推荐: LD2410, LD2450 或类似的毫米波雷达 (Recommended: LD2410, LD2450 or similar mmWave radar)
- USB数据线用于编程和调试 (USB cable for programming and debugging)

## 接线说明 (Wiring Instructions)

默认配置 (Default Configuration):
- 雷达 RX → ESP32 GPIO 17
- 雷达 TX → ESP32 GPIO 16
- 雷达 VCC → ESP32 5V/3.3V (根据雷达模块要求 / According to radar module requirements)
- 雷达 GND → ESP32 GND

## 开发环境设置 (Development Environment Setup)

### 1. 安装 VSCode (Install VSCode)

从 [Visual Studio Code官网](https://code.visualstudio.com/) 下载并安装

Download and install from [Visual Studio Code website](https://code.visualstudio.com/)

### 2. 安装 PlatformIO 扩展 (Install PlatformIO Extension)

1. 打开 VSCode (Open VSCode)
2. 点击左侧扩展图标 (Click Extensions icon on the left)
3. 搜索 "PlatformIO IDE" (Search for "PlatformIO IDE")
4. 点击安装 (Click Install)

### 3. 打开项目 (Open Project)

```bash
git clone https://github.com/HiTechNinJa/WatchYourBehind.git
cd WatchYourBehind
code .
```

### 4. 编译项目 (Build Project)

在 VSCode 中:
- 点击底部状态栏的 "√" 图标编译
- 或使用快捷键: `Ctrl+Alt+B`

In VSCode:
- Click the "√" icon in the bottom status bar to build
- Or use shortcut: `Ctrl+Alt+B`

### 5. 上传到ESP32 (Upload to ESP32)

在 VSCode 中:
- 点击底部状态栏的 "→" 图标上传
- 或使用快捷键: `Ctrl+Alt+U`

In VSCode:
- Click the "→" icon in the bottom status bar to upload
- Or use shortcut: `Ctrl+Alt+U`

### 6. 串口监视 (Serial Monitor)

在 VSCode 中:
- 点击底部状态栏的 "🔌" 图标打开串口监视器
- 或使用快捷键: `Ctrl+Alt+S`

In VSCode:
- Click the "🔌" icon in the bottom status bar to open serial monitor
- Or use shortcut: `Ctrl+Alt+S`

## 配置说明 (Configuration)

在 `src/main.cpp` 中可以修改以下配置:

You can modify the following configurations in `src/main.cpp`:

```cpp
#define RADAR_RX_PIN 16      // ESP32接收引脚 (ESP32 RX pin)
#define RADAR_TX_PIN 17      // ESP32发送引脚 (ESP32 TX pin)
#define RADAR_BAUDRATE 115200 // 波特率 (Baud rate)
#define LED_PIN 2            // LED指示灯引脚 (LED indicator pin)
```

## 功能特性 (Features)

- ✓ 实时人体存在检测 (Real-time human presence detection)
- ✓ 串口调试输出 (Serial debugging output)
- ✓ LED状态指示 (LED status indication)
- ✓ 双向串口通信 (Bidirectional serial communication)
- ✓ 可配置的引脚和波特率 (Configurable pins and baud rate)

## 使用方法 (Usage)

1. 按照接线说明连接硬件 (Connect hardware according to wiring instructions)
2. 编译并上传代码到ESP32 (Build and upload code to ESP32)
3. 打开串口监视器查看检测结果 (Open serial monitor to view detection results)
4. LED灯会根据检测状态亮灭 (LED will turn on/off based on detection status)
   - LED亮: 检测到人体存在 (LED on: Presence detected)
   - LED灭: 未检测到人体 (LED off: No presence)

## 扩展开发 (Extended Development)

### 添加自定义库 (Adding Custom Libraries)

在 `platformio.ini` 中添加库依赖:

Add library dependencies in `platformio.ini`:

```ini
lib_deps = 
    your-library-name
```

### 修改雷达协议 (Modifying Radar Protocol)

根据您使用的具体雷达模块，修改 `src/main.cpp` 中的数据解析逻辑。

Modify the data parsing logic in `src/main.cpp` according to your specific radar module.

## 故障排除 (Troubleshooting)

### 无法编译 (Build Fails)

- 确保已正确安装 PlatformIO (Ensure PlatformIO is properly installed)
- 检查网络连接，PlatformIO 需要下载工具链 (Check network connection, PlatformIO needs to download toolchains)

### 无法上传 (Upload Fails)

- 检查 USB 连接 (Check USB connection)
- 确保选择了正确的端口 (Ensure correct port is selected)
- 尝试按住 ESP32 的 BOOT 按钮再上传 (Try holding ESP32 BOOT button while uploading)

### 串口无数据 (No Serial Data)

- 检查波特率设置 (Check baud rate settings)
- 确认雷达模块已正确供电 (Confirm radar module is properly powered)
- 检查接线是否正确 (Check if wiring is correct)

## 许可证 (License)

本项目仅供学习和研究使用。

This project is for learning and research purposes only.

## 贡献 (Contributing)

欢迎提交 Issue 和 Pull Request！

Issues and Pull Requests are welcome!
