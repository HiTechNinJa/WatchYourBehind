
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
// ================= 网络配置 =================
const char* WIFI_SSID = "Link2you?"; // WiFi YOUR_WIFI_SSID
const char* WIFI_PASS = "12345678";//YOUR_WIFI_PASSWORD
const char* SERVER_URL = "http://link2you.top:5000/api/v1/device/sync";

unsigned long lastUploadTime = 0;
unsigned long uploadInterval = 1000; // 默认1秒

// 目标数据结构
struct Target {
    int16_t x;
    int16_t y;
    int16_t speed;
    int16_t resolution;
};

Target targets[3];

// 补充全局变量声明
extern uint8_t radarBuf[64];
int16_t parseCoordinate(uint16_t raw);

// 自动检测相关全局变量
int lastKnownMode = -1; // -1 表示未知，用于对比配置变化

// 解析雷达数据并填充targets数组
void parseTargetsFromRadarBuf() {
    for (int i = 0; i < 3; i++) {
        int base = 4 + i * 8;
        targets[i].x = parseCoordinate(radarBuf[base] | (radarBuf[base+1]<<8));
        targets[i].y = parseCoordinate(radarBuf[base+2] | (radarBuf[base+3]<<8));
        targets[i].speed = parseCoordinate(radarBuf[base+4] | (radarBuf[base+5]<<8));
        targets[i].resolution = radarBuf[base+6] | (radarBuf[base+7]<<8);
    }
}

// 全局变量：仅开机获取一次MAC
String deviceMac = "";

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

// ================= 引脚定义 =================
#define RX_PIN 16
#define TX_PIN 17

// ================= 全局变量 =================
long currentBaudRate = 256000;
bool isBaudLocked = false;
unsigned long lastDataPrintTime = 0;
const unsigned long DATA_PRINT_INTERVAL = 3000; // 3秒输出一次雷达坐标数据

// raw模式限流
unsigned long lastRawPrintTime = 0;
const unsigned long RAW_PRINT_INTERVAL = 1000; // 1秒输出一次raw数据

// 显示模式控制
bool viewRawMode = false; // false=解析模式(默认), true=透传(Hex)模式

// 自动检测相关
unsigned long lastAutoCheckTime = 0;
const unsigned long AUTO_CHECK_INTERVAL = 10000; // 10秒检查一次

// 串口接收缓冲区
String inputString = "";
bool stringComplete = false;

// 安全确认机制变量
bool awaitingConfirmation = false;
String pendingCmdName = "";
uint16_t pendingCmdWord = 0;
uint16_t pendingCmdValInt = 0;
uint16_t pendingCmdLen = 0; 

// 雷达数据缓冲区
const uint8_t HEAD[] = {0xAA, 0xFF, 0x03, 0x00};
const uint8_t TAIL[] = {0x55, 0xCC};
uint8_t radarBuf[64];
int radarBufIdx = 0;

// ================= 协议指令集 (Hex) =================
const uint8_t CMD_HEAD[] = {0xFD, 0xFC, 0xFB, 0xFA};
const uint8_t CMD_TAIL[] = {0x04, 0x03, 0x02, 0x01};

// ================= 函数声明 =================
void sendRadarPacket(uint16_t cmdWord, uint8_t* value, uint16_t valueLen);
bool waitForAck(uint16_t sentCmd, unsigned long timeoutMs = 600, bool silent = false);
void printHelp(bool showAll);
void scanBaudRate();
void parseRadarByte(uint8_t b);
int16_t parseCoordinate(uint16_t raw);
void uploadDataToServer();

// 核心执行函数
void runCmd(const char* name, uint16_t cmdWord, uint8_t* val, uint16_t valLen);
void runCmd(const char* name, uint16_t cmdWord, uint16_t valInt);
void enableConfig(bool silent = false);
void endConfig(bool silent = false);

// [新增] 脏数据清理函数
void clearSerialBuffer() {
    unsigned long start = millis();
    // 持续读取直到缓冲区为空，或者超过50ms（防止死循环）
    while ((Serial1.available() > 0) && (millis() - start < 50)) {
        Serial1.read();
    }
}

