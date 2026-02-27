#include "stm32f10x.h"
#include "myiic.h"
#include <math.h> // 用于光照度计算

// ==========================================
//   全局变量 (请添加到 Watch 1 窗口观察)
// ==========================================
volatile uint8_t  found_addr = 0;      // 扫描到的真实 I2C 地址
volatile uint16_t manuf_id   = 0;      // 厂商 ID (应为 0x5449)
volatile uint16_t device_id  = 0;      // 设备 ID (应为 0x3001)
volatile uint16_t config_reg = 0;      // 读回的配置寄存器值
volatile float    current_lux = 0.0f;  // 最终光照度 (Lux)
volatile uint8_t  error_code = 0;      // 0=正常, 1=没找到设备, 2=ID错误

// ==========================================
//   辅助函数
// ==========================================

// 简单的软件延时
void Delay_ms(uint32_t ms) {
    uint32_t i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 7000; j++); // 72MHz下大约1ms
}

// 写入 OPT3001 寄存器 (16位)
void OPT3001_Write_Reg(uint8_t addr, uint8_t reg, uint16_t val) {
    IIC_Start();
    IIC_Send_Byte((addr << 1) | 0); // 写地址
    if (IIC_Wait_Ack()) { IIC_Stop(); return; }
    
    IIC_Send_Byte(reg);             // 寄存器地址
    IIC_Wait_Ack();
    
    IIC_Send_Byte((val >> 8) & 0xFF); // 写高字节
    IIC_Wait_Ack();
    
    IIC_Send_Byte(val & 0xFF);        // 写低字节
    IIC_Wait_Ack();
    
    IIC_Stop();
}

// 读取 OPT3001 寄存器 (16位)
uint16_t OPT3001_Read_Reg(uint8_t addr, uint8_t reg) {
    uint16_t val = 0;
    
    // 1. 写寄存器指针
    IIC_Start();
    IIC_Send_Byte((addr << 1) | 0); // 写模式
    if (IIC_Wait_Ack()) { IIC_Stop(); return 0; }
    
    IIC_Send_Byte(reg);             // 寄存器地址
    IIC_Wait_Ack();
    // 这里有些设备需要 Stop 再 Start，有些用 Restart，OPT3001 推荐 Restart
    
    // 2. 读数据
    IIC_Start();
    IIC_Send_Byte((addr << 1) | 1); // 读模式
    if (IIC_Wait_Ack()) { IIC_Stop(); return 0; }
    
    val = IIC_Read_Byte(1) << 8;    // 读高字节 (发送ACK)
    val |= IIC_Read_Byte(0);        // 读低字节 (发送NACK)
    
    IIC_Stop();
    return val;
}

// I2C 地址扫描
uint8_t IIC_Scan(void) {
    uint8_t i;
    for (i = 0x44; i <= 0x47; i++) { // 只扫描 OPT3001 可能的4个地址
        IIC_Start();
        IIC_Send_Byte((i << 1) | 0);
        if (IIC_Wait_Ack() == 0) { // 有人应答
            IIC_Stop();
            return i;
        }
        IIC_Stop();
        Delay_ms(5);
    }
    return 0; // 没找到
}

// 计算 Lux 数值 (参考数据手册)
float Calculate_Lux(uint16_t raw_val) {
    uint16_t exponent = (raw_val >> 12) & 0x0F; // 高4位是指数
    uint16_t mantissa = raw_val & 0x0FFF;       // 低12位是尾数
    
    // Lux = 0.01 * (2 ^ exponent) * mantissa
    return 0.01f * pow(2, exponent) * mantissa;
}

// ==========================================
//   主函数
// ==========================================
int main(void) {
    // 1. 上电等待 (非常重要！给传感器一点启动时间)
    Delay_ms(100); 

    // 2. 初始化底层 I2C (PB8/PB9)
    IIC_Init();
    
    // 3. 扫描设备
    found_addr = IIC_Scan();
    
    if (found_addr == 0) {
        error_code = 1; // 错误：未找到设备 (请检查接线/焊接)
        while(1) {
            Delay_ms(100); // 卡死在这里
        }
    }

    // 4. 读取 ID 验证
    manuf_id = OPT3001_Read_Reg(found_addr, 0x7E); // 厂商ID
    device_id = OPT3001_Read_Reg(found_addr, 0x7F); // 设备ID
    
    if (device_id != 0x3001) {
        error_code = 2; // 错误：设备ID不对
    }

    // 5. 配置传感器 (开启连续转换)
    // 0xC410: 自动量程(1100), 转换时间800ms(1), 连续模式(10)
    OPT3001_Write_Reg(found_addr, 0x01, 0xC410);
    
    // 读回来确认一下配置生效没
    config_reg = OPT3001_Read_Reg(found_addr, 0x01);

    // 6. 主循环
    while (1) {
        // 读取结果寄存器 (0x00)
        uint16_t raw_data = OPT3001_Read_Reg(found_addr, 0x00);
        
        // 计算光照度
        current_lux = Calculate_Lux(raw_data);
        
        // 延时 (转换时间需要800ms，所以延时要够)
        Delay_ms(1000);
    }
}