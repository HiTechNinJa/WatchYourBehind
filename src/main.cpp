
#include <Arduino.h>
// 定义雷达专用引脚 (UART1)
#define RADAR_RX_PIN 16  // 接雷达 TX
#define RADAR_TX_PIN 17  // 接雷达 RX
// 统一波特率：256000
#define BAUDRATE 256000
void setup() {
    // ============== 关键修改：先设置缓冲区，再启动串口 ==============
    // 1. 设置电脑串口缓冲区 (UART0)
    // 增加发送缓冲区，防止雷达数据太快，发给电脑时堵塞
    Serial.setTxBufferSize(4096);
    // 2. 设置雷达串口缓冲区 (UART1)
    // 增加接收缓冲区，防止处理不及时导致丢包
    Serial1.setRxBufferSize(4096);
    // ============== 启动串口 ==============
    // 初始化电脑通信
    Serial.begin(BAUDRATE);
    // 初始化雷达通信
    Serial1.begin(BAUDRATE, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
    // 打印提示 (如果是为了连接上位机，建议注释掉下面3行，保证纯净)
    delay(500);
    Serial.println("\r\n=== Bridge Started (Buffer Fixed) ===");
}
void loop() {
    // ------------------------------------------------
    // 方向 1: 雷达 (Serial1) -> 电脑 (Serial)
    // ------------------------------------------------
    // 使用 readBytes + write 批量转发，效率比逐字节 read/write 高
    if (Serial1.available()) {
        size_t len = Serial1.available();
        uint8_t buf[256];
        
        // 最多读 256 字节，或者是当前可用长度
        size_t read_len = Serial1.readBytes(buf, min(len, sizeof(buf)));
        
        // 原样发给电脑
        Serial.write(buf, read_len);
    }

    // ------------------------------------------------
    // 方向 2: 电脑 (Serial) -> 雷达 (Serial1)
    // ------------------------------------------------
    if (Serial.available()) {
        size_t len = Serial.available();
        uint8_t buf[256];
        
        size_t read_len = Serial.readBytes(buf, min(len, sizeof(buf)));
        
        Serial1.write(buf, read_len);
    }
}