// 危险操作请求函数
void requestAction(const char* name, uint16_t cmdWord, uint16_t valInt, uint16_t len) {
    Serial.printf("\n[!!! WARNING !!!] You are about to execute: %s\n", name);
    Serial.println("Type 'yes' to confirm, or anything else to cancel.");
    
    // 存储挂起的命令
    pendingCmdName = String(name);
    pendingCmdWord = cmdWord;
    pendingCmdValInt = valInt;
    pendingCmdLen = len; 
    awaitingConfirmation = true;
}


// JSON封装并上传到服务器（根据当前模式动态上传目标数量）
void uploadDataToServer() {
    checkWiFiAndReconnect();
    if (WiFi.status() != WL_CONNECTED) return;
    WiFiClient client;
    HTTPClient http;
    http.begin(client, SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    // 使用新版API，消除警告
    ArduinoJson::JsonDocument doc;
    doc["device_mac"] = deviceMac;
    auto arr = doc["targets"].to<ArduinoJson::JsonArray>();

    // 根据lastKnownMode判断上传目标数量
    // 0x01=单目标，0x02=多目标，-1=未知（默认多目标）
    int targetCount = (lastKnownMode == 0x01) ? 1 : 3;
    for (int i = 0; i < targetCount; i++) {
        auto obj = arr.add<ArduinoJson::JsonObject>();
        obj["x"] = targets[i].x;
        obj["y"] = targets[i].y;
        obj["speed"] = targets[i].speed;
        obj["resolution"] = targets[i].resolution;
    }

    String payload;
    serializeJson(doc, payload);

    int httpCode = http.POST(payload);
    if (httpCode > 0) {
        String resp = http.getString();
        // 解析响应，动态调整上传间隔（支持加速/降频）
        ArduinoJson::JsonDocument respDoc;
        ArduinoJson::DeserializationError err = deserializeJson(respDoc, resp);
        if (!err && respDoc["data"]["next_interval"]) {
            unsigned long nextInt = respDoc["data"]["next_interval"].as<unsigned long>();
            // 只在值变化时打印提示
            if (nextInt != uploadInterval) {
                if (nextInt <= 100) {
                    Serial.println("[SYNC] 进入加速上传模式 (10Hz)");
                } else {
                    Serial.println("[SYNC] 切换为低频上传 (1Hz)");
                }
            }
            uploadInterval = nextInt;
        }
        // 处理pending_cmd等可扩展
    }
    http.end();
}

// 批量查询当前状态，增加WiFi状态显示
void queryAllInfo() {
    Serial.println("\n=== Fetching Device Status ===");
    // WiFi状态
    Serial.print("[WiFi] 状态: ");
    wl_status_t wifiStatus = WiFi.status();
    if (wifiStatus == WL_CONNECTED) {
        Serial.print("已连接  IP: ");
        Serial.print(WiFi.localIP());
        Serial.print("  MAC: ");
        Serial.println(WiFi.macAddress());
    } else if (wifiStatus == WL_NO_SSID_AVAIL) {
        Serial.println("找不到SSID");
    } else if (wifiStatus == WL_CONNECT_FAILED) {
        Serial.println("连接失败");
    } else if (wifiStatus == WL_IDLE_STATUS) {
        Serial.println("空闲");
    } else if (wifiStatus == WL_DISCONNECTED) {
        Serial.println("未连接");
    } else {
        Serial.println("未知");
    }
    runCmd("Query Version", 0x00A0, NULL, 0); delay(100); 
    runCmd("Query MAC", 0x00A5, (uint16_t)0x0001); delay(100);
    runCmd("Query Mode", 0x0091, NULL, 0); delay(100);
    runCmd("Query Zone", 0x00C1, NULL, 0);
    Serial.println("=== Status Report Complete ===\n");
}

// === 后台自动检测 ===
void performAutoCheck() {
    enableConfig(true); // 静默进入
    delay(20); 
    sendRadarPacket(0x0091, NULL, 0); 
    waitForAck(0x0091, 300, true); 
    delay(20); 
    endConfig(true); // 静默退出
}

// === 透传桥接模式 ===
void runBridgeMode() {
    Serial.println("\n\n================================================");
    Serial.println("       ENTERING TRANSPARENT BRIDGE MODE         ");
    Serial.println("================================================");
    Serial.println("1. ESP32 is now a USB-TTL bridge.");
    Serial.println("2. CLOSE this Serial Monitor now.");
    Serial.println("3. OPEN 'HLK-LD2450 Tool' and connect to this COM port.");
    Serial.println("4. To Exit: Press the RST (Reset) button on ESP32.");
    Serial.println("================================================\n");
    
    delay(100);
    clearSerialBuffer(); // 进入前清理

    while(true) {
        if (Serial.available()) Serial1.write(Serial.read());
        if (Serial1.available()) Serial.write(Serial1.read());
        yield();
    }
}

// === 发送自定义 HEX 字符串 ===
void sendRawHex(String commandStr) {
    String hexStr = commandStr.substring(4);
    hexStr.trim();
    if (hexStr.length() == 0) { Serial.println("Usage: send FD FC FB FA ..."); return; }

    Serial.print("[TX RAW] ");
    String currentByte = "";
    for (unsigned int i = 0; i < hexStr.length(); i++) {
        char c = hexStr.charAt(i);
        if (c == ' ') continue;
        currentByte += c;
        if (currentByte.length() == 2) {
            uint8_t val = (uint8_t)strtol(currentByte.c_str(), NULL, 16);
            Serial1.write(val);
            Serial.printf("%02X ", val);
            currentByte = "";
        }
    }
    if (currentByte.length() > 0) {
        uint8_t val = (uint8_t)strtol(currentByte.c_str(), NULL, 16);
        Serial1.write(val);
        Serial.printf("%02X ", val);
    }
    Serial.println();
    
    if (!viewRawMode) {
        Serial.println("(Switching to RAW view temporarily for response...)");
        unsigned long startWait = millis();
        while(millis() - startWait < 1000) {
            if (Serial1.available()) Serial.printf("%02X ", Serial1.read());
        }
        Serial.println("\n(Done)");
    }
}

// ================= 标准指令封装 =================

void enableConfig(bool silent) {
    uint8_t val[] = {0x01, 0x00};
    if (!silent) Serial.print("[CMD] Enabling Config... ");
    sendRadarPacket(0x00FF, val, 2);
    waitForAck(0x00FF, 600, silent); 
}

void endConfig(bool silent) {
    if (!silent) Serial.print("[CMD] Ending Config...   ");
    sendRadarPacket(0x00FE, NULL, 0);
    waitForAck(0x00FE, 600, silent);
}

void runCmd(const char* name, uint16_t cmdWord, uint8_t* val, uint16_t valLen) {
    Serial.printf("\n--- Executing: %s ---\n", name);
    
    // [Fix] 这里的清理依然保留
    clearSerialBuffer();

    enableConfig(false);
    delay(50);
    
    Serial.printf("[CMD] Sending Packet...  ");
    sendRadarPacket(cmdWord, val, valLen);
    waitForAck(cmdWord, 600, false); 
    delay(50);
    
    endConfig(false);
    Serial.println("--- Done ---\n");
}

void runCmd(const char* name, uint16_t cmdWord, uint16_t valInt) {
    uint8_t valArr[2];
    valArr[0] = (uint8_t)(valInt & 0xFF);
    valArr[1] = (uint8_t)((valInt >> 8) & 0xFF);
    runCmd(name, cmdWord, valArr, 2);
}

// ================= Setup & Loop =================

void setup() {
    Serial.begin(256000);
    inputString.reserve(200);
    
    delay(1000);
    Serial.println("\n\n==============================================");
    Serial.println("      LD2450 Radar Controller (Safe Init)     ");
    Serial.println("==============================================");

    // WiFi连接
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi");
    int wifiTry = 0;
    while (WiFi.status() != WL_CONNECTED && wifiTry < 20) {
        delay(500);
        Serial.print(".");
        wifiTry++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        // 仅开机时获取一次MAC
        deviceMac = WiFi.macAddress();
        Serial.print("Device MAC: ");
        Serial.println(deviceMac);
    } else {
        Serial.println("\nWiFi connect failed, continue serial only.");
    }

    scanBaudRate();

    // [新增] 上电后自动重启雷达
    runCmd("Initial Reboot Module", 0x00A3, NULL, 0);
}

void loop() {
    // 0. 定期检测WiFi连接并自动重连
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 5000) { // 每5秒检测一次
        checkWiFiAndReconnect();
        lastWiFiCheck = millis();
    }

    // 1. 处理 PC 串口输入
    if (Serial.available()) {
        char inChar = (char)Serial.read();
        if (inChar == '\n' || inChar == '\r') {
            if (inputString.length() > 0) stringComplete = true;
        } else {
            inputString += inChar;
        }
    }

    if (stringComplete) {
        String cmd = inputString;
        cmd.trim();
        inputString = "";
        stringComplete = false;
        
        if (cmd.length() > 0) {
            
            // --- 安全确认流程 ---
            if (awaitingConfirmation) {
                if (cmd.equalsIgnoreCase("yes")) {
                    Serial.println(">> Confirmed. Executing...");
                    
                    if (pendingCmdLen == 0) {
                        runCmd(pendingCmdName.c_str(), pendingCmdWord, NULL, 0);
                    } else {
                        uint8_t valArr[2];
                        valArr[0] = (uint8_t)(pendingCmdValInt & 0xFF);
                        valArr[1] = (uint8_t)((pendingCmdValInt >> 8) & 0xFF);
                        runCmd(pendingCmdName.c_str(), pendingCmdWord, valArr, pendingCmdLen);
                    }
                    
                } else {
                    Serial.println(">> Cancelled.");
                }
                awaitingConfirmation = false;
                return; 
            }

            // --- 透传/发送指令 ---
            if (cmd.equalsIgnoreCase("bridge")) {
                runBridgeMode(); 
            }
            else if (cmd.startsWith("send ")) {
                sendRawHex(cmd);
            }
            // --- 普通命令解析 ---
            else if (cmd == "?" || cmd == "help") {
                printHelp(false);
            }
            else if (cmd == "-a" || cmd == "all") {
                printHelp(true);
            }
            else if (cmd.equalsIgnoreCase("scan")) {
                scanBaudRate();
            }
            else if (cmd.equalsIgnoreCase("info")) { 
                queryAllInfo();
            }
            // === 视图切换指令 ===
            else if (cmd.equalsIgnoreCase("raw")) {
                viewRawMode = true;
                Serial.println("\n>>> 已切换为【透传模式】(Hex View) <<<");
            }
            else if (cmd.equalsIgnoreCase("parse")) {
                viewRawMode = false;
                Serial.println("\n>>> 已切换为【解析模式】(Parsed View) <<<");
            }
            // ===================
            else if (cmd.equalsIgnoreCase("ver")) {
                runCmd("Query Version", 0x00A0, NULL, 0);
            }
            else if (cmd.equalsIgnoreCase("mac")) {
                runCmd("Query MAC", 0x00A5, (uint16_t)0x0001); 
            }
            else if (cmd.equalsIgnoreCase("zone")) {
                runCmd("Query Zone", 0x00C1, NULL, 0);
            }
            else if (cmd.equalsIgnoreCase("mode")) {
                runCmd("Query Mode", 0x0091, NULL, 0);
            }
            
            // === 配置指令 ===
            else if (cmd.equalsIgnoreCase("set single")) {
                runCmd("Set Single Target", 0x0080, NULL, 0);
            }
            else if (cmd.equalsIgnoreCase("set multi")) {
                runCmd("Set Multi Target", 0x0090, NULL, 0);
            }
            else if (cmd.equalsIgnoreCase("bleon")) {
                runCmd("BLE ON", 0x00A4, (uint16_t)0x0001);
            }
            else if (cmd.equalsIgnoreCase("bleoff")) {
                runCmd("BLE OFF", 0x00A4, (uint16_t)0x0000);
            }
            
            // === 危险指令 ===
            else if (cmd.equalsIgnoreCase("reboot")) {
                requestAction("Reboot Module", 0x00A3, 0, 0); 
            }
            else if (cmd.equalsIgnoreCase("factory")) {
                requestAction("Factory Reset", 0x00A2, 0, 0); 
            }
            else if (cmd.startsWith("baud")) {
                if (cmd.indexOf("256000") != -1) {
                    requestAction("Set Baud 256000", 0x00A1, 0x0007, 2); 
                } else if (cmd.indexOf("115200") != -1) {
                    requestAction("Set Baud 115200", 0x00A1, 0x0005, 2); 
                } else {
                    Serial.println("Usage: baud 256000");
                }
            }
            else {
                Serial.printf("Unknown: %s. Try '?'\n", cmd.c_str());
            }
        }
    }

    // 2. 自动检测逻辑
    if (!viewRawMode && isBaudLocked && millis() - lastAutoCheckTime > AUTO_CHECK_INTERVAL) {
        performAutoCheck();
        lastAutoCheckTime = millis();
    }

    // 3. 处理雷达数据流
    if (isBaudLocked && Serial1.available()) {
        uint8_t b = Serial1.read();
        parseRadarByte(b);

        // 检查是否完整帧并上报
        if (radarBufIdx == 0 && WiFi.status() == WL_CONNECTED) {
            parseTargetsFromRadarBuf();
            if (millis() - lastUploadTime > uploadInterval) {
                uploadDataToServer();
                lastUploadTime = millis();
            }
        }
    }
}

