#include <Arduino.h>

// 人体存在检测雷达配置 (Human Presence Detection Radar Configuration)
// 根据实际使用的雷达模块调整以下配置
// Adjust the following configuration based on your actual radar module

// 串口配置 (Serial Configuration)
#define RADAR_RX_PIN 16  // ESP32接收雷达数据的引脚 (ESP32 pin for receiving radar data)
#define RADAR_TX_PIN 17  // ESP32发送数据到雷达的引脚 (ESP32 pin for sending data to radar)
#define RADAR_BAUDRATE 115200  // 雷达模块波特率 (Radar module baud rate)

// 状态指示LED (Status LED)
#define LED_PIN 2  // ESP32内置LED (ESP32 built-in LED)

// 全局变量 (Global Variables)
HardwareSerial radarSerial(2);  // 使用Serial2与雷达通信 (Use Serial2 for radar communication)
bool presenceDetected = false;  // 存在检测状态 (Presence detection status)

void setup() {
  // 初始化串口用于调试 (Initialize serial for debugging)
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32 人体存在检测雷达系统 ===");
  Serial.println("=== ESP32 Human Presence Detection Radar System ===");
  
  // 初始化LED指示灯 (Initialize LED indicator)
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // 初始化雷达串口 (Initialize radar serial)
  radarSerial.begin(RADAR_BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  Serial.printf("雷达串口已初始化: RX=%d, TX=%d, Baudrate=%d\n", RADAR_RX_PIN, RADAR_TX_PIN, RADAR_BAUDRATE);
  Serial.printf("Radar serial initialized: RX=%d, TX=%d, Baudrate=%d\n", RADAR_RX_PIN, RADAR_TX_PIN, RADAR_BAUDRATE);
  
  Serial.println("系统初始化完成，开始监测...");
  Serial.println("System initialized, monitoring started...");
}

void loop() {
  // 检查雷达是否有数据 (Check if radar has data)
  if (radarSerial.available()) {
    // 读取雷达数据 (Read radar data)
    String radarData = radarSerial.readStringUntil('\n');
    
    // 输出原始数据用于调试 (Output raw data for debugging)
    Serial.print("雷达数据 (Radar data): ");
    Serial.println(radarData);
    
    // 根据雷达模块协议解析数据 (Parse data according to radar module protocol)
    // 这里需要根据实际使用的雷达模块进行适配
    // This needs to be adapted based on your actual radar module
    
    // 示例：简单的检测逻辑 (Example: Simple detection logic)
    // 如果接收到包含特定关键词的数据，认为检测到人体存在
    // If data contains specific keywords, consider presence detected
    if (radarData.indexOf("presence") >= 0 || radarData.indexOf("detected") >= 0 || radarData.indexOf("occupied") >= 0) {
      if (!presenceDetected) {
        presenceDetected = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("✓ 检测到人体存在 (Presence detected)");
      }
    } else if (radarData.indexOf("absence") >= 0 || radarData.indexOf("no_presence") >= 0 || radarData.indexOf("unoccupied") >= 0) {
      if (presenceDetected) {
        presenceDetected = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("✗ 未检测到人体存在 (No presence detected)");
      }
    }
  }
  
  // 检查PC端是否有命令 (Check if there are commands from PC)
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // 将PC命令转发给雷达 (Forward PC commands to radar)
    if (command.length() > 0) {
      Serial.print("发送命令到雷达 (Sending command to radar): ");
      Serial.println(command);
      radarSerial.println(command);
    }
  }
  
  delay(10);  // 短暂延时避免CPU占用过高 (Short delay to avoid high CPU usage)
}
