#include <Arduino.h>

// ================= 引脚定义 =================
#define RX_PIN 16
#define TX_PIN 17

// ================= 全局变量 =================
long currentBaudRate = 256000;
bool isBaudLocked = false;
unsigned long lastDataPrintTime = 0;
const unsigned long DATA_PRINT_INTERVAL = 3000; // 3秒输出一次雷达坐标数据

// 自动检测相关
unsigned long lastAutoCheckTime = 0;
const unsigned long AUTO_CHECK_INTERVAL = 10000; // 10秒检查一次
int lastKnownMode = -1; // -1 表示未知，用于对比配置变化

// 串口接收缓冲区
String inputString = "";
bool stringComplete = false;

// 安全确认机制变量
bool awaitingConfirmation = false;
String pendingCmdName = "";
uint16_t pendingCmdWord = 0;
uint16_t pendingCmdValInt = 0;
bool pendingUseInt = true;

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

// 核心执行函数
void runCmd(const char* name, uint16_t cmdWord, uint8_t* val, uint16_t valLen);
void runCmd(const char* name, uint16_t cmdWord, uint16_t valInt);
void enableConfig(bool silent = false);
void endConfig(bool silent = false);

// 危险操作请求函数
void requestAction(const char* name, uint16_t cmdWord, uint16_t valInt) {
    Serial.printf("\n[!!! WARNING !!!] You are about to execute: %s\n", name);
    Serial.println("Type 'yes' to confirm, or anything else to cancel.");
    
    // 存储挂起的命令
    pendingCmdName = String(name);
    pendingCmdWord = cmdWord;
    pendingCmdValInt = valInt;
    pendingUseInt = true;
    awaitingConfirmation = true;
}

// 批量查询当前状态
void queryAllInfo() {
    Serial.println("\n=== Fetching Device Status ===");
    runCmd("Query Version", 0x00A0, NULL, 0); delay(50);
    runCmd("Query MAC", 0x00A5, (uint16_t)0x0001); delay(50);
    runCmd("Query Mode", 0x0091, NULL, 0); delay(50);
    runCmd("Query Zone", 0x00C1, NULL, 0);
    Serial.println("=== Status Report Complete ===\n");
}

// === 后台自动检测 ===
void performAutoCheck() {
    // 这是一个静默过程，除非发现配置变化
    // 注意：进入配置模式会短暂中断雷达数据上报
    
    enableConfig(true); // 静默进入
    
    // 发送查询模式指令
    sendRadarPacket(0x0091, NULL, 0); 
    // 静默等待ACK，waitForAck 内部负责比对 lastKnownMode
    waitForAck(0x0091, 300, true); 
    
    endConfig(true); // 静默退出
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
    enableConfig(false);
    Serial.printf("[CMD] Sending Packet...  ");
    sendRadarPacket(cmdWord, val, valLen);
    waitForAck(cmdWord, 600, false); 
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
    Serial.println("      LD2450 Radar Controller (Smart Check)   ");
    Serial.println("==============================================");
    
    scanBaudRate(); 
}

void loop() {
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
                    runCmd(pendingCmdName.c_str(), pendingCmdWord, pendingCmdValInt);
                } else {
                    Serial.println(">> Cancelled.");
                }
                awaitingConfirmation = false;
                return; 
            }

            // --- 普通命令解析 ---
            if (cmd == "?" || cmd == "help") {
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
                requestAction("Reboot Module", 0x00A3, 0);
            }
            else if (cmd.equalsIgnoreCase("factory")) {
                requestAction("Factory Reset", 0x00A2, 0);
            }
            else if (cmd.startsWith("baud")) {
                if (cmd.indexOf("256000") != -1) {
                    requestAction("Set Baud 256000", 0x00A1, 0x0007);
                } else if (cmd.indexOf("115200") != -1) {
                    requestAction("Set Baud 115200", 0x00A1, 0x0005);
                } else {
                    Serial.println("Usage: baud 256000");
                }
            }
            else {
                Serial.printf("Unknown: %s. Try '?'\n", cmd.c_str());
            }
        }
    }

    // 2. 自动检测逻辑 (每10秒执行一次)
    if (isBaudLocked && millis() - lastAutoCheckTime > AUTO_CHECK_INTERVAL) {
        performAutoCheck();
        lastAutoCheckTime = millis();
    }

    // 3. 处理雷达数据流
    if (isBaudLocked && Serial1.available()) {
        parseRadarByte(Serial1.read());
    }
}

// ================= 辅助功能函数 =================

void printHelp(bool showAll) {
    Serial.println("\n\n================ LD2450 安全控制台 ================");
    Serial.println(" [提示] 输入命令后按回车发送");
    Serial.println(" [自动] 系统每10秒会自动检查一次配置，如有变更会提示。");
    
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
    Serial.println("===================================================\n");
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
                     
                     // --- 0x0091 Mode Query 解析逻辑 ---
                     if (sentCmd == 0x0091) { // mode
                         uint16_t mode = ackBuf[10] | (ackBuf[11] << 8);
                         
                         // 如果是手动查询，总是打印
                         if (!silent) {
                             Serial.printf("SUCCESS -> Mode: %s\n", (mode == 0x02) ? "Multi Target" : "Single Target");
                         }
                         // 如果是自动检测，只有变化了才打印
                         else {
                             if (lastKnownMode != -1 && mode != lastKnownMode) {
                                 Serial.println("\n\n-----------------------------------------");
                                 Serial.printf("[Auto-Check] ALERT: Mode Changed to %s!\n", (mode == 0x02) ? "Multi" : "Single");
                                 Serial.println("-----------------------------------------\n");
                             }
                         }
                         // 更新记录
                         lastKnownMode = mode;
                     }
                     // --- 其他指令解析 (自动模式下通常不会触发这些) ---
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

void scanBaudRate() {
    isBaudLocked = false;
    const long RATES[] = {256000, 115200}; // 仅扫描常用，提高速度
    int numRates = sizeof(RATES) / sizeof(RATES[0]);
    
    Serial.println("\n--- Scanning Baud Rate ---");
    for (int i = 0; i < numRates; i++) {
        long rate = RATES[i];
        Serial.printf("Trying %ld... ", rate);
        Serial1.end();
        Serial1.setRxBufferSize(2048);
        Serial1.begin(rate, SERIAL_8N1, RX_PIN, TX_PIN);
        
        unsigned long scanStart = millis();
        int matchCount = 0;
        bool found = false;
        while (millis() - scanStart < 1200) {
            if (Serial1.available()) {
                uint8_t b = Serial1.read();
                if (matchCount == 0 && b == 0xAA) matchCount++;
                else if (matchCount == 1 && b == 0xFF) matchCount++;
                else if (matchCount == 2 && b == 0x03) matchCount++;
                else if (matchCount == 3 && b == 0x00) { found = true; break; }
                else { if (b == 0xAA) matchCount = 1; else matchCount = 0; }
            }
        }
        if (found) {
            Serial.println("LOCKED!");
            currentBaudRate = rate;
            isBaudLocked = true;
            Serial.println("Ready. Type '?' for help.");
            return;
        } else { Serial.println("No"); }
    }
    Serial.println("Scan failed.");
}

int16_t parseCoordinate(uint16_t raw) {
    if (raw & 0x8000) return -(int16_t)(raw & 0x7FFF);
    else return (int16_t)(raw & 0x7FFF);
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
            } else { if (millis() % 500 < 20 && millis() % 100 == 0) Serial.print("."); }
            radarBufIdx = 0;
        }
    }
}