// ================= 辅助功能函数 =================

void printHelp(bool showAll) {
    Serial.println("\n\n================ LD2450 安全控制台 ================");
    Serial.println(" [提示] 输入命令后按回车发送");
    Serial.println(" [自动] 系统每10秒会自动检查一次配置(仅在解析模式下)。");
    
    Serial.println("\n--- 透传与连接 ---");
    Serial.printf("  %-14s : %s\n", "bridge", "【桥接】进入透明传输模式 (连接官方上位机用)");
    Serial.printf("  %-14s : %s\n", "raw", "【查看】显示原始 Hex 数据 (无延迟)");
    Serial.printf("  %-14s : %s\n", "parse", "【查看】显示解析后的坐标 (3秒一次)");
    Serial.printf("  %-14s : %s\n", "send <hex>", "【发送】发送原始 HEX (如: send FD FC ...)");

    Serial.println("\n--- 调试工具 ---");
    Serial.printf("  %-14s : %s\n", "info", "一键查询所有状态");

    Serial.println("\n--- 状态查询 ---");
    Serial.printf("  %-14s : %s\n", "mode", "查询当前追踪模式");
    Serial.printf("  %-14s : %s\n", "ver / mac", "查询版本 / MAC地址");

    Serial.println("\n--- 配置修改 ---");
    Serial.printf("  %-14s : %s\n", "set single", "切换为单目标模式");
    Serial.printf("  %-14s : %s\n", "set multi", "切换为多目标模式");
    Serial.printf("  %-14s : %s\n", "bleon / bleoff", "开关蓝牙");

    Serial.println("\n--- 危险操作 (需确认) ---");
    Serial.printf("  %-14s : %s\n", "reboot", "重启模块");
    Serial.printf("  %-14s : %s\n", "factory", "恢复出厂设置");
    Serial.println("===================================================");
    Serial.println("注: Target: [T1 x,y] 表示目标1的X坐标和Y坐标，单位为毫米");
    Serial.println();
}

