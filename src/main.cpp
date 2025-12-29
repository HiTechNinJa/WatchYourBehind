#include <Arduino.h>
#define RX_PIN 16
#define TX_PIN 17
// 待扫描的波特率列表 (包含常见值和雷达专用值)
const long BAUD_RATES[] = {
    256000, // 默认
    115200, // 常见 (如果雷达被改过)
    230400, // 备选
    1382400, // 高速备选
    9600,   
    19200,
    38400,
    57600,
    460800,
    250000, // 容错值 (-2.3%)
    260000  // 容错值 (+1.5%)
};
const int NUM_RATES = sizeof(BAUD_RATES) / sizeof(BAUD_RATES[0]);
void setup() {
    Serial.begin(256000); // 电脑端保持 256000 (配合 platformio.ini)
    delay(1000);
    Serial.println("\n=== Radar Baud Rate Scanner ===");
    Serial.println("Starting scan...");
}

// 坐标解析逻辑
int16_t parseValue(uint16_t raw) {
    if (raw & 0x8000) return (raw & 0x7FFF); 
    else return -(int16_t)(raw & 0x7FFF); 
}

// 锁定后的工作循环
void parseLoop(long lockedBaud) {
    Serial.printf("\n>>> TARGET LOCKED: %ld <<<\n", lockedBaud);
    Serial.println("Starting Real-time Parsing...");
    
    const uint8_t HEAD[] = {0xAA, 0xFF, 0x03, 0x00};
    const uint8_t TAIL[] = {0x55, 0xCC};
    static uint8_t buffer[64];
    static int bufIdx = 0;
    
    while(1) {
        if (Serial1.available()) {
            uint8_t b = Serial1.read();
            
            // 状态机
            if (bufIdx < 4) {
                if (b == HEAD[bufIdx]) buffer[bufIdx++] = b;
                else {
                    if (b == HEAD[0]) { buffer[0] = b; bufIdx = 1; }
                    else bufIdx = 0;
                }
            } else {
                buffer[bufIdx++] = b;
                if (bufIdx >= 30) {
                    if (buffer[28] == TAIL[0] && buffer[29] == TAIL[1]) {
                        String output = "";
                        for (int i=0; i<3; i++) {
                            int base = 4 + i*8;
                            int16_t x = parseValue(buffer[base] | (buffer[base+1]<<8));
                            int16_t y = parseValue(buffer[base+2] | (buffer[base+3]<<8));
                            if (x!=0 || y!=0) {
                                char tmp[32];
                                sprintf(tmp, "T%d(%d,%d) ", i+1, x, y);
                                output += String(tmp);
                            }
                        }
                        if (output.length() > 0) Serial.println(output);
                        else if (millis()%500 < 50) Serial.print("."); // 心跳点
                    }
                    bufIdx = 0;
                }
            }
        }
        // 防止看门狗复位
        yield();
    }
}

void loop() {
    for (int i = 0; i < NUM_RATES; i++) {
        long currentBaud = BAUD_RATES[i];
        Serial.printf("Scanning %ld... ", currentBaud);
        
        Serial1.end();
        Serial1.setRxBufferSize(2048);
        Serial1.begin(currentBaud, SERIAL_8N1, RX_PIN, TX_PIN);
        
        // 监听 1.5 秒
        unsigned long start = millis();
        int matchCount = 0; // 连续匹配头字节数
        bool found = false;
        
        while (millis() - start < 1500) {
            if (Serial1.available()) {
                uint8_t b = Serial1.read();
                
                // 寻找 AA FF 03 00
                if (matchCount == 0 && b == 0xAA) matchCount++;
                else if (matchCount == 1 && b == 0xFF) matchCount++;
                else if (matchCount == 2 && b == 0x03) matchCount++;
                else if (matchCount == 3 && b == 0x00) {
                    found = true;
                    break;
                } else {
                    // 重置，但考虑 AA 重入
                    if (b == 0xAA) matchCount = 1;
                    else matchCount = 0;
                }
            }
        }
        
        if (found) {
            parseLoop(currentBaud); // 找到后直接进入解析循环，不再退出
        } else {
            Serial.println("No");
        }
    }
    
    Serial.println("Scan complete. Retrying in 2s...");
    delay(2000);
}