void sendRadarPacket(uint16_t cmdWord, uint8_t* value, uint16_t valueLen) {
    uint16_t dataLen = 2 + valueLen;
    Serial1.write(CMD_HEAD, 4);
    Serial1.write((uint8_t)(dataLen & 0xFF));
    Serial1.write((uint8_t)((dataLen >> 8) & 0xFF));
    Serial1.write((uint8_t)(cmdWord & 0xFF));
    Serial1.write((uint8_t)((cmdWord >> 8) & 0xFF));
    if (valueLen > 0 && value != NULL) Serial1.write(value, valueLen);
    Serial1.write(CMD_TAIL, 4);
}

// 智能 ACK 解析函数 (支持 silent 模式)
bool waitForAck(uint16_t sentCmd, unsigned long timeoutMs, bool silent) {
    unsigned long start = millis();
    uint8_t ackBuf[64]; 
    int ackIdx = 0;
    
    while (millis() - start < timeoutMs) {
        if (Serial1.available()) {
            uint8_t b = Serial1.read();
            if (ackIdx < (int)sizeof(ackBuf)) ackBuf[ackIdx++] = b;
            
            if (ackIdx >= 10 && 
                ackBuf[ackIdx-4] == 0x04 && ackBuf[ackIdx-3] == 0x03 && 
                ackBuf[ackIdx-2] == 0x02 && ackBuf[ackIdx-1] == 0x01) {
                
                if (ackBuf[0] == 0xFD && ackBuf[1] == 0xFC) {
                     uint16_t status = ackBuf[8] | (ackBuf[9] << 8);
                     if (status != 0) {
                         if(!silent) Serial.printf("FAILED (Err: 0x%04X)\n", status);
                         return false;
                     }
                     
                     if (sentCmd == 0x00A4) {
                         Serial.println("SUCCESS -> Bluetooth config saved.");
                         Serial.println(">> NOTICE: Please execute 'reboot' command to apply changes! <<");
                     }
                     else if (sentCmd == 0x0091) { // mode
                         uint16_t mode = ackBuf[10] | (ackBuf[11] << 8);
                         
                         if (!silent) {
                             Serial.printf("SUCCESS -> Mode: %s\n", (mode == 0x02) ? "Multi Target" : "Single Target");
                         }
                         else {
                             if (lastKnownMode != -1 && mode != lastKnownMode) {
                                 Serial.println("\n\n-----------------------------------------");
                                 Serial.printf("[Auto-Check] ALERT: Mode Changed to %s!\n", (mode == 0x02) ? "Multi" : "Single");
                                 Serial.println("-----------------------------------------\n");
                             }
                         }
                         lastKnownMode = mode;
                     }
                     else if (!silent) {
                         if (sentCmd == 0x00A0) { // ver
                             uint16_t major = ackBuf[12] | (ackBuf[13] << 8);
                             uint32_t minor = ackBuf[14] | (ackBuf[15] << 8) | (ackBuf[16] << 16) | (ackBuf[17] << 24);
                             Serial.printf("SUCCESS -> Firmware: V%x.%02x.%08X\n", (major>>8), (major&0xFF), minor);
                         }
                         else if (sentCmd == 0x00A5) { // mac
                             Serial.printf("SUCCESS -> MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                                ackBuf[10], ackBuf[11], ackBuf[12], ackBuf[13], ackBuf[14], ackBuf[15]);
                         }
                         else if (sentCmd == 0x00C1) { // zone
                             uint16_t zType = ackBuf[10] | (ackBuf[11] << 8);
                             Serial.printf("SUCCESS -> Zone: Type=%d (0:Off, 1:Det, 2:Filt)\n", zType);
                             for (int i=0; i<3; i++) {
                                 int base = 12 + i*8;
                                 int16_t x1 = ackBuf[base] | (ackBuf[base+1]<<8);
                                 int16_t y1 = ackBuf[base+2] | (ackBuf[base+3]<<8);
                                 int16_t x2 = ackBuf[base+4] | (ackBuf[base+5]<<8);
                                 int16_t y2 = ackBuf[base+6] | (ackBuf[base+7]<<8);
                                 Serial.printf("           Z%d: (%d,%d)-(%d,%d)\n", i+1, x1, y1, x2, y2);
                             }
                         }
                         else if (sentCmd == 0x00FF) Serial.println("ENABLED");
                         else if (sentCmd == 0x00FE) Serial.println("ENDED");
                         else Serial.println("SUCCESS");
                     }
                     return true;
                }
                ackIdx = 0; 
            }
        }
    }
    if(!silent) Serial.println("TIMEOUT");
    return false;
}

// 扫描波特率逻辑 (增加清洗)
void scanBaudRate() {
    isBaudLocked = false;
    const long RATES[] = {256000, 115200}; 
    int numRates = sizeof(RATES) / sizeof(RATES[0]);
    
    Serial.println("\n--- Scanning Baud Rate ---");

    while(!isBaudLocked) {
        for (int i = 0; i < numRates; i++) {
            long rate = RATES[i];
            Serial.printf("Trying %ld... ", rate);
            
            Serial1.end();
            Serial1.setRxBufferSize(2048);
            Serial1.begin(rate, SERIAL_8N1, RX_PIN, TX_PIN);
            
            // [新增] 关键：等待串口稳定并清理脏数据
            delay(50);
            clearSerialBuffer();
            
            unsigned long scanStart = millis();
            int matchCount = 0;
            int cfgMatchCount = 0;
            bool found = false;
            
            while (millis() - scanStart < 1200) {
                if (Serial1.available()) {
                    uint8_t b = Serial1.read();
                    
                    if (matchCount == 0 && b == 0xAA) matchCount++;
                    else if (matchCount == 1 && b == 0xFF) matchCount++;
                    else if (matchCount == 2 && b == 0x03) matchCount++;
                    else if (matchCount == 3 && b == 0x00) { found = true; break; }
                    else { if (b == 0xAA) matchCount = 1; else matchCount = 0; }
                    
                    if (cfgMatchCount == 0 && b == 0xFD) cfgMatchCount++;
                    else if (cfgMatchCount == 1 && b == 0xFC) cfgMatchCount++;
                    else if (cfgMatchCount == 2 && b == 0xFB) cfgMatchCount++;
                    else if (cfgMatchCount == 3 && b == 0xFA) { found = true; break; }
                    else { if (b == 0xFD) cfgMatchCount = 1; else cfgMatchCount = 0; }
                }
            }
            if (found) {
                Serial.println("LOCKED!");
                currentBaudRate = rate;
                isBaudLocked = true;
                Serial.println("Ready. Type '?' for help.");
                printHelp(false);
                return;
            } else { Serial.println("No"); }
        }
        
        Serial.println("Scan failed. Auto-retrying in 2 seconds...");
        Serial.println("(Press any key to stop scanning and enter console)");
        
        unsigned long waitStart = millis();
        while(millis() - waitStart < 2000) {
            if (Serial.available()) {
                Serial.println("\n>> Scan aborted by user.");
                isBaudLocked = true; 
                return;
            }
        }
        Serial.println("\n--- Retrying Scan ---");
    }
}

int16_t parseCoordinate(uint16_t raw) {
    if (raw & 0x8000) return (int16_t)(raw & 0x7FFF);
    else return -(int16_t)(raw & 0x7FFF);
}

void parseRadarByte(uint8_t b) {
    if (radarBufIdx < 4) {
        if (b == HEAD[radarBufIdx]) radarBuf[radarBufIdx++] = b;
        else {
            if (b == HEAD[0]) { radarBuf[0] = b; radarBufIdx = 1; }
            else radarBufIdx = 0;
        }
    } else {
        radarBuf[radarBufIdx++] = b;
        if (radarBufIdx >= 64) radarBufIdx = 0;
        
        if (radarBufIdx >= 30 && radarBuf[28] == TAIL[0] && radarBuf[29] == TAIL[1]) {
            if (viewRawMode) {
                if (millis() - lastRawPrintTime > RAW_PRINT_INTERVAL) {
                    Serial.print("RAW: ");
                    for (int i = 0; i < radarBufIdx; i++) {
                        Serial.printf("%02X ", radarBuf[i]);
                    }
                    Serial.println();
                    lastRawPrintTime = millis();
                } else {
                    if (millis() % 500 < 20 && millis() % 100 == 0) Serial.print("🔄\n");
                }
            }
            else {
                if (millis() - lastDataPrintTime > DATA_PRINT_INTERVAL) {
                    String output = "Target: ";
                    bool hasTarget = false;
                    for (int i=0; i<3; i++) {
                        int base = 4 + i*8;
                        int16_t x = parseCoordinate(radarBuf[base] | (radarBuf[base+1]<<8));
                        int16_t y = parseCoordinate(radarBuf[base+2] | (radarBuf[base+3]<<8));
                        if (x != 0 || y != 0) {
                            char tmp[32];
                            sprintf(tmp, "[T%d %d,%d] ", i+1, x, y);
                            output += String(tmp);
                            hasTarget = true;
                        }
                    }
                    if (hasTarget) Serial.println(output);
                    else Serial.print(".");
                    lastDataPrintTime = millis();
                } else { 
                    if (millis() % 500 < 20 && millis() % 100 == 0) Serial.print("❤️\n"); 
                }
            }
            radarBufIdx = 0;
        }
    }
}